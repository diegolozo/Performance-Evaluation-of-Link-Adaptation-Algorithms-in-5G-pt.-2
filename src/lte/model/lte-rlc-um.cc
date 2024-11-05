/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "lte-rlc-um.h"

#include "lte-rlc-header.h"
#include "lte-rlc-sdu-status-tag.h"
#include "lte-rlc-tag.h"
#include "ns3/ipv4-header.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/core-module.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteRlcUm");

NS_OBJECT_ENSURE_REGISTERED(LteRlcUm);
// JSA: Added to trace RLC buffer
std::ofstream m_RlcTxBufferStatFile;
std::string m_RlcTxBufferStatFileName;

std::ofstream m_RlcTxSubBufferStatFile;
std::string m_RlcTxSubBufferStatFileName;


uint16_t LteRlcUm::s_instanceNumber = 0;

LteRlcUm::LteRlcUm()
    : m_maxTxBufferSize(10 * 1024),
      m_txBufferSize(0),
      m_sequenceNumber(0),
      m_vrUr(0),
      m_vrUx(0),
      m_vrUh(0),
      m_windowSize(512),
      m_expectedSeqNumber(0),
      m_emptyingTime(0),
      m_customFlags(0),
      m_buffer_full(false),
      m_is_fragmented(false),
      m_useECN(false)
{
    // AG-DT Create buffer state in bufferStats
    if ((m_customFlags & IS_GNODEB) == IS_GNODEB) {
        m_my_num = s_instanceNumber;
        s_instanceNumber++;
        
        if (!m_bufferStatsOf.is_open()) {
            
            std::string file_name = "buffer_";
            file_name += std::to_string(m_my_num);
            file_name += std::string("_aqm_stats.txt");
            m_bufferStatsOf.open(file_name.c_str());
            
            m_bufferStatsOf << "Simulation Time [ns];"
                            << "Fill Percent [%];"
                            << "Delay [ms]"
                            << std::endl;
        
        }
    }
    NS_LOG_FUNCTION(this);
    m_reassemblingState = WAITING_S0_FULL;
}

LteRlcUm::~LteRlcUm()
{
    if (m_customFlags & IS_GNODEB) {
        s_instanceNumber--;
    }
    NS_LOG_FUNCTION(this);
}

TypeId
LteRlcUm::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LteRlcUm")
            .SetParent<LteRlc>()
            .SetGroupName("Lte")
            .AddConstructor<LteRlcUm>()
            .AddAttribute("MaxTxBufferSize",
                          "Maximum Size of the Transmission Buffer (in Bytes)",
                          UintegerValue(10 * 1024),
                          MakeUintegerAccessor(&LteRlcUm::m_maxTxBufferSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("ReorderingTimer",
                          "Value of the t-Reordering timer (See section 7.3 of 3GPP TS 36.322)",
                          TimeValue(MilliSeconds(100)),
                          MakeTimeAccessor(&LteRlcUm::m_reorderingTimerValue),
                          MakeTimeChecker())
            .AddAttribute(
                "EnablePdcpDiscarding",
                "Whether to use the PDCP discarding, i.e., perform discarding at the moment "
                "of passing the PDCP SDU to RLC)",
                BooleanValue(true),
                MakeBooleanAccessor(&LteRlcUm::m_enablePdcpDiscarding),
                MakeBooleanChecker())
            .AddAttribute("DiscardTimerMs",
                          "Discard timer in milliseconds to be used to discard packets. "
                          "If set to 0 then packet delay budget will be used as the discard "
                          "timer value, otherwise it will be used this value.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&LteRlcUm::m_discardTimerMs),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("customFlags", 
                          "Flags for indicating if the RLC corresponds to an UE or BS",
                          UintegerValue(0),
                          MakeUintegerAccessor(&LteRlcUm::SetCustomFlags),
                          MakeUintegerChecker<uint8_t>())
            ;
    return tid;
}

void LteRlcUm::SetCustomFlags(char _cusflags) {
    m_customFlags = _cusflags;
    
    if (_cusflags & IS_GNODEB) {
        m_my_num = s_instanceNumber;
        s_instanceNumber++;

        NS_LOG_DEBUG("TO OPEN");
        if (!m_bufferStatsOf.is_open()) {
            std::string file_name = "buffer_";
            file_name += std::to_string(m_my_num);
            file_name += std::string("_aqm_stats.txt");
            m_bufferStatsOf.open(file_name.c_str());
            m_bufferStatsOf << "Simulation Time [ns];"
                            << "Fill Percent [%];"
                            << "Delay [ms]"
                            << std::endl;
        
        }
    }

    NS_LOG_DEBUG("[LteRlcUm] Set custom flag: " << +m_customFlags);
}

bool LteRlcUm::UpdateBufferStatus(bool entering, bool exiting)
{

    // If the buffer is not from a gNodeB, dont apply an AQM
    if (m_customFlags != IS_GNODEB) {
        NS_LOG_DEBUG("FLAG OF THE NOT GNODEB BUFFER" << +m_customFlags << " Its number is: " << m_my_num);
        return false;
    }

    NS_LOG_DEBUG("NUM OF THE GNODEB BUFFER" << +m_my_num);

    // AG-DT
    uint32_t headOfLineDelayInMs;
    if (!m_txBuffer.empty())
    {
        headOfLineDelayInMs =
                (Simulator::Now() - m_txBuffer.begin()->m_waitingSince).GetMilliSeconds();
    } else {
        headOfLineDelayInMs = 0;
    }

    // Data in tx queue + estimated headers size
    double fillPerc = ((double)(m_txBufferSize + 2 * m_txBuffer.size()) / (double)m_maxTxBufferSize);
    
    NS_LOG_DEBUG("MAXBUFFERSIZE" << m_maxTxBufferSize);
    NS_LOG_DEBUG("fillPerc: " << fillPerc << " Time (ms): " << +headOfLineDelayInMs);
    
    m_bufferStatsOf << Simulator::Now().GetNanoSeconds() << ";"
                    << fillPerc << ";"
                    << +headOfLineDelayInMs
                    << std::endl;
    
    BooleanValue useAI;
    BooleanValue useECN;
    
    if (GlobalValue::GetValueByNameFailSafe("useAI", useAI)) {
        if (!useAI) {
            return false;
        }
    } else {
        NS_ABORT_MSG("useAI GlobalValue not found!");
    }

    if (GlobalValue::GetValueByNameFailSafe("useECN", useECN)) {
        m_useECN = useECN;
    } else {
        NS_ABORT_MSG("useECN GlobalValue not found!");
    }

    auto interface = Ns3AiMsgInterface::Get();
    Ns3AiMsgInterfaceImpl<LteRlcUm::dataToSend, LteRlcUm::dataToRecv>* msgInterface =
        interface->GetInterface<LteRlcUm::dataToSend, LteRlcUm::dataToRecv>();

    msgInterface->CppSendBegin();
    msgInterface->GetCpp2PyVector()->at(m_my_num).bufferDelay = headOfLineDelayInMs;
    msgInterface->GetCpp2PyVector()->at(m_my_num).bufferFillPerc = fillPerc;
    msgInterface->GetCpp2PyVector()->at(m_my_num).justCalled = true;
    msgInterface->GetCpp2PyVector()->at(m_my_num).entering = entering;
    msgInterface->GetCpp2PyVector()->at(m_my_num).exiting = exiting;
    msgInterface->GetCpp2PyVector()->at(m_my_num).simulationTime = ns3::Simulator::Now().GetSeconds();
    msgInterface->GetCpp2PyVector()->at(m_my_num).emptyingTime = m_emptyingTime;
    msgInterface->GetCpp2PyVector()->at(m_my_num).isBufferFull = m_buffer_full;
    msgInterface->GetCpp2PyVector()->at(m_my_num).isFragmented = m_is_fragmented;

    msgInterface->CppSendEnd();

    msgInterface->CppRecvBegin();
    bool decision = msgInterface->GetPy2CppVector()->at(m_my_num).dropPacket;
    msgInterface->CppRecvEnd();

    const char* _decision_char = decision ? "drop" : "save";

    NS_LOG_DEBUG("[ Buffer " << this << "] Decided to " << _decision_char << " a packet." << std::endl);

    return decision;
}

void
LteRlcUm::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_reorderingTimer.Cancel();
    m_rbsTimer.Cancel();

    LteRlc::DoDispose();
}

/**
 * RLC SAP
 */

void
LteRlcUm::DoTransmitPdcpPdu(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << m_rnti << (uint32_t)m_lcid << p->GetSize());

    // JSA: Added to trace RLC buffer
    bool letsDrop = false;
    bool DEBUG = false;

    Ptr<Packet> packet = p->Copy();
    LtePdcpHeader removeHeader;
    packet->RemoveHeader(removeHeader);

    Ipv4Header ipHeader;
    packet->PeekHeader(ipHeader);
    Ipv4Address srcAddr = ipHeader.GetSource();
    Ipv4Address dstAddr = ipHeader.GetDestination();
    
    
    int64_t TxBufferNum = m_txBuffer.size();
    int64_t TxBufferSize = m_txBufferSize;
    int64_t ThisPacketSize = p->GetSize();
    int64_t DropPacketSize = 0;
    if (m_txBufferSize + p->GetSize() <= m_maxTxBufferSize)
    {
        if (m_enablePdcpDiscarding)
        {
            // discart the packet
            uint32_t headOfLineDelayInMs = 0;
            uint32_t discardTimerMs =
                (m_discardTimerMs > 0) ? m_discardTimerMs : m_packetDelayBudgetMs;

            if (!m_txBuffer.empty())
            {
                headOfLineDelayInMs =
                    (Simulator::Now() - m_txBuffer.begin()->m_waitingSince).GetMilliSeconds();
            }
            NS_LOG_DEBUG("head of line delay in MS:" << headOfLineDelayInMs);
            if (headOfLineDelayInMs > discardTimerMs)
            {
                NS_LOG_INFO("Tx HOL is higher than this packet can allow. RLC SDU discarded");
                NS_LOG_DEBUG("headOfLineDelayInMs    = " << headOfLineDelayInMs);
                NS_LOG_DEBUG("m_packetDelayBudgetMs    = " << m_packetDelayBudgetMs);
                NS_LOG_DEBUG("packet size     = " << p->GetSize());
                m_txDropTrace(p);
            }
        }
        
        // AG-DT Enqueue check (RED, ARED)
        m_buffer_full = false; 
        letsDrop = UpdateBufferStatus(true);
        if (letsDrop && !m_useECN){

            if (DEBUG) { std::cout << "BUFFER: " << "\t" << srcAddr << "\t" << dstAddr << std::endl; }
            // Discard full RLC SDU
            NS_LOG_LOGIC("TxBuffer is full. RLC SDU discarded");
            NS_LOG_LOGIC("MaxTxBufferSize = " << m_maxTxBufferSize);
            NS_LOG_LOGIC("txBufferSize    = " << m_txBufferSize);
            NS_LOG_LOGIC("packet size     = " << p->GetSize());
            m_txDropTrace(p);

            // JSA: Trace RLC Buffer Drop case
            DropPacketSize = p->GetSize();

        }else
        {
            // AGDT - JSA
            // If we uyse ECN and the AQM marked the packet, update the headers to indicate congestion
            if (m_useECN && letsDrop) {
                
                LtePdcpHeader lteHeader;
                p->RemoveHeader(lteHeader);

                Ipv4Header _ipHeader;
                p->RemoveHeader(_ipHeader);
                _ipHeader.SetEcn(Ipv4Header::EcnType::ECN_CE);

                p->AddHeader(_ipHeader);
                p->AddHeader(lteHeader);

            }

            /** Store PDCP PDU */
            LteRlcSduStatusTag tag;
            tag.SetStatus(LteRlcSduStatusTag::FULL_SDU);
            p->AddPacketTag(tag);

            NS_LOG_INFO("Adding RLC SDU to Tx Buffer after adding LteRlcSduStatusTag: FULL_SDU");
            m_txBuffer.emplace_back(p, Simulator::Now());
            m_txBufferSize += p->GetSize();
            NS_LOG_LOGIC("NumOfBuffers = " << m_txBuffer.size());
            NS_LOG_LOGIC("txBufferSize = " << m_txBufferSize);

            // JSA: Trace RLC Buffer add
            DropPacketSize = 0;

        }
    
    }
    else
    {
        if (DEBUG) { std::cout << "BUFFER: " << "\t" << srcAddr << "\t" << dstAddr << std::endl; }
        if (DEBUG) {std::cout << "Drop! " << ipHeader.GetIdentification() << std::endl;}

        // Discard full RLC SDU
        m_buffer_full = true; 
        UpdateBufferStatus(true);
        NS_LOG_INFO("Tx Buffer is full. RLC SDU discarded");
        NS_LOG_LOGIC("MaxTxBufferSize = " << m_maxTxBufferSize);
        NS_LOG_LOGIC("txBufferSize    = " << m_txBufferSize);
        NS_LOG_LOGIC("packet size     = " << p->GetSize());
        m_txDropTrace(p);
        // JSA: Trace RLC Buffer Drop case
        DropPacketSize = p->GetSize();
    }
    // Create  File
    if (!m_RlcTxBufferStatFile.is_open())
    {
        std::ostringstream oss;
        oss <<"RlcBufferStat.txt";
        m_RlcTxBufferStatFileName = oss.str();
        m_RlcTxBufferStatFile.open(m_RlcTxBufferStatFileName.c_str());
        m_RlcTxBufferStatFile << "Time"
                              << "\t"
                              << "srcIP" // source packets in buffer
                              << "\t"
                              << "dstIP"
                              << "\t"
                              << "NumOfBuffers" // packets in buffer
                              << "\t"
                              << "txBufferSize" // bytes in buffer
                              << "\t"
                              << "PacketSize" // bytes current packet
                              << "\t"
                              << "dropSize" // bytes droped
                              << "\t"
                              << "this"
                              << "\t"
                              << "rnti"
                              << "\t"
                              << "lcid"
                              << std::endl;
    }
    
    // Write to File
    m_RlcTxBufferStatFile << Simulator::Now().GetSeconds()
                        << "\t"
                        << srcAddr 
                        << "\t"
                        << dstAddr 
                        << "\t"
                        << TxBufferNum
                        << "\t"
                        << TxBufferSize
                        << "\t"
                        << ThisPacketSize
                        << "\t"
                        << DropPacketSize 
                        << "\t"
                        << this
                        << "\t"
                        << m_rnti
                        << "\t"
                        << (uint32_t) m_lcid 
                        << std::endl;


    // JSA Update Buffer status after each arrived packet
    UpdateBufferStatus();

    /** Report Buffer Status */
    DoReportBufferStatus();
    m_rbsTimer.Cancel();
}

/**
 * MAC SAP
 */

void
LteRlcUm::DoNotifyTxOpportunity(LteMacSapUser::TxOpportunityParameters txOpParams)
{
    NS_LOG_FUNCTION(this << m_rnti << (uint32_t)m_lcid << txOpParams.bytes);
    NS_LOG_INFO("RLC layer is preparing data for the following Tx opportunity of "
                << txOpParams.bytes << " bytes for RNTI=" << m_rnti << ", LCID=" << (uint32_t)m_lcid
                << ", CCID=" << (uint32_t)txOpParams.componentCarrierId << ", HARQ ID="
                << (uint32_t)txOpParams.harqId << ", MIMO Layer=" << (uint32_t)txOpParams.layer);

    if (txOpParams.bytes <= 2)
    {
        // Stingy MAC: Header fix part is 2 bytes, we need more bytes for the data
        NS_LOG_INFO("TX opportunity too small - Only " << txOpParams.bytes << " bytes");
        return;
    }
    

    Ptr<Packet> packet = Create<Packet>();
    LteRlcHeader rlcHeader;

    // Build Data field
    uint32_t nextSegmentSize = txOpParams.bytes - 2;
    uint32_t nextSegmentId = 1;
    uint32_t dataFieldAddedSize = 0;
    std::vector<Ptr<Packet>> dataField;

    // Remove the first packet from the transmission buffer.
    // If only a segment of the packet is taken, then the remaining is given back later
    if (m_txBuffer.empty())
    {
        NS_LOG_LOGIC("No data pending");
        return;
    }

    LteRlcSduStatusTag tag_AGDT;
    m_txBuffer.begin()->m_pdu->PeekPacketTag(tag_AGDT);

    if (tag_AGDT.GetStatus() == LteRlcSduStatusTag::FULL_SDU)
    {
        m_is_fragmented = false;
        bool letsDrop = UpdateBufferStatus(false, true);

        if (letsDrop)
        {

            if (m_useECN) {
                
                Ptr<Packet> packet = m_txBuffer.begin()->m_pdu;

                LtePdcpHeader lteHeader;
                packet->RemoveHeader(lteHeader);

                Ipv4Header _ipHeader;
                packet->RemoveHeader(_ipHeader);
                _ipHeader.SetEcn(Ipv4Header::EcnType::ECN_CE);

                packet->AddHeader(_ipHeader);
                packet->AddHeader(lteHeader);

            } else {

                NS_LOG_INFO("CoDel dropped a packet");

                m_txBufferSize -= m_txBuffer.begin()->m_pdu->GetSize();
                m_txBuffer.erase(m_txBuffer.begin());

                if (m_txBuffer.empty()) {
                    m_emptyingTime = Simulator::Now().GetSeconds();
                }

                return;
            }
        }
    }
    else{
        m_is_fragmented = true;
        UpdateBufferStatus(false, true);
    }

    Ptr<Packet> firstSegment = m_txBuffer.begin()->m_pdu->Copy();
    Time firstSegmentTime = m_txBuffer.begin()->m_waitingSince;

    NS_LOG_LOGIC("SDUs in TxBuffer  = " << m_txBuffer.size());
    NS_LOG_LOGIC("First SDU buffer  = " << firstSegment);
    NS_LOG_LOGIC("First SDU size    = " << firstSegment->GetSize());
    NS_LOG_LOGIC("Next segment size = " << nextSegmentSize);
    NS_LOG_LOGIC("Remove SDU from TxBuffer");
    m_txBufferSize -= firstSegment->GetSize();
    NS_LOG_LOGIC("txBufferSize      = " << m_txBufferSize);
    m_txBuffer.erase(m_txBuffer.begin());

    // bool letsDrop = UpdateBufferStatus(false, true);

    // if (letsDrop)
    // {
    //     // AG-DT: CoDel drop
    //     NS_LOG_INFO("CoDel dropped a packet");
    //     return;
    // }

    while (firstSegment && (firstSegment->GetSize() > 0) && (nextSegmentSize > 0))
    {
        NS_LOG_LOGIC("WHILE ( firstSegment && firstSegment->GetSize > 0 && nextSegmentSize > 0 )");
        NS_LOG_LOGIC("    firstSegment size = " << firstSegment->GetSize());
        NS_LOG_LOGIC("    nextSegmentSize   = " << nextSegmentSize);
        if ((firstSegment->GetSize() > nextSegmentSize) ||
            // Segment larger than 2047 octets can only be mapped to the end of the Data field
            (firstSegment->GetSize() > 2047))
        {
            // Take the minimum size, due to the 2047-bytes 3GPP exception
            // This exception is due to the length of the LI field (just 11 bits)
            uint32_t currSegmentSize = std::min(firstSegment->GetSize(), nextSegmentSize);

            NS_LOG_LOGIC("    IF ( firstSegment > nextSegmentSize ||");
            NS_LOG_LOGIC("         firstSegment > 2047 )");

            // Segment txBuffer.FirstBuffer and
            // Give back the remaining segment to the transmission buffer
            Ptr<Packet> newSegment = firstSegment->CreateFragment(0, currSegmentSize);
            NS_LOG_LOGIC("    newSegment size   = " << newSegment->GetSize());

            // Status tag of the new and remaining segments
            // Note: This is the only place where a PDU is segmented and
            // therefore its status can change
            LteRlcSduStatusTag oldTag;
            LteRlcSduStatusTag newTag;
            firstSegment->RemovePacketTag(oldTag);
            newSegment->RemovePacketTag(newTag);
            if (oldTag.GetStatus() == LteRlcSduStatusTag::FULL_SDU)
            {
                newTag.SetStatus(LteRlcSduStatusTag::FIRST_SEGMENT);
                oldTag.SetStatus(LteRlcSduStatusTag::LAST_SEGMENT);
            }
            else if (oldTag.GetStatus() == LteRlcSduStatusTag::LAST_SEGMENT)
            {
                newTag.SetStatus(LteRlcSduStatusTag::MIDDLE_SEGMENT);
                // oldTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
            }

            // Give back the remaining segment to the transmission buffer
            firstSegment->RemoveAtStart(currSegmentSize);
            NS_LOG_LOGIC(
                "    firstSegment size (after RemoveAtStart) = " << firstSegment->GetSize());
            if (firstSegment->GetSize() > 0)
            {
                firstSegment->AddPacketTag(oldTag);

                m_txBuffer.insert(m_txBuffer.begin(), TxPdu(firstSegment, firstSegmentTime));
                m_txBufferSize += m_txBuffer.begin()->m_pdu->GetSize();

                NS_LOG_LOGIC("    TX buffer: Give back the remaining segment");
                NS_LOG_LOGIC("    TX buffers = " << m_txBuffer.size());
                NS_LOG_LOGIC("    Front buffer size = " << m_txBuffer.begin()->m_pdu->GetSize());
                NS_LOG_LOGIC("    txBufferSize = " << m_txBufferSize);
            }
            else
            {
                // Whole segment was taken, so adjust tag
                if (newTag.GetStatus() == LteRlcSduStatusTag::FIRST_SEGMENT)
                {
                    newTag.SetStatus(LteRlcSduStatusTag::FULL_SDU);
                }
                else if (newTag.GetStatus() == LteRlcSduStatusTag::MIDDLE_SEGMENT)
                {
                    newTag.SetStatus(LteRlcSduStatusTag::LAST_SEGMENT);
                }
            }
            // Segment is completely taken or
            // the remaining segment is given back to the transmission buffer
            firstSegment = nullptr;

            // Put status tag once it has been adjusted
            newSegment->AddPacketTag(newTag);

            // Add Segment to Data field
            dataFieldAddedSize = newSegment->GetSize();
            dataField.push_back(newSegment);
            newSegment = nullptr;

            // ExtensionBit (Next_Segment - 1) = 0
            rlcHeader.PushExtensionBit(LteRlcHeader::DATA_FIELD_FOLLOWS);

            // no LengthIndicator for the last one

            nextSegmentSize -= dataFieldAddedSize;
            nextSegmentId++;

            // nextSegmentSize MUST be zero (only if segment is smaller or equal to 2047)

            // (NO more segments) → exit
            // break;
        }
        else if ((nextSegmentSize - firstSegment->GetSize() <= 2) || m_txBuffer.empty())
        {
            NS_LOG_LOGIC(
                "    IF nextSegmentSize - firstSegment->GetSize () <= 2 || txBuffer.size == 0");
            // Add txBuffer.FirstBuffer to DataField
            dataFieldAddedSize = firstSegment->GetSize();
            dataField.push_back(firstSegment);
            firstSegment = nullptr;

            // ExtensionBit (Next_Segment - 1) = 0
            rlcHeader.PushExtensionBit(LteRlcHeader::DATA_FIELD_FOLLOWS);

            // no LengthIndicator for the last one

            nextSegmentSize -= dataFieldAddedSize;
            nextSegmentId++;

            NS_LOG_LOGIC("        SDUs in TxBuffer  = " << m_txBuffer.size());
            if (!m_txBuffer.empty())
            {
                NS_LOG_LOGIC("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
                NS_LOG_LOGIC(
                    "        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize());
            }
            NS_LOG_LOGIC("        Next segment size = " << nextSegmentSize);

            // nextSegmentSize <= 2 (only if txBuffer is not empty)

            // (NO more segments) → exit
            // break;
        }
        else // (firstSegment->GetSize () < m_nextSegmentSize) && (m_txBuffer.size () > 0)
        {
            NS_LOG_LOGIC("    IF firstSegment < NextSegmentSize && txBuffer.size > 0");
            // Add txBuffer.FirstBuffer to DataField
            dataFieldAddedSize = firstSegment->GetSize();
            dataField.push_back(firstSegment);

            // ExtensionBit (Next_Segment - 1) = 1
            rlcHeader.PushExtensionBit(LteRlcHeader::E_LI_FIELDS_FOLLOWS);

            // LengthIndicator (Next_Segment)  = txBuffer.FirstBuffer.length()
            rlcHeader.PushLengthIndicator(firstSegment->GetSize());

            nextSegmentSize -= ((nextSegmentId % 2) ? (2) : (1)) + dataFieldAddedSize;
            nextSegmentId++;

            NS_LOG_LOGIC("        SDUs in TxBuffer  = " << m_txBuffer.size());
            if (!m_txBuffer.empty())
            {
                NS_LOG_LOGIC("        First SDU buffer  = " << m_txBuffer.begin()->m_pdu);
                NS_LOG_LOGIC(
                    "        First SDU size    = " << m_txBuffer.begin()->m_pdu->GetSize());
            }
            NS_LOG_LOGIC("        Next segment size = " << nextSegmentSize);
            NS_LOG_LOGIC("        Remove SDU from TxBuffer");

            // (more segments)
            firstSegment = m_txBuffer.begin()->m_pdu->Copy();
            firstSegmentTime = m_txBuffer.begin()->m_waitingSince;
            m_txBufferSize -= firstSegment->GetSize();
            m_txBuffer.erase(m_txBuffer.begin());
            NS_LOG_LOGIC("        txBufferSize = " << m_txBufferSize);
        }
    }

    // Build RLC header
    rlcHeader.SetSequenceNumber(m_sequenceNumber++);

    // Build RLC PDU with DataField and Header
    auto it = dataField.begin();

    uint8_t framingInfo = 0;

    // FIRST SEGMENT
    LteRlcSduStatusTag tag;
    NS_ASSERT_MSG((*it)->PeekPacketTag(tag), "LteRlcSduStatusTag is missing");
    (*it)->PeekPacketTag(tag);
    if ((tag.GetStatus() == LteRlcSduStatusTag::FULL_SDU) ||
        (tag.GetStatus() == LteRlcSduStatusTag::FIRST_SEGMENT))
    {
        framingInfo |= LteRlcHeader::FIRST_BYTE;
    }
    else
    {
        framingInfo |= LteRlcHeader::NO_FIRST_BYTE;
    }

    while (it < dataField.end())
    {
        NS_LOG_LOGIC("Adding SDU/segment to packet, length = " << (*it)->GetSize());

        NS_ASSERT_MSG((*it)->PeekPacketTag(tag), "LteRlcSduStatusTag is missing");
        (*it)->RemovePacketTag(tag);
        if (packet->GetSize() > 0)
        {
            packet->AddAtEnd(*it);
        }
        else
        {
            packet = (*it);
        }
        it++;
    }

    // LAST SEGMENT (Note: There could be only one and be the first one)
    it--;
    if ((tag.GetStatus() == LteRlcSduStatusTag::FULL_SDU) ||
        (tag.GetStatus() == LteRlcSduStatusTag::LAST_SEGMENT))
    {
        framingInfo |= LteRlcHeader::LAST_BYTE;
    }
    else
    {
        framingInfo |= LteRlcHeader::NO_LAST_BYTE;
    }

    rlcHeader.SetFramingInfo(framingInfo);

    NS_LOG_LOGIC("RLC header: " << rlcHeader);
    packet->AddHeader(rlcHeader);

    // Sender timestamp
    RlcTag rlcTag(Simulator::Now());
    packet->AddByteTag(rlcTag, 1, rlcHeader.GetSerializedSize());
    m_txPdu(m_rnti, m_lcid, packet->GetSize());

    // Send RLC PDU to MAC layer
    LteMacSapProvider::TransmitPduParameters params;
    params.pdu = packet;
    params.rnti = m_rnti;
    params.lcid = m_lcid;
    params.layer = txOpParams.layer;
    params.harqProcessId = txOpParams.harqId;
    params.componentCarrierId = txOpParams.componentCarrierId;


    NS_LOG_INFO("Forward RLC PDU to MAC Layer");
    m_macSapProvider->TransmitPdu(params);

    if (!m_txBuffer.empty())
    {
        m_rbsTimer.Cancel();
        m_rbsTimer = Simulator::Schedule(MilliSeconds(10), &LteRlcUm::ExpireRbsTimer, this);

    } else { // The buffer is empty
        m_emptyingTime = Simulator::Now().GetSeconds();
    }
}

void
LteRlcUm::DoNotifyHarqDeliveryFailure()
{
    NS_LOG_FUNCTION(this);
}

void
LteRlcUm::DoReceivePdu(LteMacSapUser::ReceivePduParameters rxPduParams)
{
    NS_LOG_FUNCTION(this << m_rnti << (uint32_t)m_lcid << rxPduParams.p->GetSize());

    // Receiver timestamp
    RlcTag rlcTag;
    Time delay;

    bool ret = rxPduParams.p->FindFirstMatchingByteTag(rlcTag);
    NS_ASSERT_MSG(ret, "RlcTag is missing");

    delay = Simulator::Now() - rlcTag.GetSenderTimestamp();
    m_rxPdu(m_rnti, m_lcid, rxPduParams.p->GetSize(), delay.GetNanoSeconds());

    // 5.1.2.2 Receive operations

    // Get RLC header parameters
    LteRlcHeader rlcHeader;
    rxPduParams.p->PeekHeader(rlcHeader);
    NS_LOG_LOGIC("RLC header: " << rlcHeader);
    SequenceNumber10 seqNumber = rlcHeader.GetSequenceNumber();

    // 5.1.2.2.1 General
    // The receiving UM RLC entity shall maintain a reordering window according to state variable
    // VR(UH) as follows:
    // - a SN falls within the reordering window if (VR(UH) - UM_Window_Size) <= SN < VR(UH);
    // - a SN falls outside of the reordering window otherwise.
    // When receiving an UMD PDU from lower layer, the receiving UM RLC entity shall:
    // - either discard the received UMD PDU or place it in the reception buffer (see sub
    // clause 5.1.2.2.2);
    // - if the received UMD PDU was placed in the reception buffer:
    // - update state variables, reassemble and deliver RLC SDUs to upper layer and start/stop
    // t-Reordering as needed (see sub clause 5.1.2.2.3); When t-Reordering expires, the receiving
    // UM RLC entity shall:
    // - update state variables, reassemble and deliver RLC SDUs to upper layer and start
    // t-Reordering as needed (see sub clause 5.1.2.2.4).

    // 5.1.2.2.2 Actions when an UMD PDU is received from lower layer
    // When an UMD PDU with SN = x is received from lower layer, the receiving UM RLC entity shall:
    // - if VR(UR) < x < VR(UH) and the UMD PDU with SN = x has been received before; or
    // - if (VR(UH) - UM_Window_Size) <= x < VR(UR):
    //    - discard the received UMD PDU;
    // - else:
    //    - place the received UMD PDU in the reception buffer.

    NS_LOG_LOGIC("VR(UR) = " << m_vrUr);
    NS_LOG_LOGIC("VR(UX) = " << m_vrUx);
    NS_LOG_LOGIC("VR(UH) = " << m_vrUh);
    NS_LOG_LOGIC("SN = " << seqNumber);

    m_vrUr.SetModulusBase(m_vrUh - m_windowSize);
    m_vrUh.SetModulusBase(m_vrUh - m_windowSize);
    seqNumber.SetModulusBase(m_vrUh - m_windowSize);

    if (((m_vrUr < seqNumber) && (seqNumber < m_vrUh) &&
         (m_rxBuffer.count(seqNumber.GetValue()) > 0)) ||
        (((m_vrUh - m_windowSize) <= seqNumber) && (seqNumber < m_vrUr)))
    {
        NS_LOG_LOGIC("PDU discarded");
        rxPduParams.p = nullptr;
        return;
    }
    else
    {
        NS_LOG_LOGIC("Place PDU in the reception buffer");
        m_rxBuffer[seqNumber.GetValue()] = rxPduParams.p;
    }

    // 5.1.2.2.3 Actions when an UMD PDU is placed in the reception buffer
    // When an UMD PDU with SN = x is placed in the reception buffer, the receiving UM RLC entity
    // shall:

    // - if x falls outside of the reordering window:
    //    - update VR(UH) to x + 1;
    //    - reassemble RLC SDUs from any UMD PDUs with SN that falls outside of the reordering
    //    window, remove
    //      RLC headers when doing so and deliver the reassembled RLC SDUs to upper layer in
    //      ascending order of the RLC SN if not delivered before;
    //    - if VR(UR) falls outside of the reordering window:
    //        - set VR(UR) to (VR(UH) - UM_Window_Size);

    if (!IsInsideReorderingWindow(seqNumber))
    {
        NS_LOG_LOGIC("SN is outside the reordering window");

        m_vrUh = seqNumber + 1;
        NS_LOG_LOGIC("New VR(UH) = " << m_vrUh);

        ReassembleOutsideWindow();

        if (!IsInsideReorderingWindow(m_vrUr))
        {
            m_vrUr = m_vrUh - m_windowSize;
            NS_LOG_LOGIC("VR(UR) is outside the reordering window");
            NS_LOG_LOGIC("New VR(UR) = " << m_vrUr);
        }
    }

    // - if the reception buffer contains an UMD PDU with SN = VR(UR):
    //    - update VR(UR) to the SN of the first UMD PDU with SN > current VR(UR) that has not been
    //    received;
    //    - reassemble RLC SDUs from any UMD PDUs with SN < updated VR(UR), remove RLC headers when
    //    doing
    //      so and deliver the reassembled RLC SDUs to upper layer in ascending order of the RLC SN
    //      if not delivered before;

    if (m_rxBuffer.count(m_vrUr.GetValue()) > 0)
    {
        NS_LOG_LOGIC("Reception buffer contains SN = " << m_vrUr);

        uint16_t newVrUr;
        SequenceNumber10 oldVrUr = m_vrUr;

        auto it = m_rxBuffer.find(m_vrUr.GetValue());
        newVrUr = (it->first) + 1;
        while (m_rxBuffer.count(newVrUr) > 0)
        {
            newVrUr++;
        }
        m_vrUr = newVrUr;
        NS_LOG_LOGIC("New VR(UR) = " << m_vrUr);

        ReassembleSnInterval(oldVrUr, m_vrUr);
    }

    // m_vrUh can change previously, set new modulus base
    // for the t-Reordering timer-related comparisons
    m_vrUr.SetModulusBase(m_vrUh - m_windowSize);
    m_vrUx.SetModulusBase(m_vrUh - m_windowSize);
    m_vrUh.SetModulusBase(m_vrUh - m_windowSize);

    // - if t-Reordering is running:
    //    - if VR(UX) <= VR(UR); or
    //    - if VR(UX) falls outside of the reordering window and VR(UX) is not equal to VR(UH)::
    //        - stop and reset t-Reordering;
    if (m_reorderingTimer.IsRunning())
    {
        NS_LOG_LOGIC("Reordering timer is running");

        if ((m_vrUx <= m_vrUr) || ((!IsInsideReorderingWindow(m_vrUx)) && (m_vrUx != m_vrUh)))
        {
            NS_LOG_LOGIC("Stop reordering timer");
            m_reorderingTimer.Cancel();
        }
    }

    // - if t-Reordering is not running (includes the case when t-Reordering is stopped due to
    // actions above):
    //    - if VR(UH) > VR(UR):
    //        - start t-Reordering;
    //        - set VR(UX) to VR(UH).
    if (!m_reorderingTimer.IsRunning())
    {
        NS_LOG_LOGIC("Reordering timer is not running");

        if (m_vrUh > m_vrUr)
        {
            NS_LOG_LOGIC("VR(UH) > VR(UR)");
            NS_LOG_LOGIC("Start reordering timer");
            m_reorderingTimer =
                Simulator::Schedule(m_reorderingTimerValue, &LteRlcUm::ExpireReorderingTimer, this);
            m_vrUx = m_vrUh;
            NS_LOG_LOGIC("New VR(UX) = " << m_vrUx);
        }
    }
}

bool
LteRlcUm::IsInsideReorderingWindow(SequenceNumber10 seqNumber)
{
    NS_LOG_FUNCTION(this << seqNumber);
    NS_LOG_LOGIC("Reordering Window: " << m_vrUh << " - " << m_windowSize << " <= " << seqNumber
                                       << " < " << m_vrUh);

    m_vrUh.SetModulusBase(m_vrUh - m_windowSize);
    seqNumber.SetModulusBase(m_vrUh - m_windowSize);

    if (((m_vrUh - m_windowSize) <= seqNumber) && (seqNumber < m_vrUh))
    {
        NS_LOG_LOGIC(seqNumber << " is INSIDE the reordering window");
        return true;
    }
    else
    {
        NS_LOG_LOGIC(seqNumber << " is OUTSIDE the reordering window");
        return false;
    }
}

void
LteRlcUm::ReassembleAndDeliver(Ptr<Packet> packet)
{
    LteRlcHeader rlcHeader;
    packet->RemoveHeader(rlcHeader);
    uint8_t framingInfo = rlcHeader.GetFramingInfo();
    SequenceNumber10 currSeqNumber = rlcHeader.GetSequenceNumber();
    bool expectedSnLost;

    if (currSeqNumber != m_expectedSeqNumber)
    {
        expectedSnLost = true;
        NS_LOG_LOGIC("There are losses. Expected SN = " << m_expectedSeqNumber
                                                        << ". Current SN = " << currSeqNumber);
        m_expectedSeqNumber = currSeqNumber + 1;
    }
    else
    {
        expectedSnLost = false;
        NS_LOG_LOGIC("No losses. Expected SN = " << m_expectedSeqNumber
                                                 << ". Current SN = " << currSeqNumber);
        m_expectedSeqNumber++;
    }

    // Build list of SDUs
    uint8_t extensionBit;
    uint16_t lengthIndicator;
    do
    {
        extensionBit = rlcHeader.PopExtensionBit();
        NS_LOG_LOGIC("E = " << (uint16_t)extensionBit);

        if (extensionBit == 0)
        {
            m_sdusBuffer.push_back(packet);
        }
        else // extensionBit == 1
        {
            lengthIndicator = rlcHeader.PopLengthIndicator();
            NS_LOG_LOGIC("LI = " << lengthIndicator);

            // Check if there is enough data in the packet
            if (lengthIndicator >= packet->GetSize())
            {
                NS_LOG_LOGIC("INTERNAL ERROR: Not enough data in the packet ("
                             << packet->GetSize() << "). Needed LI=" << lengthIndicator);
            }

            // Split packet in two fragments
            Ptr<Packet> data_field = packet->CreateFragment(0, lengthIndicator);
            packet->RemoveAtStart(lengthIndicator);

            m_sdusBuffer.push_back(data_field);
        }
    } while (extensionBit == 1);

    // Current reassembling state
    if (m_reassemblingState == WAITING_S0_FULL)
    {
        NS_LOG_LOGIC("Reassembling State = 'WAITING_S0_FULL'");
    }
    else if (m_reassemblingState == WAITING_SI_SF)
    {
        NS_LOG_LOGIC("Reassembling State = 'WAITING_SI_SF'");
    }
    else
    {
        NS_LOG_LOGIC("Reassembling State = Unknown state");
    }

    // Received framing Info
    NS_LOG_LOGIC("Framing Info = " << (uint16_t)framingInfo);

    // Reassemble the list of SDUs (when there is no losses)
    if (!expectedSnLost)
    {
        switch (m_reassemblingState)
        {
        case WAITING_S0_FULL:
            switch (framingInfo)
            {
            case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                m_reassemblingState = WAITING_S0_FULL;

                /**
                 * Deliver one or multiple PDUs
                 */
                for (auto it = m_sdusBuffer.begin(); it != m_sdusBuffer.end(); it++)
                {
                    m_rlcSapUser->ReceivePdcpPdu(*it);
                }
                m_sdusBuffer.clear();
                break;

            case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                m_reassemblingState = WAITING_SI_SF;

                /**
                 * Deliver full PDUs
                 */
                while (m_sdusBuffer.size() > 1)
                {
                    m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                }

                /**
                 * Keep S0
                 */
                m_keepS0 = m_sdusBuffer.front();
                m_sdusBuffer.pop_front();
                break;

            case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                m_reassemblingState = WAITING_S0_FULL;

                /**
                 * Discard SI or SN
                 */
                m_sdusBuffer.pop_front();

                /**
                 * Deliver zero, one or multiple PDUs
                 */
                while (!m_sdusBuffer.empty())
                {
                    m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                }
                break;

            case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                if (m_sdusBuffer.size() == 1)
                {
                    m_reassemblingState = WAITING_S0_FULL;
                }
                else
                {
                    m_reassemblingState = WAITING_SI_SF;
                }

                /**
                 * Discard SI or SN
                 */
                m_sdusBuffer.pop_front();

                if (!m_sdusBuffer.empty())
                {
                    /**
                     * Deliver zero, one or multiple PDUs
                     */
                    while (m_sdusBuffer.size() > 1)
                    {
                        m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                        m_sdusBuffer.pop_front();
                    }

                    /**
                     * Keep S0
                     */
                    m_keepS0 = m_sdusBuffer.front();
                    m_sdusBuffer.pop_front();
                }
                break;

            default:
                /**
                 * ERROR: Transition not possible
                 */
                NS_LOG_LOGIC(
                    "INTERNAL ERROR: Transition not possible. FI = " << (uint32_t)framingInfo);
                break;
            }
            break;

        case WAITING_SI_SF:
            switch (framingInfo)
            {
            case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                m_reassemblingState = WAITING_S0_FULL;

                /**
                 * Deliver (Kept)S0 + SN
                 */
                m_keepS0->AddAtEnd(m_sdusBuffer.front());
                m_sdusBuffer.pop_front();
                m_rlcSapUser->ReceivePdcpPdu(m_keepS0);

                /**
                 * Deliver zero, one or multiple PDUs
                 */
                while (!m_sdusBuffer.empty())
                {
                    m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                }
                break;

            case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                m_reassemblingState = WAITING_SI_SF;

                /**
                 * Keep SI
                 */
                if (m_sdusBuffer.size() == 1)
                {
                    m_keepS0->AddAtEnd(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                }
                else // m_sdusBuffer.size () > 1
                {
                    /**
                     * Deliver (Kept)S0 + SN
                     */
                    m_keepS0->AddAtEnd(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                    m_rlcSapUser->ReceivePdcpPdu(m_keepS0);

                    /**
                     * Deliver zero, one or multiple PDUs
                     */
                    while (m_sdusBuffer.size() > 1)
                    {
                        m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                        m_sdusBuffer.pop_front();
                    }

                    /**
                     * Keep S0
                     */
                    m_keepS0 = m_sdusBuffer.front();
                    m_sdusBuffer.pop_front();
                }
                break;

            case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::LAST_BYTE):
            case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
            default:
                /**
                 * ERROR: Transition not possible
                 */
                NS_LOG_LOGIC(
                    "INTERNAL ERROR: Transition not possible. FI = " << (uint32_t)framingInfo);
                break;
            }
            break;

        default:
            NS_LOG_LOGIC(
                "INTERNAL ERROR: Wrong reassembling state = " << (uint32_t)m_reassemblingState);
            break;
        }
    }
    else // Reassemble the list of SDUs (when there are losses, i.e. the received SN is not the
         // expected one)
    {
        switch (m_reassemblingState)
        {
        case WAITING_S0_FULL:
            switch (framingInfo)
            {
            case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                m_reassemblingState = WAITING_S0_FULL;

                /**
                 * Deliver one or multiple PDUs
                 */
                for (auto it = m_sdusBuffer.begin(); it != m_sdusBuffer.end(); it++)
                {
                    m_rlcSapUser->ReceivePdcpPdu(*it);
                }
                m_sdusBuffer.clear();
                break;

            case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                m_reassemblingState = WAITING_SI_SF;

                /**
                 * Deliver full PDUs
                 */
                while (m_sdusBuffer.size() > 1)
                {
                    m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                }

                /**
                 * Keep S0
                 */
                m_keepS0 = m_sdusBuffer.front();
                m_sdusBuffer.pop_front();
                break;

            case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                m_reassemblingState = WAITING_S0_FULL;

                /**
                 * Discard SN
                 */
                m_sdusBuffer.pop_front();

                /**
                 * Deliver zero, one or multiple PDUs
                 */
                while (!m_sdusBuffer.empty())
                {
                    m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                }
                break;

            case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                if (m_sdusBuffer.size() == 1)
                {
                    m_reassemblingState = WAITING_S0_FULL;
                }
                else
                {
                    m_reassemblingState = WAITING_SI_SF;
                }

                /**
                 * Discard SI or SN
                 */
                m_sdusBuffer.pop_front();

                if (!m_sdusBuffer.empty())
                {
                    /**
                     * Deliver zero, one or multiple PDUs
                     */
                    while (m_sdusBuffer.size() > 1)
                    {
                        m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                        m_sdusBuffer.pop_front();
                    }

                    /**
                     * Keep S0
                     */
                    m_keepS0 = m_sdusBuffer.front();
                    m_sdusBuffer.pop_front();
                }
                break;

            default:
                /**
                 * ERROR: Transition not possible
                 */
                NS_LOG_LOGIC(
                    "INTERNAL ERROR: Transition not possible. FI = " << (uint32_t)framingInfo);
                break;
            }
            break;

        case WAITING_SI_SF:
            switch (framingInfo)
            {
            case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                m_reassemblingState = WAITING_S0_FULL;

                /**
                 * Discard S0
                 */
                m_keepS0 = nullptr;

                /**
                 * Deliver one or multiple PDUs
                 */
                while (!m_sdusBuffer.empty())
                {
                    m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                }
                break;

            case (LteRlcHeader::FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                m_reassemblingState = WAITING_SI_SF;

                /**
                 * Discard S0
                 */
                m_keepS0 = nullptr;

                /**
                 * Deliver zero, one or multiple PDUs
                 */
                while (m_sdusBuffer.size() > 1)
                {
                    m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                }

                /**
                 * Keep S0
                 */
                m_keepS0 = m_sdusBuffer.front();
                m_sdusBuffer.pop_front();

                break;

            case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::LAST_BYTE):
                m_reassemblingState = WAITING_S0_FULL;

                /**
                 * Discard S0
                 */
                m_keepS0 = nullptr;

                /**
                 * Discard SI or SN
                 */
                m_sdusBuffer.pop_front();

                /**
                 * Deliver zero, one or multiple PDUs
                 */
                while (!m_sdusBuffer.empty())
                {
                    m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                    m_sdusBuffer.pop_front();
                }
                break;

            case (LteRlcHeader::NO_FIRST_BYTE | LteRlcHeader::NO_LAST_BYTE):
                if (m_sdusBuffer.size() == 1)
                {
                    m_reassemblingState = WAITING_S0_FULL;
                }
                else
                {
                    m_reassemblingState = WAITING_SI_SF;
                }

                /**
                 * Discard S0
                 */
                m_keepS0 = nullptr;

                /**
                 * Discard SI or SN
                 */
                m_sdusBuffer.pop_front();

                if (!m_sdusBuffer.empty())
                {
                    /**
                     * Deliver zero, one or multiple PDUs
                     */
                    while (m_sdusBuffer.size() > 1)
                    {
                        m_rlcSapUser->ReceivePdcpPdu(m_sdusBuffer.front());
                        m_sdusBuffer.pop_front();
                    }

                    /**
                     * Keep S0
                     */
                    m_keepS0 = m_sdusBuffer.front();
                    m_sdusBuffer.pop_front();
                }
                break;

            default:
                /**
                 * ERROR: Transition not possible
                 */
                NS_LOG_LOGIC(
                    "INTERNAL ERROR: Transition not possible. FI = " << (uint32_t)framingInfo);
                break;
            }
            break;

        default:
            NS_LOG_LOGIC(
                "INTERNAL ERROR: Wrong reassembling state = " << (uint32_t)m_reassemblingState);
            break;
        }
    }
}

void
LteRlcUm::ReassembleOutsideWindow()
{
    NS_LOG_LOGIC("Reassemble Outside Window");

    auto it = m_rxBuffer.begin();

    while ((it != m_rxBuffer.end()) && !IsInsideReorderingWindow(SequenceNumber10(it->first)))
    {
        NS_LOG_LOGIC("SN = " << it->first);

        // Reassemble RLC SDUs and deliver the PDCP PDU to upper layer
        ReassembleAndDeliver(it->second);

        auto it_tmp = it;
        ++it;
        m_rxBuffer.erase(it_tmp);
    }

    if (it != m_rxBuffer.end())
    {
        NS_LOG_LOGIC("(SN = " << it->first << ") is inside the reordering window");
    }
}

void
LteRlcUm::ReassembleSnInterval(SequenceNumber10 lowSeqNumber, SequenceNumber10 highSeqNumber)
{
    NS_LOG_LOGIC("Reassemble SN between " << lowSeqNumber << " and " << highSeqNumber);

    SequenceNumber10 reassembleSn = lowSeqNumber;
    NS_LOG_LOGIC("reassembleSN = " << reassembleSn);
    NS_LOG_LOGIC("highSeqNumber = " << highSeqNumber);
    while (reassembleSn < highSeqNumber)
    {
        NS_LOG_LOGIC("reassembleSn < highSeqNumber");
        auto it = m_rxBuffer.find(reassembleSn.GetValue());
        NS_LOG_LOGIC("it->first  = " << it->first);
        NS_LOG_LOGIC("it->second = " << it->second);
        if (it != m_rxBuffer.end())
        {
            NS_LOG_LOGIC("SN = " << it->first);

            // Reassemble RLC SDUs and deliver the PDCP PDU to upper layer
            ReassembleAndDeliver(it->second);

            m_rxBuffer.erase(it);
        }

        reassembleSn++;
    }
}

void
LteRlcUm::DoReportBufferStatus()
{
    Time holDelay(0);
    uint32_t queueSize = 0;

    if (!m_txBuffer.empty())
    {
        holDelay = Simulator::Now() - m_txBuffer.front().m_waitingSince;

        queueSize =
            m_txBufferSize + 2 * m_txBuffer.size(); // Data in tx queue + estimated headers size
    }

    LteMacSapProvider::ReportBufferStatusParameters r;
    r.rnti = m_rnti;
    r.lcid = m_lcid;
    r.txQueueSize = queueSize;
    r.txQueueHolDelay = holDelay.GetMilliSeconds();
    r.retxQueueSize = 0;
    r.retxQueueHolDelay = 0;
    r.statusPduSize = 0;

    NS_LOG_LOGIC("Send ReportBufferStatus = " << r.txQueueSize << ", " << r.txQueueHolDelay);
    m_macSapProvider->ReportBufferStatus(r);
}

void
LteRlcUm::ExpireReorderingTimer()
{
    NS_LOG_FUNCTION(this << m_rnti << (uint32_t)m_lcid);
    NS_LOG_LOGIC("Reordering timer has expired");

    // 5.1.2.2.4 Actions when t-Reordering expires
    // When t-Reordering expires, the receiving UM RLC entity shall:
    // - update VR(UR) to the SN of the first UMD PDU with SN >= VR(UX) that has not been received;
    // - reassemble RLC SDUs from any UMD PDUs with SN < updated VR(UR), remove RLC headers when
    // doing so
    //   and deliver the reassembled RLC SDUs to upper layer in ascending order of the RLC SN if not
    //   delivered before;
    // - if VR(UH) > VR(UR):
    //    - start t-Reordering;
    //    - set VR(UX) to VR(UH).

    SequenceNumber10 newVrUr = m_vrUx;

    while (m_rxBuffer.find(newVrUr.GetValue()) != m_rxBuffer.end())
    {
        newVrUr++;
    }
    SequenceNumber10 oldVrUr = m_vrUr;
    m_vrUr = newVrUr;
    NS_LOG_LOGIC("New VR(UR) = " << m_vrUr);

    ReassembleSnInterval(oldVrUr, m_vrUr);

    if (m_vrUh > m_vrUr)
    {
        NS_LOG_LOGIC("Start reordering timer");
        m_reorderingTimer =
            Simulator::Schedule(m_reorderingTimerValue, &LteRlcUm::ExpireReorderingTimer, this);
        m_vrUx = m_vrUh;
        NS_LOG_LOGIC("New VR(UX) = " << m_vrUx);
    }
}

void
LteRlcUm::ExpireRbsTimer()
{
    NS_LOG_LOGIC("RBS Timer expires");

    if (!m_txBuffer.empty())
    {
        DoReportBufferStatus();
        m_rbsTimer = Simulator::Schedule(MilliSeconds(10), &LteRlcUm::ExpireRbsTimer, this);
    }
}

} // namespace ns3
