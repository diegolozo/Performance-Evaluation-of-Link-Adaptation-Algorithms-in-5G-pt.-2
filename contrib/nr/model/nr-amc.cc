/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
// Modified by A. Gonzalez and D. Torreblanca (2024)
// SPDX-License-Identifier: GPL-2.0-only

#include "nr-amc.h"

#include "lena-error-model.h"
#include "nr-error-model.h"
#include "nr-lte-mi-error-model.h"
#include "nr-eesm-t1.h"

#include <ns3/double.h>
#include <ns3/enum.h>
#include <ns3/log.h>
#include <ns3/math.h>
#include <ns3/nr-spectrum-value-helper.h>
#include <ns3/uinteger.h>
#include "ns3/core-module.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NrAmc");
NS_OBJECT_ENSURE_REGISTERED(NrAmc);

uint16_t NrAmc::s_instanceNumber = 0;

NrAmc::NrAmc()
{
    std::cout << "CREANDO NR-AMC" << std::endl;
    std::cout << "--------------" << std::endl;
    m_my_num = s_instanceNumber;
    s_instanceNumber++;
    NS_LOG_INFO("Initialize AMC module");
}

NrAmc::~NrAmc()
{
    s_instanceNumber--;
}

void
NrAmc::SetDlMode()
{
    NS_LOG_FUNCTION(this);
    m_emMode = NrErrorModel::DL;
}

void
NrAmc::SetUlMode()
{
    NS_LOG_FUNCTION(this);
    m_emMode = NrErrorModel::UL;
}

TypeId
NrAmc::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NrAmc")
            .SetParent<Object>()
            .AddAttribute("NumRefScPerRb",
                          "Number of Subcarriers carrying Reference Signals per RB",
                          UintegerValue(1),
                          MakeUintegerAccessor(&NrAmc::SetNumRefScPerRb, &NrAmc::GetNumRefScPerRb),
                          MakeUintegerChecker<uint8_t>(0, 12))
            .AddAttribute("AmcModel",
                          "AMC model used to assign CQI",
                          EnumValue(NrAmc::ErrorModel),
                          MakeEnumAccessor<AmcModel>(&NrAmc::SetAmcModel, &NrAmc::GetAmcModel),
                          MakeEnumChecker(NrAmc::ErrorModel,
                                          "ErrorModel",
                                          NrAmc::ShannonModel,
                                          "ShannonModel"))
            .AddAttribute("ErrorModelType",
                          "Type of the Error Model to use when AmcModel is set to ErrorModel. "
                          "This parameter has to match the ErrorModelType in nr-spectrum-model,"
                          "because they need to refer to same MCS tables and indexes",
                          TypeIdValue(NrLteMiErrorModel::GetTypeId()),
                          MakeTypeIdAccessor(&NrAmc::SetErrorModelType, &NrAmc::GetErrorModelType),
                          MakeTypeIdChecker())
            .AddConstructor<NrAmc>();
    return tid;
}

double NrAmc::UpdateBlerTarget(double sinrEff) const
{
    BooleanValue useAI;
    
    if (GlobalValue::GetValueByNameFailSafe("useAI", useAI)) {
        if (!useAI) {
            return 0;
        }
    } else {
        NS_ABORT_MSG("useAI GlobalValue not found!");
    }

    auto interface = Ns3AiMsgInterface::Get();
    Ns3AiMsgInterfaceImpl<NrAmc::dataToSend, NrAmc::dataToRecv>* msgInterface =
        interface->GetInterface<NrAmc::dataToSend, NrAmc::dataToRecv>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyVector()->at(m_my_num).simulationTime = ns3::Simulator::Now().GetSeconds();
    msgInterface->GetCpp2PyVector()->at(m_my_num).sinrEffective = sinrEff;

    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    double blerTarget = msgInterface->GetPy2CppVector()->at(m_my_num).blerTarget;
    msgInterface->CppRecvEnd();

    // NS_LOG_DEBUG("[ Buffer " << this << "] Decided to " << _decision_char << " a packet." << std::endl);
    NS_LOG_DEBUG("El valor del sinrEff era " << sinrEff << " y el BLER Target quedó en " << blerTarget << std::endl);

    return blerTarget;
}

TypeId
NrAmc::GetInstanceTypeId() const
{
    return NrAmc::GetTypeId();
}

uint8_t
NrAmc::GetMcsFromCqi(uint8_t cqi) const
{
    NS_LOG_FUNCTION(cqi);
    NS_ASSERT_MSG(cqi >= 0 && cqi <= 15, "CQI must be in [0..15] = " << cqi);

    double spectralEfficiency = m_errorModel->GetSpectralEfficiencyForCqi(cqi);
    uint8_t mcs = 0;

    while ((mcs < m_errorModel->GetMaxMcs()) &&
           (m_errorModel->GetSpectralEfficiencyForMcs(mcs + 1) <= spectralEfficiency))
    {
        ++mcs;
    }

    NS_LOG_LOGIC("mcs = " << +mcs);

    return mcs;
}

uint8_t
NrAmc::GetNumRefScPerRb() const
{
    NS_LOG_FUNCTION(this);
    return m_numRefScPerRb;
}

void
NrAmc::SetNumRefScPerRb(uint8_t nref)
{
    NS_LOG_FUNCTION(this);
    m_numRefScPerRb = nref;
}

uint32_t
NrAmc::CalculateTbSize(uint8_t mcs, uint8_t rank, uint32_t nprb) const
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(mcs));

    NS_ASSERT_MSG(mcs <= m_errorModel->GetMaxMcs(),
                  "MCS=" << static_cast<uint32_t>(mcs) << " while maximum MCS is "
                         << static_cast<uint32_t>(m_errorModel->GetMaxMcs()));

    uint32_t payloadSize = GetPayloadSize(mcs, rank, nprb);
    uint32_t tbSize = payloadSize;

    if (m_errorModelType != LenaErrorModel::GetTypeId())
    {
        if (payloadSize >= m_crcLen)
        {
            tbSize =
                payloadSize - m_crcLen; // subtract parity bits of m_crcLen used in transport block
        }
        uint32_t cbSize =
            m_errorModel->GetMaxCbSize(payloadSize,
                                       mcs); // max size of a code block (including m_crcLen)
        if (tbSize > cbSize)                 // segmentation of the transport block occurs
        {
            double C = ceil(tbSize / cbSize);
            tbSize = payloadSize - static_cast<uint32_t>(
                                       C * m_crcLen); // subtract bits of m_crcLen used in code
                                                      // blocks, in case of code block segmentation
        }
    }

    NS_LOG_INFO(" mcs:" << (unsigned)mcs << " TB size:" << tbSize);

    return tbSize;
}

uint32_t
NrAmc::GetPayloadSize(uint8_t mcs, uint8_t rank, uint32_t nprb) const
{
    return m_errorModel->GetPayloadSize(NrSpectrumValueHelper::SUBCARRIERS_PER_RB -
                                            GetNumRefScPerRb(),
                                        mcs,
                                        rank,
                                        nprb,
                                        m_emMode);
}

uint8_t
NrAmc::CreateCqiFeedbackWbTdma(const SpectrumValue& sinr, uint8_t& mcs) const
{
    NS_LOG_FUNCTION(this);

    // produces a single CQI/MCS value

    // std::vector<int> cqi;
    uint8_t cqi = 0;
    double seAvg = 0;

    Values::const_iterator it;
    if (m_amcModel == ShannonModel)
    {
        // use shannon model
        double m_ber = GetBer(); // Shannon based model reference BER
        uint32_t rbNum = 0;
        for (it = sinr.ConstValuesBegin(); it != sinr.ConstValuesEnd(); it++)
        {
            double sinr_ = (*it);
            if (sinr_ == 0.0)
            {
                // cqi.push_back (-1); // SINR == 0 (linear units) means no signal in this RB
            }
            else
            {
                /*
                 * Compute the spectral efficiency from the SINR
                 *                                        SINR
                 * spectralEfficiency = log2 (1 + -------------------- )
                 *                                    -ln(5*BER)/1.5
                 * NB: SINR must be expressed in linear units
                 */

                double s = log2(1 + (sinr_ / ((-std::log(5.0 * m_ber)) / 1.5)));
                seAvg += s;

                int cqi_ = GetCqiFromSpectralEfficiency(s);
                rbNum++;

                NS_LOG_LOGIC(" PRB =" << sinr.GetSpectrumModel()->GetNumBands() << ", sinr = "
                                      << sinr_ << " (=" << 10 * std::log10(sinr_) << " dB)"
                                      << ", spectral efficiency =" << s << ", CQI = " << cqi_
                                      << ", BER = " << m_ber);
                // cqi.push_back (cqi_);
            }
        }
        if (rbNum != 0)
        {
            seAvg /= rbNum;
        }
        cqi = GetCqiFromSpectralEfficiency(seAvg); // ceil (cqiAvg);
        mcs = GetMcsFromSpectralEfficiency(seAvg); // ceil(mcsAvg);
    }
    else if (m_amcModel == ErrorModel)
    {
        switch (m_cqiModel)
        {
        case PROBE_CQI:
            cqi = ProbeCqiAlgorithm(sinr, mcs);
            break;
        
        case NEW_BLER_TARGET:
            cqi = NewBlerTargetAlgorithm(sinr, mcs);
            break;

        case EXP_BLER_TARGET:
            cqi = ExpBlerCqiAlgorithm(sinr, mcs);
            break;

        case HYBRID_BLER_TARGET:
            cqi = HybridBlerCqiAlgorithm(sinr, mcs);
            break;

        case LENA_DEFAULT:
        default:
            cqi = OriginalCqiAlgorithm(sinr, mcs);
            break;
        }
    }
    NS_LOG_DEBUG("Voy a devolver este CQI: " << +cqi << " en este tiempo: " << Simulator::Now().GetSeconds());
    return cqi;
}

uint8_t
NrAmc::OriginalCqiAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const
{
    uint8_t cqi = 0;
    Values::const_iterator it;

    std::vector<int> rbMap;
    int rbId = 0;
    for (it = sinr.ConstValuesBegin(); it != sinr.ConstValuesEnd(); it++)
    {
        if (*it != 0.0)
        {
            rbMap.push_back(rbId);
        }
        rbId += 1;
    }

    mcs = 0;
    Ptr<NrErrorModelOutput> output;
    while (mcs <= m_errorModel->GetMaxMcs())
    {
        uint8_t rank = 1; // This function is SISO only
        auto tbSize = CalculateTbSize(mcs, rank, rbMap.size());
        output = m_errorModel->GetTbDecodificationStats(sinr,
                                                        rbMap,
                                                        tbSize,
                                                        mcs,
                                                        NrErrorModel::NrErrorModelHistory());
        if (output->m_tbler > 0.1)
        {
            break;
        }
        mcs++;
    }

    if (mcs > 0)
    {
        mcs--;
    }

    if ((output->m_tbler > 0.1) && (mcs == 0))
    {
        cqi = 0;
    }
    else if (mcs == m_errorModel->GetMaxMcs())
    {
        cqi = 15; // all MCSs can guarantee the 10 % of BER
    }
    else
    {
        double s = m_errorModel->GetSpectralEfficiencyForMcs(mcs);
        cqi = 0;
        while ((cqi < 15) && (m_errorModel->GetSpectralEfficiencyForCqi(cqi + 1) <= s))
        {
            ++cqi;
        }
    }
    NS_LOG_DEBUG(this << "\t MCS " << (uint16_t)mcs << "-> CQI " << cqi);
    return cqi;
}

uint8_t
NrAmc::ProbeCqiAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const
{
    uint8_t cqi = 0;
    Values::const_iterator it;

    std::vector<int> rbMap;
    int rbId = 0;
    for (it = sinr.ConstValuesBegin(); it != sinr.ConstValuesEnd(); it++)
    {
        if (*it != 0.0)
        {
            rbMap.push_back(rbId);
        }
        rbId += 1;
    }
    mcs = 0;
    Ptr<NrErrorModelOutput> output;
    cqi = MyCqi();
    NS_LOG_DEBUG("El m_currentState vale " << m_currentState << " en tiempo: " << Simulator::Now().GetSeconds());
    switch (m_currentState)
    {
    case OUT_STEP:
        NS_LOG_DEBUG("Me meti al OUT_STEP del original en tiempo: " << Simulator::Now().GetSeconds());
        while (mcs <= m_errorModel->GetMaxMcs())
        {
            uint8_t rank = 1; // This function is SISO only
            auto tbSize = CalculateTbSize(mcs, rank, rbMap.size());
            output = m_errorModel->GetTbDecodificationStats(sinr,
                                                            rbMap,
                                                            tbSize,
                                                            mcs,
                                                            NrErrorModel::NrErrorModelHistory());
            if (output->m_tbler > 0.1)
            {
                break;
            }
            mcs++;
        }

        if (mcs > 0)
        {
            mcs--;
        }

        if ((output->m_tbler > 0.1) && (mcs == 0))
        {
            cqi = 0;
        }
        else if (mcs == m_errorModel->GetMaxMcs())
        {
            cqi = 15; // all MCSs can guarantee the 10 % of BER
        }
        else
        {
            double s = m_errorModel->GetSpectralEfficiencyForMcs(mcs);
            cqi = 0;
            while ((cqi < 15) && (m_errorModel->GetSpectralEfficiencyForCqi(cqi + 1) <= s))
            {
                ++cqi;
            }
        }
        NS_LOG_DEBUG(this << "\t MCS " << (uint16_t)mcs << "-> CQI " << +cqi);
        NS_LOG_DEBUG("Actualicé el m_setCqiVal a: " << +m_setCqiVal << " en el tiempo : " << Simulator::Now().GetSeconds());
        m_setCqiVal = cqi;
        
        break;
    
    case IN_STEP:
        NS_LOG_DEBUG("Me meti al IN_STEP del original en tiempo: " << Simulator::Now().GetSeconds());
        uint8_t mcs_max;
        mcs_max = GetMcsFromCqi(cqi);
        mcs = 0;
        
        while (mcs <= m_errorModel->GetMaxMcs())
        {
            uint8_t rank = 1; // This function is SISO only
            auto tbSize = CalculateTbSize(mcs, rank, rbMap.size());
            output = m_errorModel->GetTbDecodificationStats(sinr,
                                                            rbMap,
                                                            tbSize,
                                                            mcs,
                                                            NrErrorModel::NrErrorModelHistory());
            if (mcs == mcs_max)
            {
                break;
            }
            mcs++;
        }
        NS_LOG_DEBUG(this << "\t MCS " << (uint16_t)mcs << "-> CQI " << +cqi);
        break;
    }
    return cqi;
}

uint8_t
NrAmc::NewBlerTargetAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const
{
    uint8_t cqi = 0;
    Values::const_iterator it;

    std::vector<int> rbMap;
    int rbId = 0;
    for (it = sinr.ConstValuesBegin(); it != sinr.ConstValuesEnd(); it++)
    {
        if (*it != 0.0)
        {
            rbMap.push_back(rbId);
        }
        rbId += 1;
    }

    mcs = 0;
    Ptr<NrErrorModelOutput> output;

    while (mcs <= m_errorModel->GetMaxMcs())
    {
        uint8_t rank = 1; // This function is SISO only
        auto tbSize = CalculateTbSize(mcs, rank, rbMap.size());
        output = m_errorModel->GetTbDecodificationStats(sinr,
                                                        rbMap,
                                                        tbSize,
                                                        mcs,
                                                        NrErrorModel::NrErrorModelHistory());
        if (output->m_tbler > m_blerTarget)
        {
            break;
        }
        mcs++;
    }

    if (mcs > 0)
    {
        mcs--;
    }

    if ((output->m_tbler > 0.1) && (mcs == 0))
    {
        cqi = 0;
    }
    else if (mcs == m_errorModel->GetMaxMcs())
    {
        cqi = 15; // all MCSs can guarantee the 10 % of BER
    }
    else
    {
        double s = m_errorModel->GetSpectralEfficiencyForMcs(mcs);
        cqi = 0;
        while ((cqi < 15) && (m_errorModel->GetSpectralEfficiencyForCqi(cqi + 1) <= s))
        {
            ++cqi;
        }
    }
    NS_LOG_DEBUG(this << "\t MCS " << (uint16_t)mcs << "-> CQI " << cqi);
    return cqi;
}

uint8_t
NrAmc::ExpBlerCqiAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const
{
    uint8_t cqi = 0;
    Values::const_iterator it;

    std::vector<int> rbMap;
    int rbId = 0;
    for (it = sinr.ConstValuesBegin(); it != sinr.ConstValuesEnd(); it++)
    {
        if (*it != 0.0)
        {
            rbMap.push_back(rbId);
        }
        rbId += 1;
    }

    mcs = 0;
    Ptr<NrErrorModelOutput> output;

    while (mcs <= m_errorModel->GetMaxMcs())
    {
        uint8_t rank = 1; // This function is SISO only
        auto tbSize = CalculateTbSize(mcs, rank, rbMap.size());
        output = m_errorModel->GetTbDecodificationStats(sinr,
                                                        rbMap,
                                                        tbSize,
                                                        mcs,

                                                        NrErrorModel::NrErrorModelHistory());
        double sinr_eff = Get_SinrEff(sinr, rbMap, mcs, 0, rbMap.size());
        double sinr_eff_db = 10 * log10(sinr_eff);
        double exp_blerTarget = 0.3*exp(-0.08*sinr_eff_db);
        NS_LOG_DEBUG("Para el SINReff " << sinr_eff_db << " [dB], se tiene exp_blerTarget = " << exp_blerTarget);

        if (output->m_tbler > exp_blerTarget)
        {
            break;
        }
        mcs++;
    }

    if (mcs > 0)
    {
        mcs--;
    }

    if ((output->m_tbler > 0.1) && (mcs == 0))
    {
        cqi = 0;
    }
    else if (mcs == m_errorModel->GetMaxMcs())
    {
        cqi = 15; // all MCSs can guarantee the 10 % of BER
    }
    else
    {
        double s = m_errorModel->GetSpectralEfficiencyForMcs(mcs);
        cqi = 0;
        while ((cqi < 15) && (m_errorModel->GetSpectralEfficiencyForCqi(cqi + 1) <= s))
        {
            ++cqi;
        }
    }
    NS_LOG_DEBUG(this << "\t MCS " << (uint16_t)mcs << "-> CQI " << +cqi);
    return cqi;
}

uint8_t
NrAmc::HybridBlerCqiAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const
{
    uint8_t cqi = 0;
    Values::const_iterator it;

    std::vector<int> rbMap;
    int rbId = 0;
    for (it = sinr.ConstValuesBegin(); it != sinr.ConstValuesEnd(); it++)
    {
        if (*it != 0.0)
        {
            rbMap.push_back(rbId);
        }
        rbId += 1;
    }

    mcs = 0;
    Ptr<NrErrorModelOutput> output;

    while (mcs <= m_errorModel->GetMaxMcs())
    {
        uint8_t rank = 1; // This function is SISO only
        auto tbSize = CalculateTbSize(mcs, rank, rbMap.size());
        output = m_errorModel->GetTbDecodificationStats(sinr,
                                                        rbMap,
                                                        tbSize,
                                                        mcs,

                                                        NrErrorModel::NrErrorModelHistory());
        double sinr_eff = Get_SinrEff(sinr, rbMap, mcs, 0, rbMap.size());
        double sinr_eff_db = 10 * log10(sinr_eff);

        double blerTarget;

        // if (sinr_eff_db <= 10)
        // {
        //     blerTarget = 0.3*exp(-0.08*sinr_eff_db);
        //     NS_LOG_DEBUG("Para el SINReff " << sinr_eff_db << " [dB], se tiene exp_blerTarget = " << blerTarget);
        // }
        // else
        // {
        //     blerTarget = m_blerTarget;
        // }
        blerTarget = UpdateBlerTarget(sinr_eff_db);
        

        if (output->m_tbler > blerTarget)
        {
            break;
        }
        mcs++;
    }

    if (mcs > 0)
    {
        mcs--;
    }

    if ((output->m_tbler > 0.1) && (mcs == 0))
    {
        cqi = 0;
    }
    else if (mcs == m_errorModel->GetMaxMcs())
    {
        cqi = 15; // all MCSs can guarantee the 10 % of BER
    }
    else
    {
        double s = m_errorModel->GetSpectralEfficiencyForMcs(mcs);
        cqi = 0;
        while ((cqi < 15) && (m_errorModel->GetSpectralEfficiencyForCqi(cqi + 1) <= s))
        {
            ++cqi;
        }
    }
    NS_LOG_DEBUG(this << "\t MCS " << (uint16_t)mcs << "-> CQI " << +cqi);
    return cqi;
}

void
NrAmc::SetBlerTarget(double blerTarget)
{
    m_blerTarget = blerTarget;
}

void
NrAmc::SetCqiModel(NrAmc::CqiAlgorithm algorithm)
{
    m_cqiModel = algorithm;
    NS_LOG_DEBUG("Cqi Algorithm set: " << +algorithm);
}

bool
NrAmc::Set(const uint8_t cqiGain, Time stepDuration, Time stepFrequency)
{

    NS_ABORT_MSG_IF(m_active, "CQI PROBE INFO WAS ALREADY SET AAAAAAA");

    m_cqiGain = cqiGain;
    m_stepDuration = stepDuration;
    m_stepFrequency = stepFrequency;

    m_lastActivationTime = Seconds(0);
    m_currentState = OUT_STEP;
    m_active = true;

    NS_LOG_DEBUG("Set m_stepDuration " << m_stepDuration << " m_stepFrequency " << m_stepFrequency << " m_cqiGain " << +m_cqiGain);
    return true;
}

uint8_t
NrAmc::MyCqi()
{
    
    if (!m_active)
    {
        return m_setCqiVal;
    }

    uint8_t cqi = m_setCqiVal;
    Time curr_time = Simulator::Now();

    switch (m_currentState)
    {
    case OUT_STEP:

        if ( (curr_time - m_lastActivationTime) > m_stepFrequency )
        {
            m_currentState = IN_STEP;
            m_lastActivationTime = curr_time;

            if ((cqi + m_cqiGain) > 15)
            {
                cqi = 15;
            } 
            else 
            {
                cqi += m_cqiGain;
            }
            m_setCqiVal = cqi;
            NS_LOG_INFO("CQI ascended from " << +(cqi-m_cqiGain) << " to " << +cqi );
        }
        break;

    case IN_STEP:
        cqi = m_setCqiVal;
        if ( (curr_time - m_lastActivationTime) >= m_stepDuration )  // Nos pasamos
        {
            m_currentState = OUT_STEP;
            m_lastActivationTime = curr_time;

            cqi -= m_cqiGain;
            NS_LOG_INFO("CQI descend from " << +(cqi + m_cqiGain) << " to " << +cqi );
        }
        break;
    
    default:
        break;
    }

    return cqi;

}

uint8_t
NrAmc::GetCqiFromSpectralEfficiency(double s) const
{
    NS_LOG_FUNCTION(s);
    NS_ASSERT_MSG(s >= 0.0, "negative spectral efficiency = " << s);
    uint8_t cqi = 0;
    while ((cqi < 15) && (m_errorModel->GetSpectralEfficiencyForCqi(cqi + 1) < s))
    {
        ++cqi;
    }
    NS_LOG_LOGIC("cqi = " << cqi);
    return cqi;
}

uint8_t
NrAmc::GetMcsFromSpectralEfficiency(double s) const
{
    NS_LOG_FUNCTION(s);
    NS_ASSERT_MSG(s >= 0.0, "negative spectral efficiency = " << s);
    uint8_t mcs = 0;
    while ((mcs < m_errorModel->GetMaxMcs()) &&
           (m_errorModel->GetSpectralEfficiencyForMcs(mcs + 1) < s))
    {
        ++mcs;
    }
    NS_LOG_LOGIC("mcs = " << +mcs);
    return mcs;
}

uint32_t
NrAmc::GetMaxMcs() const
{
    NS_LOG_FUNCTION(this);
    return m_errorModel->GetMaxMcs();
}

void
NrAmc::SetAmcModel(NrAmc::AmcModel m)
{
    NS_LOG_FUNCTION(this);
    m_amcModel = m;
}

NrAmc::AmcModel
NrAmc::GetAmcModel() const
{
    NS_LOG_FUNCTION(this);
    return m_amcModel;
}

void
NrAmc::SetErrorModelType(const TypeId& type)
{
    NS_LOG_FUNCTION(this);
    ObjectFactory factory;
    m_errorModelType = type;

    factory.SetTypeId(m_errorModelType);
    m_errorModel = DynamicCast<NrErrorModel>(factory.Create());
    NS_ASSERT(m_errorModel != nullptr);
}

TypeId
NrAmc::GetErrorModelType() const
{
    NS_LOG_FUNCTION(this);
    return m_errorModelType;
}

double
NrAmc::GetBer() const
{
    NS_LOG_FUNCTION(this);
    if (GetErrorModelType() == NrLteMiErrorModel::GetTypeId() ||
        GetErrorModelType() == LenaErrorModel::GetTypeId())
    {
        return 0.00005; // Value for LTE error model
    }
    else
    {
        return 0.00001; // Value for NR error model
    }
}

NrAmc::McsParams
NrAmc::GetMaxMcsParams(const NrSinrMatrix& sinrMat, size_t subbandSize) const
{
    auto mcs = uint8_t{0};
    switch (m_amcModel)
    {
    case ShannonModel:
        NS_ABORT_MSG("ShannonModel is not yet supported");
        break;
    case ErrorModel:
        mcs = GetMaxMcsForErrorModel(sinrMat);
        break;
    default:
        NS_ABORT_MSG("AMC model not supported");
        break;
    }
    auto wbCqi = GetWbCqiFromMcs(mcs);
    auto nRbs = sinrMat.GetNumRbs();
    auto nSbs = (nRbs + subbandSize - 1) / subbandSize;

    // Create subband CQI. Workaround: using WB-CQI as SB-CQI
    // TODO: remove workaround and compute actual SB-CQI
    auto sbCqis = std::vector<uint8_t>(nSbs, wbCqi);

    auto tbSize = CalcTbSizeForMimoMatrix(mcs, sinrMat);
    return McsParams{mcs, wbCqi, sbCqis, tbSize};
}

uint8_t
NrAmc::GetMaxMcsForErrorModel(const NrSinrMatrix& sinrMat) const
{
    auto mcs = uint8_t{0};
    while (mcs <= m_errorModel->GetMaxMcs())
    {
        auto tbler = CalcTblerForMimoMatrix(mcs, sinrMat);
        // TODO: Change target TBLER from default 0.1 when using MCS table 3
        if (tbler > 0.1)
        {
            break;
        }
        // The current configuration produces a sufficiently low TBLER, try next value
        mcs++;
    }
    if (mcs > 0)
    {
        // The loop exited because the MCS exceeded max MCS or because of high TBLER. Reduce MCS
        mcs--;
    }

    return mcs;
}

uint8_t
NrAmc::GetWbCqiFromMcs(uint8_t mcs) const
{
    // Based on OSS CreateCqiFeedbackWbTdma
    auto cqi = uint8_t{0};
    if (mcs == 0)
    {
        cqi = 0;
    }
    else if (mcs == m_errorModel->GetMaxMcs())
    {
        // TODO: define constant for max CQI
        cqi = 15;
    }
    else
    {
        auto s = m_errorModel->GetSpectralEfficiencyForMcs(mcs);
        cqi = 0;
        // TODO: define constant for max CQI
        while ((cqi < 15) && (m_errorModel->GetSpectralEfficiencyForCqi(cqi + 1) <= s))
        {
            ++cqi;
        }
    }
    return cqi;
}

double
NrAmc::CalcTblerForMimoMatrix(uint8_t mcs, const NrSinrMatrix& sinrMat) const
{
    auto dummyRnti = uint16_t{0};
    auto duration = Time{1.0}; // Use an arbitrary non-zero time as the chunk duration
    auto mimoChunk = MimoSinrChunk{sinrMat, dummyRnti, duration};
    auto mimoChunks = std::vector<MimoSinrChunk>{mimoChunk};
    auto rank = sinrMat.GetRank();

    // Create the RB map (indices of used RBs, i.e., indices of RBs where SINR is non-zero)
    std::vector<int> rbMap{}; // TODO: change type of rbMap from int to size_t
    int nRbs = static_cast<int>(sinrMat.GetNumRbs());
    for (int rbIdx = 0; rbIdx < nRbs; rbIdx++)
    {
        if (sinrMat(0, rbIdx) != 0.0)
        {
            rbMap.push_back(rbIdx);
        }
    }

    auto tbSize = CalcTbSizeForMimoMatrix(mcs, sinrMat);
    auto dummyHistory = NrErrorModel::NrErrorModelHistory{}; // Create empty HARQ history
    auto outputOfEm = m_errorModel->GetTbDecodificationStatsMimo(mimoChunks,
                                                                 rbMap,
                                                                 tbSize,
                                                                 mcs,
                                                                 rank,
                                                                 dummyHistory);
    return outputOfEm->m_tbler;
}

uint32_t
NrAmc::CalcTbSizeForMimoMatrix(uint8_t mcs, const NrSinrMatrix& sinrMat) const
{
    auto nRbs = sinrMat.GetNumRbs();
    auto nRbSyms = nRbs * NR_AMC_NUM_SYMBOLS_DEFAULT;
    auto rank = sinrMat.GetRank();
    return CalculateTbSize(mcs, rank, nRbSyms);
}

double
NrAmc::Get_SinrEff(const SpectrumValue& sinr,
                          const std::vector<int>& map,
                          uint8_t mcs,
                          double a,
                          double b) const
{
    NS_LOG_FUNCTION(sinr << &map << (uint8_t)mcs);
    NS_ABORT_MSG_IF(map.size() == 0,
                    " Error: number of allocated RBs cannot be 0 - EESM method - SinrEff function");

    double SINRexp = 0.0;
    double sinrExpSum = 0.0;
    NrEesmT1 table;
    double beta = table.m_betaTable->at(mcs);
    for (uint32_t i = 0; i < map.size(); i++)
    {
        double sinrLin = sinr[map.at(i)];
        SINRexp = exp(-sinrLin / beta);
        sinrExpSum += SINRexp;
    }
    double eff_SINR = -beta * log((a + sinrExpSum) / b);

    NS_LOG_INFO("Effective SINR = " << eff_SINR);

    return eff_SINR;
}

} // namespace ns3
