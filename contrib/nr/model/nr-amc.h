/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
// Modified by A. Gonzalez and D. Torreblanca (2024)
// SPDX-License-Identifier: GPL-2.0-only

#ifndef NR_AMC_H
#define NR_AMC_H

#include "nr-error-model.h"
#include "nr-phy-mac-common.h"
// AGDT: Added for ML integration
#include <ns3/ai-module.h>
#include <fstream>

namespace ns3
{

/**
 * \ingroup error-models
 * \brief Adaptive Modulation and Coding class for the NR module
 *
 * The class has two option to calculate the CQI feedback (which is the MCS to use
 * in the future transmissions): the "ShannonModel" or the "ErrorModel" model,
 * which uses the output of an error model to find the optimal MCS.
 *
 * Please note that it is necessary, even when using the ShannonModel, to correctly
 * configure the ErrorModel type, which must be the same as the one set in the
 * NrSpectrumPhy class.
 *
 * \section nr_amc_conf Configuration
 *
 * The attributes of this class can be configured through the helper methods
 * NrHelper::SetGnbDlAmcAttribute() and NrHelper::SetGnbUlAmcAttribute()
 * for what regards the GNB side (DL or UL). It is important to note that the
 * UE gets a pointer to the GNB AMC to which is connected to.
 *
 * \todo Pass NrAmc parameters through RRC, and don't pass pointers to AMC
 * between GNB and UE
 */
class NrAmc : public Object
{
  public:
    /**
     * \brief GetTypeId
     * \return the TypeId of the Object
     */
    static TypeId GetTypeId();

    /**
     * \brief GetInstanceTypeId
     * \return the instance typeid
     */
    TypeId GetInstanceTypeId() const override;

    /**
     * \brief NrAmc constructor
     */
    NrAmc();

    /**
     * \brief ~NrAmc deconstructor
     */
    ~NrAmc() override;



    // AGDT Type of data to be saved in ns3 ai vectos
    struct dataToSend {
      double simulationTime;
      double sinrEffective;
    };

    struct dataToRecv {
      double blerTarget;
    };

    /// AGDT Update Bler Target
    double UpdateBlerTarget(double sinrEff) const;

    typedef enum {
      IS_GNODEB = 0b1,
      IS_UE = 0b10
    } OR_FLAGS;

    /**
     * \brief Set the object to be in "DL" mode.
     *
     * In this mode, all the requests made to the underlying error model
     * will be done keeping in consideration that the requests refers to
     * DL transmissions.
     */

    void SetDlMode();

    /**
     * \brief Set the object to be in "UL" mode.
     *
     * In this mode, all the requests made to the underlying error model
     * will be done keeping in consideration that the requests refers to
     * UL transmissions.
     */
    void SetUlMode();

    /**
     * \brief Valid types of the model used to create a cqi feedback
     *
     * \see CreateCqiFeedbackWbTdma
     */
    enum AmcModel
    {
        ShannonModel, //!< Shannon based model (very conservative)
        ErrorModel    //!< Error Model version (can use different error models, see NrErrorModel)
    };

    /**
     * \brief Get the MCS value from a CQI value
     * \param cqi the CQI
     * \return the MCS that corresponds to that CQI (it depends on the error model)
     */
    uint8_t GetMcsFromCqi(uint8_t cqi) const;

    /**
     * \return The number of reference subcarriers per resource block
     */
    uint8_t GetNumRefScPerRb() const;

    /**
     * \brief Set the the number of subcarriers carrying reference signals per resource block
     *
     * By default it is fixed at 1. For LTE, it should be 4.
     *
     * \param nref the number of reference subcarriers
     *
     */
    void SetNumRefScPerRb(uint8_t nref);

    /**
     * \brief Create a CQI/MCS wideband feedback from a SINR values
     *
     * For CQI creation, a CSI reference resource equal to all RBs in
     * which the gNB/UE has transmitted power, and from which the SINR can be
     * measured, during 1 OFDM symbol, is assumed.
     *
     * \param sinr the sinr values
     * \param mcsWb The calculated MCS
     * \return The calculated CQI
     */
    uint8_t CreateCqiFeedbackWbTdma(const SpectrumValue& sinr, uint8_t& mcsWb) const;

    /**
     * \brief Get CQI from a SpectralEfficiency value
     * \param s spectral efficiency
     * \return the CQI (depends on the Error Model)
     */
    uint8_t GetCqiFromSpectralEfficiency(double s) const;

    /**
     * \brief Get MCS from a SpectralEfficiency value
     * \param s spectral efficiency
     * \return the MCS (depends on the Error Model)
     */
    uint8_t GetMcsFromSpectralEfficiency(double s) const;

    /**
     * \brief Get the maximum MCS (depends on the underlying error model)
     * \return the maximum MCS
     */
    uint32_t GetMaxMcs() const;

    /**
     * \brief Set the AMC model type
     * \param m the AMC model
     */
    void SetAmcModel(AmcModel m);
    /**
     * \brief Get the AMC model type
     * \return the AMC model type
     */
    AmcModel GetAmcModel() const;

    /**
     * \brief Set Error model type
     * \param type the Error model type
     */
    void SetErrorModelType(const TypeId& type);
    /**
     * \brief Get the error model type
     * \return the error model type
     */
    TypeId GetErrorModelType() const;

    /**
     * \brief Calculate the TransportBlock size (in bytes) giving the MCS and the number of RB
     * assigned
     *
     * It depends on the error model and the "mode" configured with SetMode().
     * Please note that this function expects in input the RB, not the RBG of the transmission.
     *
     * \param mcs the MCS of the transmission
     * \param rank the MIMO rank
     * \param nprb The number of physical resource blocks used in the transmission
     * \return the TBS in bytes
     */
    uint32_t CalculateTbSize(uint8_t mcs, uint8_t rank, uint32_t nprb) const;

    /**
     * \brief Calculate the Payload Size (in bytes) from MCS and the number of RB
     * \param mcs MCS of the transmission
     * \param rank the MIMO rank
     * \param nprb Number of Physical Resource Blocks (not RBG)
     * \return the payload size in bytes
     */
    uint32_t GetPayloadSize(uint8_t mcs, uint8_t rank, uint32_t nprb) const;

    static constexpr size_t NR_AMC_NUM_SYMBOLS_DEFAULT = 12; ///< Num OFDM syms for TB size

    /// \brief Parameters related to MCS selection
    struct McsParams
    {
        uint8_t mcs{};                 ///< MCS value
        uint8_t wbCqi{};               ///< Wideband CQI
        std::vector<uint8_t> sbCqis{}; ///< Subband CQI values
        uint32_t tbSize{};             ///< Expected transport block size
    };

    /// \brief Find maximum MCS supported for this channel and obtain related parameters
    /// \param sinrMat the MIMO SINR matrix (rank * nRbs)
    /// \param subbandSize the size of each subband, used to create subband CQI values
    /// \return a struct with the optimal MCS and corresponding CQI and TB size
    McsParams GetMaxMcsParams(const NrSinrMatrix& sinrMat, size_t subbandSize) const;


    uint8_t ProbeCqiAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const;
    uint8_t OriginalCqiAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const;
    uint8_t ExpBlerCqiAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const;
    uint8_t HybridBlerCqiAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const;
    uint8_t ExpMeanBlerTargetAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const;
    uint8_t QLearnBlerTargetAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const;

    /**
     * This algorithm follows the same procedure as the original algorithm to obtain
     * the bler per sinr-mcs pair. The bler target must be set using an auxiliary function.
    */
    uint8_t NewBlerTargetAlgorithm(const SpectrumValue& sinr, uint8_t& mcs) const;

    enum State {
        IN_STEP,
        OUT_STEP
    };

    enum CqiAlgorithm {
      LENA_DEFAULT,
      PROBE_CQI,
      NEW_BLER_TARGET,
      EXP_BLER_TARGET,
      HYBRID_BLER_TARGET,
      EXP_MEAN_BLER_TARGET,
      Q_LEARN_BLER_TARGET
    };

    /* Functions to change the CQI/BLER/MCS algorithm */
    static void SetCqiModel(NrAmc::CqiAlgorithm algorithm);
    inline static CqiAlgorithm m_cqiModel = LENA_DEFAULT;

    /* Probe Cqi Algorithm auxiliary functions and variables */
    static bool Set(const uint8_t cqiGain, Time stepDuration, Time stepFrequency);
    static uint8_t MyCqi();

    inline static bool m_active = false;
    inline static State m_currentState = OUT_STEP;
    inline static uint8_t m_cqiGain = 0;
    inline static Time m_stepDuration = Seconds(0);
    inline static Time m_stepFrequency = Seconds(0);
    inline static Time m_lastActivationTime = Seconds(0);
    inline static uint8_t m_setCqiVal = 0;

    /* New Bler Target Algorithm auxiliary functions and variables */
    inline static double m_blerTarget = 0.1; // Also used in hybrid bler target
    static void SetBlerTarget(double blerTarget);


    /**
     * \brief compute the effective SINR for the specified MCS and SINR, according
     * to the EESM method.
     * \param sinr the perceived sinrs in the whole bandwidth (vector, per RB)
     * \param map the actives RBs for the TB
     * \param mcs the MCS of the TB
     * \param a the sum term to the exponential SINR
     * \param b the denominator for the exponentials sum
     * \return the effective SINR
     */
    double Get_SinrEff(const SpectrumValue& sinr,
                   const std::vector<int>& map,
                   uint8_t mcs,
                   double a,
                   double b) const;
                   
  private:

    // AGDT
    static uint16_t s_instanceNumber;
    uint16_t m_my_num;
    double m_emptyingTime; // In seconds
    bool m_buffer_full;
    bool m_is_fragmented;

    uint32_t m_maxTxBufferSize; ///< maximum transmit buffer status
    uint32_t m_txBufferSize;    ///< transmit buffer size

    /// \brief Find maximum MCS supported for this channel, using the Shannon model
    /// \param sinrMat the MIMO SINR matrix (rank * nRbs)
    /// \return the maximum MCS
    uint8_t GetMaxMcsForShannonModel(const NrSinrMatrix& sinrMat) const;

    /// \brief Find maximum MCS supported for this channel, using the NR error model
    /// \param sinrMat the MIMO SINR matrix (rank * nRbs)
    /// \return the maximum MCS
    uint8_t GetMaxMcsForErrorModel(const NrSinrMatrix& sinrMat) const;

    /// \brief Compute the CQI value that corresponds to this MCS
    /// \param mcs the MCS
    /// \return the wideband CQI
    uint8_t GetWbCqiFromMcs(uint8_t mcs) const;

    /// \brief Compute the TB size for this channel and MCS
    /// \param mcs the MCS
    /// \param sinrMat the MIMO SINR matrix (rank * nRbs)
    /// \return the TB size
    uint32_t CalcTbSizeForMimoMatrix(uint8_t mcs, const NrSinrMatrix& sinrMat) const;

    /// \brief Compute the TBLER for this channel and MCS, using the NR error model
    /// \param mcs the MCS
    /// \param sinrMat the MIMO SINR matrix (rank * nRbs)
    /// \return the TBLER
    double CalcTblerForMimoMatrix(uint8_t mcs, const NrSinrMatrix& sinrMat) const;

    /**
     * \brief Get the requested BER in assigning MCS (Shannon-bound model)
     * \return BER
     */
    double GetBer() const;

  private:
    AmcModel m_amcModel;                           //!< Type of the CQI feedback model
    Ptr<NrErrorModel> m_errorModel;                //!< Pointer to an instance of ErrorModel
    TypeId m_errorModelType;                       //!< Type of the error model
    uint8_t m_numRefScPerRb{1};                    //!< number of reference subcarriers per RB
    NrErrorModel::Mode m_emMode{NrErrorModel::DL}; //!< Error model mode
    static const unsigned int m_crcLen = 24 / 8;   //!< CRC length (in bytes)
};

} // end namespace ns3

#endif /* NR_AMC_H */
