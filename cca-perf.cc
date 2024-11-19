/* Include ns3 libraries */
#include "ns3/applications-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/quic-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-mac-scheduler-tdma-rr.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/point-to-point-helper.h"
#include <ns3/antenna-module.h>
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/buildings-propagation-loss-model.h>
#include <ns3/hybrid-buildings-propagation-loss-model.h>
#include "ns3/ns3-ai-msg-interface.h"
#include "ns3/lte-rlc-um.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/enum.h"
#include "ns3/error-model.h"

/* Include systems libraries */
#include <sys/types.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <map>

/* Include custom libraries (aux files for the simulation) */
#include "cmdline-colors.h"
#include "cca-perf-clientapp.h"
#include "cca-perf-serverapp.h"

#include "physical-scenarios.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("cca-perf");

/* Global Variables */

std::map<ns3::FlowId, uint64_t> rxBytesOldMap;

auto itime = std::chrono::high_resolution_clock::now(); // Initial time
auto tic = std::chrono::high_resolution_clock::now();   // Initial time per cycle

const std::string LOG_FILENAME = "output.log";

static double SegmentSize = 1448.0; // 1448
double simTime = 60;           // in second
double iniTime = 0.02;           // in second
double endTime = simTime - iniTime;     // in second
bool randstart = true;           // To enable/disable rand start
bool circlepath = true;           // To enable/disable circle path


bool verbose = true;            // verbose mode

/* Noise vars */
// Mean chosen by https://www.qualcomm.com/content/dam/qcomm-martech/dm-assets/documents/5g_nr_millimeter_wave_network_coverage_simulation_studies_for_global_cities.pdf page 4
const double NOISE_MEAN = 5;    // Default value is 5
const double NOISE_VAR = 1;     // Noise variance
const double NOISE_BOUND = 3;   // Noise bound, read NormalDistribution for info about the parameter.
const Time NOISE_T_RES = MilliSeconds(15); // Time to schedule the add noise function


/* Trace functions, definitions are at EOF */
static void CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval);
static void RtoTracer(Ptr<OutputStreamWrapper> stream, Time oldval, Time newval);
static void RttTracer(Ptr<OutputStreamWrapper> stream, Time oldval, Time newval);
static void NextTxTracer(Ptr<OutputStreamWrapper> stream, SequenceNumber32 old [[maybe_unused]], SequenceNumber32 nextTx);
static void NextRxTracer(Ptr<OutputStreamWrapper> stream, SequenceNumber32 old [[maybe_unused]], SequenceNumber32 nextRx);
static void InFlightTracer(Ptr<OutputStreamWrapper> stream, uint32_t old [[maybe_unused]], uint32_t inFlight);
static void SsThreshTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval);
static void TraceTcp(uint32_t nodeId, uint32_t socketId);
static void TraceQuic(uint32_t nodeId, uint32_t socketId);


/* Helper functions, definitions are at EOF */
static void InstallTCP (Ptr<Node> remoteHost, Ptr<Node> sender, uint16_t serverPort, float startTime, float stopTime, float dataRate, std::string _bitrateType);
static void InstallUDP (Ptr<Node> remoteHost, Ptr<Node> sender, uint16_t sinkPort, float startTime, float stopTime, float dataRate, std::string _bitrateType);
static void InstallQUIC (Ptr<Node> remoteHost, Ptr<Node> sender, uint16_t sinkPort, float startTime, float stopTime, float dataRate, std::string _bitrateType);
static void CalculatePosition(NodeContainer* ueNodes, NodeContainer* gnbNodes, std::ostream* os);
static void ChangeVelocity(NodeContainer* ueNodes, NodeContainer* gnbNodes);
static void AddRandomNoise(Ptr<NrPhy> ue_phy);
static void PrintNodeAddressInfo(bool ignore_localh);
static void processFlowMonitor(Ptr<FlowMonitor> monitor, Ptr<ns3::FlowClassifier> flowmonHelper, double AppStartTime);
static void ShowStatus();

static void ProcessFlowMonitorCallback();

static Ptr<FlowMonitor> globalMonitor;
static Ptr<ns3::FlowClassifier> globalFlowClassifier;
static double globalAppStartTime;

static void ProcessFlowMonitorCallback()
{
    processFlowMonitor(globalMonitor, globalFlowClassifier, globalAppStartTime);
}



// connect to a number of traces
// static void
// CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
// {
//   *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
// }

// static void
// RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
// {
//   *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
// }

static void
Rx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> p, const QuicHeader& q, Ptr<const QuicSocketBase> qsb)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << p->GetSize() << std::endl;
}

static void
Traces(uint32_t serverId, std::string pathVersion, std::string finalPart)
{
    if (verbose){
        std::cout << TXT_CYAN << "\nTrace QUIC: " << serverId << " at: "<<  
                1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(tic-itime).count()<< TXT_CLEAR << std::endl;
    }
    AsciiTraceHelper asciiTraceHelper;

    std::ostringstream pathCW;
    pathCW << "/NodeList/" << serverId << "/$ns3::QuicL4Protocol/SocketList/0/QuicSocketBase/CongestionWindow";
    NS_LOG_INFO("Matches cw " << Config::LookupMatches(pathCW.str().c_str()).GetN());

    std::ostringstream fileCW;
    fileCW << pathVersion << "QUIC-cwnd-change"  << serverId << "" << finalPart;

    std::ostringstream pathRTT;
    pathRTT << "/NodeList/" << serverId << "/$ns3::QuicL4Protocol/SocketList/0/QuicSocketBase/RTT";

    std::ostringstream fileRTT;
    fileRTT << pathVersion << "QUIC-rtt"  << serverId << "" << finalPart;

    std::ostringstream pathRCWnd;
    pathRCWnd<< "/NodeList/" << serverId << "/$ns3::QuicL4Protocol/SocketList/0/QuicSocketBase/RWND";

    std::ostringstream fileRCWnd;
    fileRCWnd<<pathVersion << "QUIC-rwnd-change"  << serverId << "" << finalPart;

    std::ostringstream fileName;
    fileName << pathVersion << "QUIC-rx-data" << serverId << "" << finalPart;
    std::ostringstream pathRx;
    // pathRx << "/NodeList/" << serverId << "/$ns3::QuicL4Protocol/SocketList/*/QuicSocketBase/Rx";
    // NS_LOG_INFO("Matches rx " << Config::LookupMatches(pathRx.str().c_str()).GetN());

    // Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
    // Config::ConnectWithoutContext (pathRx.str ().c_str (), MakeBoundCallback (&Rx, stream));

    // Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileCW.str ().c_str ());
    // Config::ConnectWithoutContext (pathCW.str ().c_str (), MakeBoundCallback(&CwndChange, stream1));

    // Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileRTT.str ().c_str ());
    // Config::ConnectWithoutContext (pathRTT.str ().c_str (), MakeBoundCallback(&RttChange, stream2));

    // Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream (fileRCWnd.str ().c_str ());
    // Config::ConnectWithoutContextFailSafe (pathRCWnd.str ().c_str (), MakeBoundCallback(&CwndChange, stream4));
    if (verbose){
        std::cout << TXT_CYAN << "\nTrace QUIC: " << serverId << " at: "<<  
                1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(tic-itime).count()<< TXT_CLEAR << std::endl;
    }
}


int main(int argc, char* argv[]) {

    double frequency = 27.3e9;      // central frequency 28e9
    double bandwidth = 400e6;       // bandwidth Hz
    double mobility = true;         // whether to enable mobility default: false
    bool logging = true;            // whether to enable logging from the simulation, another option is by
                                    // exporting the NS_LOG environment variable
    bool shadowing = true;          // to enable shadowing effect
    bool addNoise = true;           // To enable/disable AWGN

    std::string AQMEnq = "None";       // AQM 'None', 'RED', 'ARED''
    std::string AQMDeq = "None";       // AQM 'None', 'CoDel'

    int rng = 20;                    // initial seed value 

    // double hBS;                     // base station antenna height in meters
    // double hUE;                     // user antenna height in meters
    double txPower = 40;            // txPower [dBm] 40 dBm=10W
    uint16_t numerology = 3;        // 120 kHz and 125 microseg
    std::string scenario = "UMa";   // scenario
    enum BandwidthPartInfo::Scenario scenarioEnum = BandwidthPartInfo::UMa;

    double dataRate1 = 60;         //Data rate Mbps
    double dataRate2 = 60;         //Data rate Mbps
    double dataRate3 = 60;         //Data rate Mbps
    double serverDelay = 0.01;      // remote 0.040 ; edge 0.004
    double rlcBufferPerc = 100;     // x*DBP
    double rlcBuffer = 999999999; // Bytes BDP=250Mbps*100ms default: 999999999

    // CQI Probe own variables.
    uint8_t cqiHighGain = 2;         // Step of CQI probe
    Time ProbeCqiDuration = MilliSeconds(20);  // miliseconds
    Time stepFrequency = MilliSeconds(500); // miliseconds
    double blerTarget = 0.1;
    int amcAlgorithm = (int)NrAmc::CqiAlgorithm::HYBRID_BLER_TARGET;
    int phyDistro = (int)PhysicalDistributionOptions::TREES;

    // Trace activation
    bool NRTrace = true;    // whether to enable Trace NR
    bool TCPTrace = true;   // whether to enable Trace TCP

    // RB Info and position
    uint16_t gNbNum = 1;    // Numbers of RB
    double gNbX = 50.0;     // X position
    double gNbY = 50.0;     // Y position
    uint16_t gNbD = 80;     // Distance between gNb

    // UE Info and position
    uint16_t ueN1  = 1; // Numbers of User per RB
    uint16_t ueN2 = 0; // Numbers of User per RB
    uint16_t ueN3 = 0; // Numbers of User per RB
    uint16_t ueNumPergNb = ueN1 +ueN2+ ueN3; // Numbers of User per RB
    
    Ipv4Address ip_net_UE = "7.0.0.0";
    Ipv4Mask ip_mask_UE = "255.255.0.0";
    Ipv4Address ip_net_Servers[3] = {"172.16.1.0", "172.16.2.0", "172.16.3.0"};
    // Ipv4Address ip_net_Server1 = "172.16.1.0";
    // Ipv4Address ip_net_Server2 = "172.16.2.0";
    // Ipv4Address ip_net_Server3 = "172.16.3.0";
    Ipv4Mask ip_mask_Server = "255.255.255.0";


    // BUILDING Position
    bool enableBuildings = true; // 
    uint32_t gridWidth = 3 ;//
    uint32_t numOfBuildings = 2;
    // uint32_t apartmentsX = 1;
    // uint32_t nFloors = 10;

    double buildX = 37.0; // Initial Position
    double buildY = 30.0; // 30
    double buildDx = 10; // Distance between building
    double buildDy = 10; //
    double buildLx = 8; //4
    double buildLy = 10;
    double CustomLoss = 0; // Extra Custom Loss
    double CustomLossLos = 0; // Extra Custom Loss LoS
    double CustomLossNlos = 0; // Extra Custom Loss NLoS

    std::string serverType = "Remote"; // Transport Protocol
    std::string bitrateType1 = "CBR"; // Bitrate
    std::string bitrateType2 = "CBR"; // Bitrate
    std::string bitrateType3 = "CBR"; // Bitrate
    std::string tcpTypeId1 = "TcpBbr";
    std::string tcpTypeId2 = "TcpBbr";
    std::string tcpTypeId3 = "UDP";
    std::string flowType1 = "TCP";
    std::string flowType2 = "TCP";
    std::string flowType3 = "UDP";
    double speed1 = 1;
    double speed2 = 0;
    double speed3 = 1;
    double xUE1 = 20;
    double xUE2 = 40;
    double xUE3 = 80;

    std::string aiSegmentName = "My Seg";
    bool useAI = false;
    static GlobalValue g_useAI = GlobalValue("useAI", "Represents if AI (more genereally AQMs) are used", BooleanValue(false), MakeBooleanChecker());
    static GlobalValue g_useECN = GlobalValue("useECN", "Represents if ECN should be used instead of dropping packets", BooleanValue(false), MakeBooleanChecker());
    bool useECN = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("frequency", "The central carrier frequency in Hz.", frequency);
    cmd.AddValue("mobility",
                 "If set to 1 UEs will be mobile, when set to 0 UE will be static. By default, "
                 "they are mobile.",
                 mobility);
    cmd.AddValue("logging", "If set to 0, log components will be disabled.", logging);
    cmd.AddValue("simTime", "Simulation Time (s)", simTime);
    cmd.AddValue("iniTime", "Initial Time (s)", iniTime);
    cmd.AddValue("endTime", "End Time (s)", endTime);
    cmd.AddValue("randstart", "Randoms start 0 or 1 (off or on)", randstart);
    cmd.AddValue("bandwidth", "bandwidth in Hz.", bandwidth);
    cmd.AddValue("dataRate1", "server1 dataRate.", dataRate1);
    cmd.AddValue("dataRate2", "server2 dataRate.", dataRate2);
    cmd.AddValue("dataRate3", "server3 dataRate.", dataRate3);
    cmd.AddValue("serverType", "Type of Server: Remote or Edge", serverType);
    cmd.AddValue("bitrateType1", "Type of bitrate: WBT, VBR1, VBR2 or CBR", bitrateType1);
    cmd.AddValue("bitrateType2", "Type of bitrate: WBT, VBR1, VBR2 or CBR", bitrateType2);
    cmd.AddValue("bitrateType3", "Type of bitrate: WBT, VBR1, VBR2 or CBR", bitrateType3);
    cmd.AddValue("serverDelay", "Server Delay: ms", serverDelay);
    cmd.AddValue("rlcBufferPerc", "Percent RLC Buffer", rlcBufferPerc);
    cmd.AddValue("tcpTypeId1", "TCP flavor: TcpBbr , TcpNewReno, TcpCubic, TcpVegas, TcpIllinois, TcpYeah, TcpHighSpeed, TcpBic", tcpTypeId1);
    cmd.AddValue("tcpTypeId2", "TCP flavor: TcpBbr , TcpNewReno, TcpCubic, TcpVegas, TcpIllinois, TcpYeah, TcpHighSpeed, TcpBic", tcpTypeId2);
    cmd.AddValue("tcpTypeId3", "TCP flavor: TcpBbr , TcpNewReno, TcpCubic, TcpVegas, TcpIllinois, TcpYeah, TcpHighSpeed, TcpBic", tcpTypeId3);
    cmd.AddValue("enableBuildings", "If set to 1, enable Buildings", enableBuildings);
    cmd.AddValue("shadowing", "If set to 1, enable Shadowing", shadowing);
    cmd.AddValue("xUE1", "Position UEs Group 1", xUE1);
    cmd.AddValue("xUE2", "Position UEs Group 2", xUE2);
    cmd.AddValue("xUE3", "Position UEs Group 3", xUE3);
    cmd.AddValue("speed1", "Speed UEs Group 1", speed1);
    cmd.AddValue("speed2", "Speed UEs Group 2", speed2);
    cmd.AddValue("speed3", "Speed UEs Group 3", speed3);
    cmd.AddValue("ueN1", "Number of UE Group 1", ueN1);
    cmd.AddValue("ueN2", "Number of UE Group 2", ueN2);
    cmd.AddValue("ueN3", "Number of UE Group 3", ueN3);
    cmd.AddValue("AQMEnq", "AQM: None, RED, ARED in RLC buffer", AQMEnq);
    cmd.AddValue("AQMDeq", "AQM: None, CoDel in RLC buffer", AQMDeq);
    cmd.AddValue("phyDistro", "Physical distribution of the Buildings-UEs-gNbs. Options:\n\t0:Default\n\t1:Trees\n\t2:Indoor Router\n\t3:Neighborhood\nCurrent value: ", phyDistro);   
    cmd.AddValue("verbose", "Verbose 0 or 1 (off or on)", verbose);
    cmd.AddValue("useAI", "useAI for buffer drop decision", useAI);
    cmd.AddValue("aiSegmentName", "For identifying useAI shared memory, DO NOT SET THIS, it will be automatically set by cca-perf.py", aiSegmentName);
    cmd.AddValue("rng", "Set initial seed value", rng);
    cmd.AddValue("circlepath", "Circle Path  arround the gNb 0 or 1 (off or on)", circlepath);
    cmd.AddValue("CustomLoss", "Extra Custom Loss. 0 by default", CustomLoss);
    cmd.AddValue("CustomLossLos", "Extra Custom Loss LoS. 0 by default", CustomLossLos);
    cmd.AddValue("CustomLossNlos", "Extra Custom Loss NLoS. 0 by default", CustomLossNlos);
    cmd.AddValue("useECN", "Use ECN instead of dropping packets", useECN);
    cmd.AddValue("cqiHighGain", "Steps of CQI Probe. Means CQI=round(CQI*cqiHighGain)", cqiHighGain);
    cmd.AddValue("ProbeCqiDuration", "Duration of the Probe CQI override in s.", ProbeCqiDuration);
    cmd.AddValue("stepFrequency", "Time between activations of Probe CQI in s", stepFrequency);
    cmd.AddValue("addNoise", "Add normal distributed noise to the simulation", addNoise);
    cmd.AddValue("blerTarget", "Set the bler target for the AMC (Default: 0.1)", blerTarget);
    cmd.AddValue("amcAlgo", "Choose the algorithm to be used in the amc possible values:\n\t0:Original\n\t1:ProbeCqi\n\t2:NewBlerTarget\n\t3:ExpBlerTarget\n\t4:HybridBlerTarget\n\t5:PythonBlerTarget\nCurrent value: ", amcAlgorithm);
    cmd.Parse(argc, argv);

    if (!g_useECN.SetValue(BooleanValue(useECN))) {
            NS_ABORT_MSG("Unable to set useECN value to " << useECN);
    }

    if (verbose){ 
        std::cout << TXT_CYAN << "Start Over 3.41" << TXT_CLEAR << std::endl;
    }
    ueNumPergNb = ueN1 +ueN2 + ueN3;

    uint16_t maxGroup = 3;
    uint16_t ueNs[maxGroup];// Array with Numbers of User per Group
    ueNs[0] = ueN1;
    ueNs[1] = ueN2;
    ueNs[2] = ueN3;

    double xUEs[maxGroup];// Array with initial position of User per Group
    xUEs[0] = xUE1;
    xUEs[1] = xUE2;
    xUEs[2] = xUE3;

    double speedXs[maxGroup];
    speedXs[0] = speed1;
    speedXs[1] = speed2;
    speedXs[2] = speed3;

    double dataRates[maxGroup];
    dataRates[0] = dataRate1;
    dataRates[1] = dataRate2;
    dataRates[2] = dataRate3;

    std::string tcpTypeIds[maxGroup];
    tcpTypeIds[0] = tcpTypeId1;
    tcpTypeIds[1] = tcpTypeId2;
    tcpTypeIds[2] = tcpTypeId3;

    std::string bitrateTypes[maxGroup];
    bitrateTypes[0] = bitrateType1;
    bitrateTypes[1] = bitrateType2;
    bitrateTypes[2] = bitrateType3;
    
    std::string flowTypes[3]; // Array with type of flow per Group


    if (useAI) {
        if (!g_useAI.SetValue(BooleanValue(true))) {
            NS_ABORT_MSG("Unable to set useAI value to true!");
        }

        int VECTOR_SIZE = 32;

        auto interface = Ns3AiMsgInterface::Get(); // Singleton
        interface->SetIsMemoryCreator(false);
        interface->SetUseVector(true);
        interface->SetHandleFinish(true);
        interface->SetNames(aiSegmentName, "My Cpp to Python Msg", "My Python to Cpp Msg", "My Lockable");
        Ns3AiMsgInterfaceImpl<NrAmc::dataToSend, NrAmc::dataToRecv>* msgInterface =
            interface->GetInterface<NrAmc::dataToSend, NrAmc::dataToRecv>(); // Singleton

        assert(msgInterface->GetCpp2PyVector()->size() == VECTOR_SIZE);

        if (verbose) {
            std::cout << TXT_CYAN << "Using AI" << TXT_CLEAR << std::endl;
        }
    }


    double AppStartTime = 0.2 ; // APP start time
    double AppEndTime = simTime - 0.2; // APP start time
    AppStartTime = std::max(AppStartTime,  iniTime);
    AppEndTime   = std::min(  AppEndTime,  endTime);

    double AppStartTimes[ueNumPergNb];
    double AppEndTimes[ueNumPergNb];

    /* AMC Algorithm change */
    NrAmc::SetCqiModel((NrAmc::CqiAlgorithm)amcAlgorithm);
    NrAmc::Set(cqiHighGain, ProbeCqiDuration, stepFrequency); // To configure the ProbeCQI algorithm
    NrAmc::SetBlerTarget(blerTarget);

    /* Server type - Distance */
    if (serverType == "Remote")
    {
        serverDelay = 0.04;
        serverDelay = serverDelay / 1000;
    }
    else
    {
        serverDelay = 0.004;
    }

    // rlcBuffer = round((dataRate1*ueN1 + dataRate2*ueN2 + dataRate3*ueN3)*1e6/8*serverDelay*rlcBufferPerc/100); // Bytes BDP=250Mbps*100ms default: 999999999
    rlcBuffer = round( 600 * 1e6/8*serverDelay * 2); // channel bandwidth 600 Mbps
    // new Datarate per UE

    // dataRate = dataRate / ueNumPergNb;

    for ( uint32_t g = 0; g < maxGroup; ++g )
    {
        if (verbose){ }
        if ( tcpTypeIds[g].substr(0,3) == "Tcp" ){
            flowTypes[g] = "TCP";
        } else {
            flowTypes[g] = tcpTypeIds[g];
        }
    }


    if (verbose){
        for (uint32_t g = 0; g < maxGroup; ++g )
        {
            std::cout <<"FlowType" << g + 1<< ": " << flowTypes[g] << "\ttcpTypeId" << g + 1 << ": " << tcpTypeIds[g] << "\tUEs: " << ueNs[g]  << std::endl;
            
        }

    }

    
    /********************************************************************************************************************
     * LOGS
    ********************************************************************************************************************/
    // Redirect logs to output file, clog -> LOG_FILENAME
    std::ofstream of(LOG_FILENAME);
    auto clog_buff = std::clog.rdbuf();
    std::clog.rdbuf(of.rdbuf());

    // enable logging
    if (logging)
    {
        
        // LogComponentEnable("MyApp", LOG_LEVEL_INFO);
        // LogComponentEnable("PhysicalDistro", LOG_LEVEL_INFO);
        // LogComponentEnable ("PacketMetadata", LOG_LEVEL_ALL);
        // LogComponentEnable ("Packet", LOG_LEVEL_ALL);
        // LogComponentEnable ("MyAppCCA", LOG_LEVEL_ALL);
        LogComponentEnable ("MyClientApp", LOG_LEVEL_ALL);
        LogComponentEnable ("MyServerApp", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicBbr", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicCongestionControl", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicHeader", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicL4Protocol", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicL5Protocol", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicSocketBase", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicSocketFactory", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicSocketRxBuffer", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicSocketTxBuffer", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicSocketTxEdfScheduler", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicSocketTxPFifoScheduler", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicSocketTxScheduler", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicSocket", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicStreamBase", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicStreamRxBuffer", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicStreamTxBuffer", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicStream", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicSubheader", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicTransportParameters", LOG_LEVEL_ALL);
        // LogComponentEnable ("QuicTransportParameters", LOG_LEVEL_ALL);
        // LogComponentEnable ("Packet", LOG_LEVEL_ALL);
        // LogComponentEnable ("PacketMetadata", LOG_LEVEL_ALL);
        LogComponentEnableAll (LOG_PREFIX_TIME);
        // LogComponentEnable ("Socket", LOG_LEVEL_ALL);
        // LogComponentEnable ("NrUeMac", LOG_LEVEL_ALL);
        // LogComponentEnable ("NrGnbMac", LOG_LEVEL_ALL);
        // LogComponentEnable ("NrMacSchedulerNs3", LOG_LEVEL_ALL);
        // LogComponentEnable ("NrUePhy", LOG_LEVEL_ALL);
        // LogComponentEnable ("NrGnbPhy", LOG_LEVEL_ALL);



        // LogComponentEnable ("ThreeGppSpectrumPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable("ThreeGppPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("ThreeGppChannelModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("ChannelConditionModel", LOG_LEVEL_ALL);
        // LogComponentEnable("TcpCongestionOps",LOG_LEVEL_ALL);
        // LogComponentEnable("TcpBic",LOG_LEVEL_ALL);
        // LogComponentEnable("TcpBbr",LOG_LEVEL_ALL);
        // LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
        // LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
        // LogComponentEnable ("LteRlcUm", LOG_LEVEL_LOGIC);
        // LogComponentEnable ("LtePdcp", LOG_LEVEL_INFO);
    }

    /********************************************************************************************************************
     * Servertype, TCP config & settings, scenario definition
    ********************************************************************************************************************/

    // 
    /*
     * Default values for the simulation. We are progressively removing all
     * the instances of SetDefault, but we need it for legacy code (LTE)
     */
    

   Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(rlcBuffer));
    

    // TCP config
    // TCP Settig
    // attibutes in: https://www.nsnam.org/docs/release/3.27/doxygen/classns3_1_1_tcp_socket.html
    uint32_t delAckCount = 1;
    std::string queueDisc = "FifoQueueDisc";
    queueDisc = std::string("ns3::") + queueDisc;

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4194304)); // TcpSocket maximum transmit buffer size (bytes). 4194304 = 4MB
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(4194304)); // TcpSocket maximum receive buffer size (bytes). 6291456 = 6MB
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10)); // TCP initial congestion window size (segments). RFC 5681 = 10
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(SegmentSize)); // TCP maximum segment size in bytes (may be adjusted based on MTU discovery).
    Config::SetDefault("ns3::TcpSocket::TcpNoDelay", BooleanValue(false)); // Set to true to disable Nagle's algorithm

    if (useECN) {
        Config::SetDefault("ns3::TcpSocketBase::UseEcn", EnumValue(TcpSocketState::On)); // Enables Explicit Congestion Notification (ECN) on TCP Sockets
    }

    // Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (200)));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));  // Number of packets to wait before sending a TCP ack
    Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue (Seconds (.4))); // Timeout value for TCP delayed acks, in seconds. default 0.2 sec
    Config::SetDefault("ns3::TcpSocket::DataRetries", UintegerValue(50)); // Number of data retransmission attempts. Default 6
    // Config::SetDefault("ns3::TcpSocket::PersistTimeout", TimeValue (Seconds (60))); // Number of data retransmission attempts. Default 6

    Config::Set ("/NodeList/*/DeviceList/*/TxQueue/MaxSize", QueueSizeValue (QueueSize ("100p")));
    Config::Set ("/NodeList/*/DeviceList/*/RxQueue/MaxSize", QueueSizeValue (QueueSize ("100p")));

    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("100p"))); //A FIFO packet queue that drops tail-end packets on overflow
    Config::SetDefault(queueDisc + "::MaxSize", QueueSizeValue(QueueSize("100p"))); //100p Simple queue disc implementing the FIFO (First-In First-Out) policy

    Config::SetDefault("ns3::TcpL4Protocol::RecoveryType",
                        TypeIdValue(TypeId::LookupByName("ns3::TcpClassicRecovery"))); //set the congestion window value to the slow start threshold and maintain it at such value until we are fully recovered
    Config::SetDefault ("ns3::RttEstimator::InitialEstimation", TimeValue (MilliSeconds (10)));

    // Cubic Configuration
    // Config::SetDefault("ns3::TcpCubic::Beta", DoubleValue(0.9)); // Beta for multiplicative decrease. Default 0.7

    // BIC Configuration
    // Config::SetDefault("ns3::TcpBic::Beta", DoubleValue(1.5)); // Beta for multiplicative decrease. Default 0.8
    // Config::SetDefault("ns3::TcpBic::HyStart", BooleanValue(false)); // Enable (true) or disable (false) hybrid slow start algorithm. Default true

    // BBR Configuration    
    // Config::SetDefault("ns3::TcpBbr::Stream", UintegerValue(8)); // Random number stream (default is set to 4 to align with Linux results)
    // Config::SetDefault("ns3::TcpBbr::HighGain", DoubleValue(3)); // Value of high gain. Default 2.89
    // Config::SetDefault("ns3::TcpBbr::BwWindowLength", UintegerValue(5)); // Length of bandwidth windowed filter. Default 10
    // Config::SetDefault("ns3::TcpBbr::RttWindowLength", TimeValue(Seconds(1))); // Length of RTT windowed filter. Default 10
    // Config::SetDefault("ns3::TcpBbr::AckEpochAckedResetThresh", UintegerValue(1 << 12)); // Max allowed val for m_ackEpochAcked, after which sampling epoch is reset. Default 1 << 12
    // Config::SetDefault("ns3::TcpBbr::ExtraAckedRttWindowLength", UintegerValue(50)); // Window length of extra acked window. Default 5
    // Config::SetDefault("ns3::TcpBbr::ProbeRttDuration", TimeValue(MilliSeconds(400))); // Time to be spent in PROBE_RTT phase. Default 200
    // LogComponentEnable("TcpBbr",LOG_LEVEL_ALL);

    // QUIC Configuration
    // 4 MB of buffer
    Config::SetDefault ("ns3::QuicSocketBase::SocketRcvBufSize", UintegerValue (1 << 25)); // 1 << 21 or 999999999
    Config::SetDefault ("ns3::QuicSocketBase::SocketSndBufSize", UintegerValue (1 << 25)); // 1 << 21
    Config::SetDefault ("ns3::QuicStreamBase::StreamSndBufSize", UintegerValue (1 << 25)); // 1 << 21
    Config::SetDefault ("ns3::QuicStreamBase::StreamRcvBufSize", UintegerValue (1 << 25)); // 1 << 21

    Config::SetDefault ("ns3::QuicSocketBase::SchedulingPolicy", TypeIdValue(QuicSocketTxEdfScheduler::GetTypeId ()));
    std::string transport_prot = "TcpCubic"; // TcpVegas work
    transport_prot = std::string ("ns3::") + transport_prot;
    Config::SetDefault ("ns3::QuicL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));

    Config::SetDefault ("ns3::QuicSocketState::kMaxPacketsReceivedBeforeAckSend", UintegerValue(1)); // The maximum number of packets without sending an ACK

    // LogLevel log_precision = LOG_LEVEL_INFO;
    // LogComponentEnable ("QuicEchoClientApplication", log_precision);
    // LogComponentEnable ("QuicEchoServerApplication", log_precision);
    // //LogComponentEnable ("QuicSocketBase", log_precision);
    // LogComponentEnable ("QuicStreamBase", log_precision);
    // LogComponentEnable("QuicStreamRxBuffer", log_precision);
    // LogComponentEnable("QuicStreamTxBuffer", log_precision);
    // LogComponentEnable("QuicSocketTxScheduler", log_precision);
    // LogComponentEnable("QuicSocketTxEdfScheduler", log_precision);
    // //LogComponentEnable ("Socket", log_precision);
    // // LogComponentEnable ("Application", log_precision);
    // // LogComponentEnable ("Node", log_precision);
    // //LogComponentEnable ("InternetStackHelper", log_precision);
    // //LogComponentEnable ("QuicSocketFactory", log_precision);
    // //LogComponentEnable ("ObjectFactory", log_precision);
    // //LogComponentEnable ("TypeId", log_precision);
    // //LogComponentEnable ("QuicL4Protocol", log_precision);
    // LogComponentEnable ("QuicL5Protocol", log_precision);
    // //LogComponentEnable ("ObjectBase", log_precision);

    // //LogComponentEnable ("QuicEchoHelper", log_precision);
    // //LogComponentEnable ("UdpSocketImpl", log_precision);
    // //LogComponentEnable ("QuicHeader", log_precision);
    // //LogComponentEnable ("QuicSubheader", log_precision);
    // //LogComponentEnable ("Header", log_precision);
    // //LogComponentEnable ("PacketMetadata", log_precision);
    // LogComponentEnable ("QuicSocketTxBuffer", log_precision);



    /********************************************************************************************************************
    * Create base stations and mobile terminal
    * Define positions, mobility types and speed of UE and gNB.
    ********************************************************************************************************************/
    // create base stations and mobile terminals
    NodeContainer gnbNodes;
    NodeContainer ueNodes;
    gnbNodes.Create(gNbNum);
    ueNodes.Create(gNbNum*ueNumPergNb);

    switch ((PhysicalDistributionOptions)phyDistro)
    {
        case PhysicalDistributionOptions::NEIGHBORHOOD:
            NeighborhoodPhysicalDistribution(gnbNodes, ueNodes);
            break;

        case PhysicalDistributionOptions::IND_ROUTER:
            IndoorRouterPhysicalDistribution(gnbNodes, ueNodes);
            break;

        case PhysicalDistributionOptions::TREES:
            TreePhysicalDistribution(gnbNodes, ueNodes, mobility);
            break;

        case PhysicalDistributionOptions::DEFAULT:
        default:
            DefaultPhysicalDistribution(gnbNodes, ueNodes, mobility);
            break;
    }


    /********************************************************************************************************************
     * NR Helpers and Stuff
     ********************************************************************************************************************/
    
    /**
     * Setup the NR module. We create the various helpers needed for the
     * NR simulation:
     * - EpcHelper, which will setup the core network
     * - IdealBeamformingHelper, which takes care of the beamforming part
     * - NrHelper, which takes care of creating and connecting the various
     * part of the NR stack
     */
    /*
     * Create NR simulation helpers
     */
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    // Configure ideal beamforming method
    idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                         TypeIdValue(DirectPathBeamforming::GetTypeId()));// dir at gNB, dir at UE

    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(epcHelper);

    /*
     * Spectrum configuration. We create a single operational band and configure the scenario.
     */
    // Setup scenario depending if there are buildings or not
    if (enableBuildings)
    {
        scenarioEnum = BandwidthPartInfo::UMa_Buildings;
    } 
    else 
    {
        scenarioEnum = BandwidthPartInfo::UMa; // Urban Macro
    }

    
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1; // in this example we have a single band, and that band is
                                    // composed of a single component carrier

    /* Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
     * a single BWP per CC and a single BWP in CC.
     *
     * Hence, the configured spectrum is:
     *
     * |---------------Band---------------|
     * |---------------CC-----------------|
     * |---------------BWP----------------|
     */
    CcBwpCreator::SimpleOperationBandConf bandConf(frequency,
                                                   bandwidth,
                                                   numCcPerBand,
                                                   scenarioEnum);
    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);

     // Initialize channel and pathloss, plus other things inside band.
     
    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(10)));

    std::string errorModel = "ns3::NrEesmIrT1"; //ns3::NrEesmCcT1, ns3::NrEesmCcT2, ns3::NrEesmIrT1, ns3::NrEesmIrT2, ns3::NrLteMiErrorModel
    // nrHelper->SetUlErrorModel(errorModel); 
    // nrHelper->SetDlErrorModel(errorModel);
    Config::SetDefault("ns3::NrAmc::ErrorModelType", TypeIdValue(TypeId::LookupByName(errorModel)));
    Config::SetDefault("ns3::NrAmc::AmcModel", EnumValue(NrAmc::ErrorModel )); // NrAmc::ShannonModel // NrAmc::ErrorModel

    // std::string pathlossModel="ns3::ThreeGppUmaPropagationLossModel";

    nrHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(10)));
    nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(shadowing)); // false: allow see effect of path loss only
                                                                                // cancel shadowing effect set 0.0

    // Ptr<HybridBuildingsPropagationLossModel> propagationLossModel =
    //     CreateObject<HybridBuildingsPropagationLossModel>(); // BuildingsPropagationLossModel HybridBuildingsPropagationLossModel

    // propagationLossModel->SetAttribute("ShadowSigmaOutdoor", DoubleValue(7.0)); // Standard deviation of the normal distribution used for calculate the shadowing for outdoor nodes
    // propagationLossModel->SetAttribute("ShadowSigmaIndoor", DoubleValue(8.0)); // Standard deviation of the normal distribution used for calculate the shadowing for indoor nodes
    // propagationLossModel->SetAttribute("ShadowSigmaExtWalls", DoubleValue(5.0)); // Standard deviation of the normal distribution used for calculate the shadowing due to ext walls
    // propagationLossModel->SetAttribute("InternalWallLoss", DoubleValue(5.7)); // Additional loss for each internal wall [dB]

    // propagationLossModel->SetAttribute("ExternalWallLoss", DoubleValue(40)); // Additional loss for each internal wall [dB]
    // for (const auto& bwpp : band.GetBwps())
    // {
    //     bwpp.get()->m_propagation = propagationLossModel;   // Set the propagation loss model for each bwp
    // }

    Config::SetDefault("ns3::ThreeGppPropagationLossModel::CustomLoss", DoubleValue(CustomLoss));
    Config::SetDefault("ns3::ThreeGppPropagationLossModel::CustomLossLos", DoubleValue(CustomLossLos));
    Config::SetDefault("ns3::ThreeGppPropagationLossModel::CustomLossNlos", DoubleValue(CustomLossNlos));

    // Initialize channel and pathloss, plus other things inside band.
    nrHelper->InitializeOperationBand(&band);
    BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps({band});

    // Configure scheduler
    nrHelper->SetSchedulerTypeId(NrMacSchedulerTdmaRR::GetTypeId());


    // Antennas for the UEs
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));
    
    // Antennas for the gNbs
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<IsotropicAntennaModel>()));

    // install nr net devices
    NetDeviceContainer gnbNetDev = nrHelper->InstallGnbDevice(gnbNodes, allBwps);
    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice(ueNodes, allBwps);

    int64_t randomStream = 1;
    randomStream += nrHelper->AssignStreams(gnbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);

    for (uint32_t u = 0; u < gnbNodes.GetN(); ++u)
    {
        nrHelper->GetGnbPhy(gnbNetDev.Get(u), 0)->SetTxPower(txPower);
        nrHelper->GetGnbPhy(gnbNetDev.Get(u), 0)->SetAttribute("Numerology", UintegerValue(numerology));
    }

    // Add Noise to all UE
    if (addNoise) 
    {   
        for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
        {
            // Get the physical layer and add noise whenerver DlDataSinr is executed
            Ptr<NrUePhy> uePhy = nrHelper->GetUePhy(ueNetDev.Get(u), 0);
            uePhy->SetNoiseFigure(NOISE_MEAN);

            for (int i = 0; i < (Seconds(simTime) - Seconds(0.2)) / NOISE_T_RES; i++)
            {
                Simulator::Schedule(NOISE_T_RES * i + Seconds(0.1), &AddRandomNoise, uePhy);
            }
        }
    }

    // When all the configuration is done, explicitly call UpdateConfig ()
    for (auto it = gnbNetDev.Begin(); it != gnbNetDev.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }

    for (auto it = ueNetDev.Begin(); it != ueNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }

    /********************************************************************************************************************
     * Setup and install IP, internet and remote servers
     ********************************************************************************************************************/

    if (verbose){
        std::cout << TXT_CYAN << "Install IP" << TXT_CLEAR << std::endl;
    }
    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    if (verbose){
        std::cout << "\t- SGW/PGW" << std::endl;
    }
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));    // Core latency
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    if (verbose){
        std::cout << "\t- Remote Servers" << std::endl;
    }
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(maxGroup);
    Ptr<Node> remoteHosts[maxGroup];
    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        remoteHosts[g] = remoteHostContainer.Get(g);
        
    }
    
    InternetStackHelper internet;
    QuicHelper stack;

    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        if ( flowTypes[g] == "QUIC")
        {
            stack.InstallQuic (remoteHosts[g]);
        }
        else 
        {
            internet.Install(remoteHosts[g]);
        }
       
    }

    // connect a remoteHost to pgw. Setup routing too. Star Topology
    if (verbose){
        std::cout << "\t- Connect each remote Server to PGW" << std::endl;
    }

    PointToPointHelper p2phs[maxGroup];
    NetDeviceContainer internetDevices[maxGroup];
    Ipv4InterfaceContainer internetIpIfaces[maxGroup];
    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        p2phs[g].SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
        p2phs[g].SetDeviceAttribute("Mtu", UintegerValue(1500)); //2500
        p2phs[g].SetChannelAttribute("Delay", TimeValue(Seconds(serverDelay)));

        internetDevices[g] = p2phs[g].Install(pgw, remoteHosts[g]);
        Ipv4AddressHelper ipv4h;
        ipv4h.SetBase(ip_net_Servers[g], ip_mask_Server);
        internetIpIfaces[g] = ipv4h.Assign(internetDevices[g]);
        if (verbose){ std::cout << "\t\t- Server" << g + 1 << " Ready" << std::endl; }
    }
    

    if (verbose){
        std::cout << "\t- Set IP Routes" << std::endl;
    }
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRoutings[maxGroup];

    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        remoteHostStaticRoutings[g] = ipv4RoutingHelper.GetStaticRouting(remoteHosts[g]->GetObject<Ipv4>());
        remoteHostStaticRoutings[g]->AddNetworkRouteTo(Ipv4Address(ip_net_UE), Ipv4Mask(ip_mask_UE), 1);
    }

    if (verbose){
        std::cout << "\t- Set IP to UE" << std::endl;
    }
    uint32_t ueActual = 0;
    for (uint32_t g = 0; g < maxGroup; ++g )
    {

        for (uint32_t u = 0; u < ueNs[g] ; ++u)
        {
            Ptr<Node> ueNode = ueNodes.Get(ueActual);
            if ( flowTypes[g] == "QUIC")
            {
                stack.InstallQuic (ueNode);
            }
            else
            {
                internet.Install(ueNode);
            }
            ueActual++;
            if (verbose){ std::cout << "\t\t- UE" << ueActual << " Ready" << std::endl; }
        }
    }

    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNetDev));

    if (verbose){
        std::cout << "\t- Set UEs Position" << std::endl;
    }
    double UEposX[ueNumPergNb];
    double UEposY[ueNumPergNb];
    double UEposZ[ueNumPergNb];
    double UEspeedX[ueNumPergNb];
    double UEspeedY[ueNumPergNb];
    
    // double xUE = 20;  //Initial X Position UE
    double yUE = 0;
    if (circlepath){
        yUE = 30;  //Initial Y Position UE
    }else{
        yUE = 10;  //Initial Y Position UE

    }
    double hUE = 1.5;          // user antenna height in meters
    double ueDistance = .01;    //Distance between UE
    // double speedX = 1;               // in m/s for walking UT.
    double speedY = 0;               // in m/s for walking UT.

    ueActual = 0;
    // for (uint32_t g = 0; g < maxGroup; ++g )
    // {
    //     if (verbose){ std::cout << "\t\tUEs group " << g + 1 << std::endl; }
    //     for (uint32_t u = 0; u < ueNs[g] ; ++u)
    //     {
    //         UEposX[ueActual] = xUEs[g]+ (float) ueDistance * ueActual;
    //         UEposY[ueActual] = yUE;
    //         UEposZ[ueActual] = hUE ;
    //         UEspeedX[ueActual] = speedXs[g];
    //         UEspeedY[ueActual] = speedY;
            
    //         ueNodes.Get(ueActual)->GetObject<MobilityModel>()->SetPosition(Vector((float) UEposX[ueActual], (float) UEposY[ueActual] , (float) UEposZ[ueActual])); // (x, y, z) in m
    //         ueNodes.Get(ueActual)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector( UEspeedX[ueActual], UEspeedY[ueActual],  0)); // move UE1 along the x axis
    //         if (verbose){ std::cout << "\t\t- UE: " << ueActual + 1 << "\tPos:(" << UEposX[ueActual] << ", " << UEposY[ueActual] << ", " << UEposZ[ueActual] << ")\t Speed: (" << UEspeedX[u] << ", " << UEspeedY[u] <<", 0)" << std::endl; }
    //         NS_LOG_INFO( "- UE: " << ueActual << "\t" << "Pos: "<<"(" << UEposX[ueActual] << ", " << UEposY[ueActual] << ", " << UEposZ[ueActual] << ")" << "\t" << "Speed: (" << UEspeedX[u] << ", " << UEspeedY[u] <<", 0)" );
    //         ueActual++;
    //     }
    // }

    if (circlepath){
        if (verbose){ std::cout << "\t- CirclePath" << std::endl; }
        Simulator::Schedule(MilliSeconds(100), &ChangeVelocity, &ueNodes, &gnbNodes);
    }

    // attach UEs to the closest eNB
    if (verbose){ std::cout << "\t- Attach UEs to eNb" << std::endl; }
    nrHelper->AttachToClosestEnb(ueNetDev, gnbNetDev);

    // assign IP address to UEs
    if (verbose){ std::cout << "\t- Assign IP Address to UEs" << std::endl; }
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        if (verbose){
            std::cout << "\t\t- IP to UE" << u + 1 << std::endl;
        }
        Ptr<Node> ueNode = ueNodes.Get(u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }



    if (verbose){ std::cout << TXT_CYAN << "Install TCP and App" << TXT_CLEAR << std::endl; }

    if (verbose){ std::cout << "\t- AppStartTime = "<< AppStartTime << std::endl; }

    //Seed the random number generator of c++ with the current time
    //This will be used to change the run number everytime we run the simulation

    //Change the seed and the run for ns3
    RngSeedManager::SetSeed(10);
    RngSeedManager::SetRun (rng);
       
    Ptr<UniformRandomVariable>  uv;
    uv = CreateObject<UniformRandomVariable>();
    uv->SetAttribute ("Min", DoubleValue (0));
    uv->SetAttribute ("Max", DoubleValue (1));
    double dt = 0;


    ueActual = 0;
    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        if (verbose){ std::cout << "\t- Server " << g + 1 << std::endl; }
        if ( flowTypes[g] == "TCP" ){
            if (verbose){ std::cout << "\t\t- Install "<< flowTypes[g] <<" in server: " << std::to_string(remoteHosts[g]->GetId()) << std::endl; }

            // Create and bind the socket...
            TypeId tid = TypeId::LookupByName("ns3::" + tcpTypeIds[g]);
            std::stringstream nodeId;
            nodeId << remoteHosts[g]->GetId();
            std::string specificNode = "/NodeList/" + nodeId.str() + "/$ns3::TcpL4Protocol/SocketType";
            Config::Set(specificNode, TypeIdValue(tid));
            Ptr<Socket> localSocket =
                Socket::CreateSocket(remoteHosts[g], TcpSocketFactory::GetTypeId());
        }

        // Configuring APPs
        if (verbose){ std::cout << "\t\t- " << ueNs[g] << " UEs in server: " << std::to_string(remoteHosts[g]->GetId()) << std::endl; }
        for (uint32_t u = 0; u < ueNs[g] ; ++u)
        {
            if (randstart)
            {
                dt = uv->GetValue(); 
            }
            if (verbose) { std::cout << "\t\t- Starting at "<< AppStartTime + dt << std::endl; }


            uint16_t connNumPerUe = 1;
            if (verbose){ std::cout << "\t\t- Install APP "<< tcpTypeIds[g] <<" between nodes: " << std::to_string(remoteHosts[g]->GetId()) << "<->"<< std::to_string(ueNodes.Get (ueActual)->GetId()) << std::endl; }

            if ( flowTypes[g] == "UDP")
            {
                uint16_t dlPort = 1234;

                for (uint32_t c = 0; c < connNumPerUe; ++c){
                    InstallUDP (remoteHosts[g], ueNodes.Get (ueActual), dlPort, AppStartTime + dt, AppEndTime, dataRates[g], bitrateTypes[g]);
                }
            }else if ( flowTypes[g] == "TCP")
            {
                uint16_t serverPort = 8080 + u;
                
                for (uint32_t c = 0; c < connNumPerUe; ++c){
                    InstallTCP (remoteHosts[g], ueNodes.Get (ueActual), serverPort++, AppStartTime + dt, AppEndTime, dataRates[g], bitrateTypes[g]);
                }
                
                // Install Tracer
                Simulator::Schedule(Seconds(AppStartTime) , 
                                    &TraceTcp, remoteHosts[g]->GetId(), u + 1  );

            }else if ( flowTypes[g] == "QUIC")
            {
                uint32_t dlPort = 1025;
 
                 for (uint32_t c = 0; c < connNumPerUe; ++c){
                    InstallQUIC (remoteHosts[g], ueNodes.Get (ueActual), dlPort++, AppStartTime + dt, AppEndTime, dataRates[g], bitrateTypes[g]);
                }
                // Install Tracer
                Simulator::Schedule(Seconds(AppStartTime) , 
                                    &TraceQuic, remoteHosts[g]->GetId(), u );

           }
            AppStartTimes[ueActual] = AppStartTime + dt;
            AppEndTimes[ueActual] = AppEndTime;

            ueActual++;

        }
    }


    

    /********************************************************************************************************************
    * Trace and file generation
    ********************************************************************************************************************/
    // enable the traces provided by the nr module
    if (NRTrace)
    {
        if (verbose){
            std::cout << "\t- NR Trace "<< std::endl ;
        }
        nrHelper->EnableTraces();
    }

    // All tcp trace
    if(TCPTrace){
        if (verbose){
            std::cout << "\t- TCP ASCII Trace "<< std::endl ;
        }
        Ptr<OutputStreamWrapper> ascii_wrapServer[maxGroup];
        Ptr<OutputStreamWrapper> ascii_wrapUE;
        for (uint32_t g = 0; g < maxGroup; ++g )
        {
            std::string  filestream = "ip-ascii-" + std::to_string(g);
            filestream = filestream + "-all.txt";
            ascii_wrapServer[g] = new OutputStreamWrapper(filestream, std::ios::out);
            internet.EnableAsciiIpv4(ascii_wrapServer[g], internetIpIfaces[g]);
            
        }
        
        ascii_wrapUE = new OutputStreamWrapper("ip-ascii-all-ue.txt", std::ios::out);
        internet.EnableAsciiIpv4(ascii_wrapUE, ueIpIface);

        for (uint32_t g = 0; g < maxGroup; ++g )
        {
            // p2phs[g].EnablePcap("p2p_server_gw", internetDevices[g]);
        }

    }


    // Status Bar
    if (verbose){
        std::cout << "\t- Status Bar "<< std::endl ;
        Simulator::Schedule(MilliSeconds(100), &ShowStatus);
    }

    // Calculate the node positions
    if (verbose){
        std::cout << "\t- Node Position "<< std::endl ;
    }
    std::string logMFile="mobilityPosition.txt";
    std::ofstream mymcf;
    mymcf.open(logMFile);
    mymcf  << "Time\t" << "UE\t" << "x\t" << "y\t" << "Vx\t" << "Vy\t"  << "D0" << std::endl;
    Simulator::Schedule(MilliSeconds(100), &CalculatePosition, &ueNodes, &gnbNodes, &mymcf);

    // 
    // generate graph.ini
    //
    if (verbose){
        std::cout << "\t- graph.ini "<< std::endl ;
    }
    std::string iniFile="graph.ini";
    std::ofstream inif;
    inif.open(iniFile);
    inif << "[general]" << std::endl;
    inif << "resamplePeriod = 100" << std::endl;
    inif << "simTime = " << simTime << std::endl;
    inif << "iniTime = " << iniTime << std::endl;
    inif << "endTime = " << endTime << std::endl;
    inif << "AppStartTime = " << AppStartTime << std::endl;
    inif << "AppEndTime = " << AppEndTime << std::endl;
    inif << "NRTrace = " << NRTrace << std::endl;
    inif << "TCPTrace = " << TCPTrace << std::endl;
    // inif << "flowType = " << flowType1 << std::endl;

    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        inif << "flowType" << g + 1 << " = " << flowTypes[g] << std::endl;
    }
    // inif << "tcpTypeId = " << tcpTypeId1 << std::endl;
    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        inif << "tcpTypeId" << g + 1 << " = " << tcpTypeIds[g] << std::endl;
    }
    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        inif << "bitrateType" << g + 1 << " = " << bitrateTypes[g] << std::endl;
    }
    inif << "frequency = " << frequency << std::endl;
    inif << "bandwidth = " << bandwidth << std::endl;
    // inif << "serverID = " << (int)(ueNumPergNb+gNbNum+2+1) << std::endl;
    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        inif << "serverID" << g + 1 << " = " << (int)(ueNumPergNb+gNbNum+3+g) << std::endl;
    }
    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        inif << "ip_net_Server" << g + 1 << " = " << ip_net_Servers[g] << std::endl;
    }
    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        inif << "UEN" << g + 1 << " = " << (int)(ueNs[g]) << std::endl;
    }
    // inif << "ip_mask_Server = " << ip_mask_Server << std::endl;
    
    inif << "UENum = " << (int)(ueNumPergNb) << std::endl;
    inif << "SegmentSize = " << SegmentSize << std::endl;
    inif << "rlcBuffer = " << rlcBuffer << std::endl;
    inif << "rlcBufferPerc = " << rlcBufferPerc << std::endl;
    inif << "serverType = " << serverType << std::endl;
    inif << "dataRate1 = " << dataRate1 << std::endl;
    inif << "dataRate2 = " << dataRate2 << std::endl;
    inif << "dataRate3 = " << dataRate3 << std::endl;
    inif << "phyDistro = "   << phyDistro   << std::endl;
    inif << "AQMEnq = " << AQMEnq  << std::endl;
    inif << "AQMDeq = " << AQMDeq  << std::endl;
    inif << "circlepath = "   << circlepath   << std::endl;
    inif << "CustomLoss = " << CustomLoss << std::endl;
    inif << "CustomLossLos = " << CustomLossLos << std::endl;
    inif << "CustomLossNlos = " << CustomLossNlos << std::endl;
    inif << "useECN = " << useECN << std::endl;
    inif << "amcAlgorithm = " << +amcAlgorithm << std::endl;
    inif << "cqiHighGain = " << +cqiHighGain << std::endl;
    inif << "ProbeCqiDuration = " << ProbeCqiDuration.GetSeconds()*1000 << " ms" << std::endl;
    inif << "stepFrequency = " << stepFrequency.GetSeconds()*1000 << " ms" << std::endl;
    inif << "addNoise = " << addNoise << std::endl;
    inif << "simlabel = " << "A" << amcAlgorithm << "S" << phyDistro << std::endl;
    inif << std::endl;

    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        inif << "[server-" << g + 1 << "]" << std::endl;
        inif << "flowType" << g + 1 << " = " << flowTypes[g] << std::endl;
        inif << "tcpTypeId" << g + 1 << " = " << tcpTypeIds[g] << std::endl;
        inif << "SegmentSize = " << SegmentSize << std::endl;
        inif << "ip_net_Server = " << ip_net_Servers[g] << std::endl;
        inif << "ip_mask_Server = " << ip_mask_Server << std::endl;
        inif << "serverID = " << (int)(ueNumPergNb + gNbNum + 3 + g) << std::endl;
        inif << "dataRate = " << dataRates[g] << std::endl;
        inif << "UEN = " << (int)(ueNs[g]) << std::endl;
        inif << std::endl;
    }

    inif << "[gNb]" << std::endl;
    inif << "frequency = " << frequency << std::endl;
    inif << "bandwidth = " << bandwidth << std::endl;
    inif << "gNbNum = " << gNbNum << std::endl;
    inif << "gNbX = "   << gNbX   << std::endl;
    inif << "gNbY = "   << gNbY   << std::endl;
    inif << "gNbD = "   << gNbD   << std::endl;
    inif << "ip_net_UE = "   << ip_net_UE   << std::endl;
    inif << "ip_mask_UE = "  << ip_mask_UE   << std::endl;
    inif << std::endl;

    for (uint32_t u = 0; u < ueNumPergNb; ++u)
    {
        inif << "[UE-" << (u + 1) << "]" << std::endl;
        inif << "xUE = " << UEposX[u] << std::endl;
        inif << "yUE = " << UEposY[u] << std::endl;
        inif << "zUE = " << UEposZ[u] << std::endl;
        inif << "speedX = " << UEspeedX[u] << std::endl;
        inif << "speedY = " << UEspeedY[u] << std::endl;
        inif << "AppStartTime = " << AppStartTimes[u] << std::endl;
        inif << "AppEndTime = " << AppEndTimes[u] << std::endl;
        inif << "circlepath = "   << circlepath   << std::endl;
        inif << std::endl;
        
    }

    inif << std::endl;
    inif << "[building]" << std::endl;
    inif << "enableBuildings = " << enableBuildings << std::endl;
    inif << "gridWidth = " << gridWidth << std::endl;
    inif << "buildN = " << numOfBuildings << std::endl;
    inif << "buildX = " << buildX << std::endl;
    inif << "buildY = " << buildY << std::endl;
    inif << "buildDx = " << buildDx << std::endl;
    inif << "buildDy = " << buildDy << std::endl;
    inif << "buildLx = " << buildLx << std::endl;
    inif << "buildLy = " << buildLy << std::endl;

    inif.close();


    if (verbose){
        std::cout << "\t- Node Address Info"<< std::endl ;
    }
    PrintNodeAddressInfo(true);

    if (verbose){
        std::cout << "Flow Monitor"<< std::endl ;
    }
    FlowMonitorHelper flowmonHelper;
    NodeContainer endpointNodes;
    for (uint32_t g = 0; g < maxGroup; ++g )
    {
        endpointNodes.Add(remoteHosts[g]);
    }
    endpointNodes.Add(ueNodes);

    Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install(endpointNodes);
    monitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));
    

    // /* REM HELPER, uncomment for use  */
    // Ptr<NrRadioEnvironmentMapHelper> remHelper = CreateObject<NrRadioEnvironmentMapHelper>();
    // remHelper->SetMinX(0);
    // remHelper->SetMaxX(40);
    // remHelper->SetResX(40);
    // remHelper->SetMinY(0);
    // remHelper->SetMaxY(170);
    // remHelper->SetResY(170);
    // if (phyDistro == 1)
    //     remHelper->SetZ(1.5);
    // else if (phyDistro == 2)
    //     remHelper->SetZ(25.5);
    // remHelper->SetSimTag("rem");

    // remHelper->SetRemMode(NrRadioEnvironmentMapHelper::COVERAGE_AREA);
    // remHelper->CreateRem(gnbNetDev.Get(0), ueNetDev.Get(0), 0);

    if (verbose){
        std::cout << "Run Simulation"<< std::endl ;
    }

    globalMonitor = monitor;
    globalFlowClassifier = flowmonHelper.GetClassifier();
    globalAppStartTime = AppStartTime;

    Simulator::Schedule(Seconds(0.5), &ProcessFlowMonitorCallback);

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // static std::map<FlowId, uint64_t> rxBytesOldMap;
    // Simulator::Schedule(Seconds(1.0), processFlowMonitor(monitor, flowmonHelper.GetClassifier(), AppStartTime));
    // processFlowMonitor(monitor, flowmonHelper.GetClassifier(), AppStartTime);


    Simulator::Destroy();
    mymcf.close();

    std::clog.rdbuf(clog_buff); // Redirect clog to original buffer

    if (verbose){
        std::cout << "\nThis is The End" << std::endl;
    }
    auto toc = std::chrono::high_resolution_clock::now();
    std::cout << "Total Time: " << "\033[1;35m"  << 1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(toc-itime).count() << "\033[0m"<<  std::endl;

    return 0;
}



// Trace congestion window
static void
CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    //*stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / SegmentSize << std::endl;
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldval  << "\t" << newval  << std::endl;
}


static void
RtoTracer(Ptr<OutputStreamWrapper> stream, Time oldval, Time newval)
{
    *stream->GetStream()  << Simulator::Now().GetSeconds() << "\t" << (float) oldval.GetSeconds()<< "\t" << (float) newval.GetSeconds() << std::endl;
}
/**
 * RTT tracer.
 *
 * \param context The context.
 * \param oldval Old value.
 * \param newval New value.
 */
static void
RttTracer(Ptr<OutputStreamWrapper> stream, Time oldval, Time newval)
{
    *stream->GetStream()  << Simulator::Now().GetSeconds() << "\t" << (float) oldval.GetSeconds()<< "\t" << (float) newval.GetSeconds() << std::endl;

}

/**
 * Next TX tracer.
 *
 * \param context The context.
 * \param old Old sequence number.
 * \param nextTx Next sequence number.
 */
static void
NextTxTracer(Ptr<OutputStreamWrapper> stream, SequenceNumber32 old [[maybe_unused]], SequenceNumber32 nextTx)
{
    *stream->GetStream()  << Simulator::Now().GetSeconds() << "\t" << old<< "\t" << nextTx << std::endl;
}

/**
 * Next RX tracer.
 *
 * \param context The context.
 * \param old Old sequence number.
 * \param nextRx Next sequence number.
 */
static void
NextRxTracer(Ptr<OutputStreamWrapper> stream, SequenceNumber32 old [[maybe_unused]], SequenceNumber32 nextRx)
{
    *stream->GetStream()  << Simulator::Now().GetSeconds() << "\t" << old<< "\t" << nextRx << std::endl;
}

/**
 * In-flight tracer.
 *
 * \param context The context.
 * \param old Old value.
 * \param inFlight In flight value.
 */
static void
InFlightTracer(Ptr<OutputStreamWrapper> stream, uint32_t old [[maybe_unused]], uint32_t inFlight)
{
    *stream->GetStream()  << Simulator::Now().GetSeconds() << "\t" << old<< "\t" << inFlight << std::endl;
}

/**
 * Slow start threshold tracer.
 *
 * \param context The context.
 * \param oldval Old value.
 * \param newval New value.
 */
static void
SsThreshTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream()  << Simulator::Now().GetSeconds() << "\t" << oldval<< "\t" << newval << std::endl;
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
}

static void
TraceQuic(uint32_t nodeId, uint32_t socketId)
{
    if (verbose){
        std::cout << TXT_CYAN << "\nStart trace QUIC: Server: " << nodeId << " Socket: " << socketId << " at: "<<  
            1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(tic-itime).count()<< TXT_CLEAR <<
            " s" << std::endl;
    }

    AsciiTraceHelper asciiTraceHelper;

    // Init Congestion Window Tracer
    // std::ostringstream pathCW;
    // pathCW << "/NodeList/" << std::to_string(nodeId) << "/$ns3::QuicL4Protocol/SocketList/" << std::to_string(socketId) << "/QuicSocketBase/CongestionWindow";
    // std::ostringstream fileCW;
    // fileCW << "QUIC-cwnd-change-"  << nodeId << ".txt" ;
    // Ptr<OutputStreamWrapper> cw_stream = asciiTraceHelper.CreateFileStream (fileCW.str ().c_str ());
    // Config::ConnectWithoutContext (pathCW.str ().c_str (), MakeBoundCallback(&CwndChange, cw_stream));

    // // Init Congestion RTT
    // std::ostringstream pathRTT;
    // pathRTT << "/NodeList/" << std::to_string(nodeId) << "/$ns3::QuicL4Protocol/SocketList/" << std::to_string(socketId) << "/QuicSocketBase/RTT";
    // std::ostringstream fileRTT;
    // fileRTT << "QUIC-rtt-"  << nodeId << ".txt" ;
    // Ptr<OutputStreamWrapper> rtt_stream = asciiTraceHelper.CreateFileStream (fileRTT.str ().c_str ());
    // Config::ConnectWithoutContext (pathRTT.str ().c_str (), MakeBoundCallback(&RttChange, rtt_stream));


}



static void
TraceTcp(uint32_t nodeId, uint32_t socketId)
{
    if (verbose){
        std::cout << TXT_CYAN << "\nStart trace TCP: Server: " << nodeId << " Socket: " << socketId << " at: "<<  
            1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(tic-itime).count()<< TXT_CLEAR <<
            " s" << std::endl;
    }


    // Init Congestion Window Tracer
    AsciiTraceHelper asciicwnd;
    Ptr<OutputStreamWrapper> stream = asciicwnd.CreateFileStream( "tcp-cwnd-"
                                    + std::to_string(nodeId) +"-"+std::to_string(socketId)+".txt");
    *stream->GetStream() << "Time" << "\t" << "oldval" << "\t" << "newval" << std::endl;

    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                    "/$ns3::TcpL4Protocol/SocketList/" +
                                    std::to_string(socketId) + "/CongestionWindow",
                                    MakeBoundCallback(&CwndTracer, stream));

    // Init Congestion RTO
    AsciiTraceHelper asciirto;
    Ptr<OutputStreamWrapper> rtoStream = asciirto.CreateFileStream("tcp-rto-"
                                    + std::to_string(nodeId) +"-"+std::to_string(socketId)+".txt");
    *rtoStream->GetStream() << "Time" << "\t" << "oldval" << "\t" << "newval" << std::endl;
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                    "/$ns3::TcpL4Protocol/SocketList/" +
                                    std::to_string(socketId) + "/RTO",
                                    MakeBoundCallback(&RtoTracer,rtoStream));

    // Init Congestion RTT
    AsciiTraceHelper asciirtt;
    Ptr<OutputStreamWrapper> rttStream = asciirtt.CreateFileStream("tcp-rtt-"
                                    + std::to_string(nodeId) +"-"+std::to_string(socketId)+".txt");
    *rttStream->GetStream() << "Time" << "\t" << "oldval" << "\t" << "newval" << std::endl;
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                    "/$ns3::TcpL4Protocol/SocketList/" +
                                    std::to_string(socketId) + "/RTT",
                                    MakeBoundCallback(&RttTracer,rttStream));

    // Init Congestion NextTxTracer
    AsciiTraceHelper asciinexttx;
    Ptr<OutputStreamWrapper> nexttxStream = asciinexttx.CreateFileStream("tcp-nexttx-"
                                    + std::to_string(nodeId) +"-"+std::to_string(socketId)+".txt");
    *nexttxStream->GetStream() << "Time" << "\t" << "oldval" << "\t" << "newval" << std::endl;
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                    "/$ns3::TcpL4Protocol/SocketList/" +
                                    std::to_string(socketId) + "/NextTxSequence",
                                    MakeBoundCallback(&NextTxTracer,nexttxStream));

    // Init Congestion NextRxTracer
    AsciiTraceHelper asciinextrx;
    Ptr<OutputStreamWrapper> nextrxStream = asciinextrx.CreateFileStream("tcp-nextrx-"
                                    + std::to_string(nodeId) +"-"+std::to_string(socketId)+".txt");
    *nextrxStream->GetStream() << "Time" << "\t" << "oldval" << "\t" << "newval" << std::endl;
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                    "/$ns3::TcpL4Protocol/SocketList/" +
                                    std::to_string(socketId) + "/RxBuffer/NextRxSequence",
                                    MakeBoundCallback(&NextRxTracer,nextrxStream));

                                    
    // Init Congestion InFlightTracer
    AsciiTraceHelper asciiinflight;
    Ptr<OutputStreamWrapper> inflightStream = asciiinflight.CreateFileStream("tcp-inflight-"
                                    + std::to_string(nodeId) +"-"+std::to_string(socketId)+".txt");
    *inflightStream->GetStream() << "Time" << "\t" << "oldval" << "\t" << "newval" << std::endl;
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                    "/$ns3::TcpL4Protocol/SocketList/" +
                                    std::to_string(socketId) + "/BytesInFlight",
                                    MakeBoundCallback(&InFlightTracer,inflightStream));

    // Init Congestion SsThreshTracer
    AsciiTraceHelper asciissth;
    Ptr<OutputStreamWrapper> ssthStream = asciissth.CreateFileStream("tcp-ssth-"
                                    + std::to_string(nodeId) +"-"+std::to_string(socketId)+".txt");
    *ssthStream->GetStream() << "Time" << "\t" << "oldval" << "\t" << "newval" << std::endl;
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                    "/$ns3::TcpL4Protocol/SocketList/" +
                                    std::to_string(socketId) + "/SlowStartThreshold",
                                    MakeBoundCallback(&SsThreshTracer,ssthStream));

}

static void InstallUDP (Ptr<Node> remoteHost,
                        Ptr<Node> sender,
                        uint16_t sinkPort,
                        float startTime,
                        float stopTime, 
                        float dataRate,
                        std::string _bitrateType)
{
    /*
     * remoteHost ----------> sender 
     * 
     * 
    */
     
    // double interval = SegmentSize*8/dataRate; // MicroSeconds
    std::cout << "\t\t- UDP " << std::to_string(remoteHost->GetId()) << "<->"<< std::to_string(sender->GetId()) ;
    std::cout << "\tDataRate: " << dataRate << "\tPacketSize: " << SegmentSize << std::endl;
    
    Address sinkAddress (InetSocketAddress (sender->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal (), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (sender);

    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (simTime));

    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (remoteHost, UdpSocketFactory::GetTypeId ());
    Ptr<MyServerApp> udp_app = CreateObject<MyServerApp> ();
    udp_app->Setup (ns3UdpSocket, sinkAddress, sinkPort, SegmentSize, 1000000000, DataRate (std::to_string(dataRate) + "Mb/s"), _bitrateType, false);

    remoteHost->AddApplication (udp_app);

    udp_app->SetStartTime (Seconds (startTime));
    udp_app->SetStopTime (Seconds (stopTime));



}

static void InstallTCP (Ptr<Node> remoteHost,
                        Ptr<Node> sender,
                        uint16_t serverPort,
                        float startTime,
                        float stopTime, 
                        float dataRate, 
                        std::string _bitrateType)
{
    /*
     * remoteHost ----------> sender 
     * 
     * 
    */

    std::cout << "\t\t- TCP " << std::to_string(remoteHost->GetId()) << "<->"<< std::to_string(sender->GetId()) 
                << "\tDataRate: " << dataRate
                << "\tPort: " << serverPort << std::endl;


    // Address sinkAddress (InetSocketAddress (sender->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal (), serverPort));
    Address clientAddress (InetSocketAddress (sender->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ()));

    // PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), serverPort));
    // ApplicationContainer sinkApps = packetSinkHelper.Install (sender);

    // sinkApps.Start (Seconds (0.));
    // sinkApps.Stop (Seconds (simTime));
    // Client APP

    //     VideoStreamClientHelper videoClient (internetIpIfaces.GetAddress (1), 5000);
    // ApplicationContainer clientApp = videoClient.Install (ueNodes.Get (0));
    // clientApp.Start (Seconds (0.5));
    // clientApp.Stop (Seconds (100.0));


    Ptr<Socket> serverSocket = Socket::CreateSocket (remoteHost, TcpSocketFactory::GetTypeId ());

    // Server APP
    Ptr<MyServerApp> tcp_server_app = CreateObject<MyServerApp> ();
    tcp_server_app->Setup (serverSocket, clientAddress, serverPort , SegmentSize, 1000000000, DataRate (std::to_string(dataRate) + "Mb/s"), _bitrateType, true);
    remoteHost->AddApplication (tcp_server_app);

    tcp_server_app->SetStartTime (Seconds (startTime));
    tcp_server_app->SetStopTime (Seconds (stopTime));
    
    Ptr<Socket> clientSocket = Socket::CreateSocket (sender, TcpSocketFactory::GetTypeId ());
    Ptr<MyClientApp> tcp_client_app = CreateObject<MyClientApp> ();
    tcp_client_app->Setup (clientSocket, remoteHost, serverPort);
    sender->AddApplication (tcp_client_app);

    tcp_client_app->SetStartTime (Seconds (startTime));
    tcp_client_app->SetStopTime (Seconds (stopTime));

}

static void InstallQUIC (Ptr<Node> remoteHost,
                        Ptr<Node> sender,
                        uint16_t sinkPort,
                        float startTime,
                        float stopTime, 
                        float dataRate,
                        std::string _bitrateType)
{

    double SegmentSize = 1200;
    // double interval = SegmentSize*8/dataRate; // MicroSeconds
    // double interval = SegmentSize*8 / dataRate ; // MicroSeconds

    std::cout << "\t\t- QUIC " << std::to_string(remoteHost->GetId()) << "<->"<< std::to_string(sender->GetId()) ;
    std::cout << "\tDataRate: " << dataRate << "\tPacketSize: " << SegmentSize << std::endl;

    QuicServerHelper dlPacketSinkHelper (sinkPort);
    ApplicationContainer sinkApps = dlPacketSinkHelper.Install (sender);

    Address sinkAddress (InetSocketAddress (sender->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal (), sinkPort));
    // PacketSinkHelper packetSinkHelper ("ns3::QuicSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    // ApplicationContainer sinkApps = packetSinkHelper.Install (sender);

    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (simTime));

    Ptr<Socket> ns3QuicSocket = Socket::CreateSocket (remoteHost, QuicSocketFactory::GetTypeId ());
    Ptr<MyServerApp> quic_app = CreateObject<MyServerApp> (); 
    quic_app->Setup (ns3QuicSocket, sinkAddress, sinkPort, SegmentSize, 1000000000, DataRate (std::to_string(dataRate) + "Mb/s"), _bitrateType, false);

    remoteHost->AddApplication (quic_app);

    quic_app->SetStartTime (Seconds (startTime));
    quic_app->SetStopTime (Seconds (stopTime));




}

static void
ShowStatus()
{
    auto toc = std::chrono::high_resolution_clock::now();
    Time now = Simulator::Now(); 
    pid_t pid = getpid();

    std::string ok = "■";
    std::string nok = ".";
    std::string here = "-";
    std::string bar = "";
    std::string pre = "";
    int sizeBar = 20;
    int n = round((now.GetSeconds() / simTime) * sizeBar);

    for (int i = 1; i < n ; ++i) {
        bar = bar + ok;
    }
    // blink effect
    if ((int) round(now.GetSeconds()*10) % 2 == 0){
        bar = bar + here;
    }else{
        bar = bar + nok;
    }
    for (int i = n; i < sizeBar ; ++i) {
        bar = bar + nok;
    }
    if ((now.GetSeconds()  < 10 ) && (simTime >= 10)){
        pre = " ";
    }
    double PT = 1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(toc-tic).count()  ; // Process Time each Cycle
    double ET = 1.e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(toc-itime).count() ; // Elapsed Time from start
    double RT = 1.e-9*(std::chrono::duration_cast<std::chrono::nanoseconds>(toc-itime).count() * simTime / now.GetSeconds() - std::chrono::duration_cast<std::chrono::nanoseconds>(toc-itime).count())  ; // Remanent Time estimation
    std::cout << "\r\e[K" <<
            "pid: " << TXT_GREEN << pid << TXT_CLEAR << 
            " [ "  << bar << " ]" <<
            " ST: " << "\033[1;32m" << pre << std::fixed << std::setprecision(1) <<now.GetSeconds() << "/" << std::fixed << std::setprecision(0) << simTime << "\033[0m" <<  
            " PT: " << "\033[1;32m" << std::fixed << std::setprecision(1) << PT << "\033[0m" <<
            " ET: " << "\033[1;35m" << std::fixed << std::setprecision(1) << ET << "\033[0m" <<
            " RT: " << "\033[1;34m" << std::fixed << std::setprecision(1) << RT << " ( " << RT / 60 << " m )" << "\033[0m" <<
            std::flush;

    tic=toc;
    Simulator::Schedule(MilliSeconds(100), &ShowStatus);
}


/**
 * Calulate the Position
 */

static void
CalculatePosition(NodeContainer* ueNodes, NodeContainer* gnbNodes, std::ostream* os)
{
    auto toc = std::chrono::high_resolution_clock::now();
    Time now = Simulator::Now(); 
    
  
    for (uint32_t u = 0; u < ueNodes->GetN(); ++u)
    {
        Ptr<MobilityModel> modelu = ueNodes->Get(u)->GetObject<MobilityModel>();
        Ptr<ConstantVelocityMobilityModel> modele = ueNodes->Get(u)->GetObject<ConstantVelocityMobilityModel>();
        Ptr<MobilityModel> modelb = gnbNodes->Get(0)->GetObject<MobilityModel>();
        // Vector NBposition = modelb->GetPosition ();
        Vector UEposition = modelu->GetPosition ();
        Vector velocity = modelu->GetVelocity ();
        double distance = modelu->GetDistanceFrom (modelb);


        // write new info 
        *os  << now.GetSeconds()<< "\t" << (u+1) << "\t"<< UEposition.x << "\t" << UEposition.y << "\t" << velocity.x << "\t" << velocity.y << "\t" << distance << std::endl;

        
    }
    tic=toc;

    Simulator::Schedule(MilliSeconds(100), &CalculatePosition, ueNodes, gnbNodes, os);
}

/**Change Velocity
 */

static void
ChangeVelocity(NodeContainer* ueNodes, NodeContainer* gnbNodes)
{
    auto toc = std::chrono::high_resolution_clock::now();
    Time now = Simulator::Now(); 
    
  
    for (uint32_t u = 0; u < ueNodes->GetN(); ++u)
    {
        Ptr<MobilityModel> modelu = ueNodes->Get(u)->GetObject<MobilityModel>();
        Ptr<ConstantVelocityMobilityModel> modele = ueNodes->Get(u)->GetObject<ConstantVelocityMobilityModel>();
        Ptr<MobilityModel> modelb = gnbNodes->Get(0)->GetObject<MobilityModel>();
        Vector NBposition = modelb->GetPosition ();
        Vector UEposition = modelu->GetPosition ();
        Vector velocity = modelu->GetVelocity ();
        double distance = modelu->GetDistanceFrom (modelb);


        // Calculating new speed
        double speed = sqrt(pow(velocity.x, 2) + pow(velocity.y, 2));
        double d = sqrt(pow(NBposition.x - UEposition.x, 2) + pow(NBposition.y - UEposition.y, 2));
        double Vx = ((NBposition.y - UEposition.y) / d) * speed;
        double Vy = -((NBposition.x - UEposition.x) / d) * speed;
        // double newspeed = sqrt(pow(Vx, 2) + pow(Vy, 2));

        modele->SetVelocity(Vector(Vx, Vy,  0)); 


        
    }
    tic=toc;

    Simulator::Schedule(MilliSeconds(100), &ChangeVelocity, ueNodes, gnbNodes);
}

/**
 * Adds Normal Distributed Noise to the specified Physical layer (it is assumed that corresponds to the UE)
 * The function is implemented to be called at the same time as `DlDataSinrCallback`, this implies that unused arguments 
 * had to be added so it would function.
 * 
 * \example  Config::ConnectWithoutContext("/NodeList/.../DeviceList/.../ComponentCarrierMapUe/.../NrUePhy/DlDataSinr",
 *                  MakeBoundCallback(&AddRandomNoise, uePhy))
 * 
*/
static void AddRandomNoise(Ptr<NrPhy> ue_phy)
{
    Ptr<NormalRandomVariable> awgn = CreateObject<NormalRandomVariable>();
    awgn->SetAttribute("Mean", DoubleValue(NOISE_MEAN));
    awgn->SetAttribute("Variance", DoubleValue(NOISE_VAR));
    awgn->SetAttribute("Bound", DoubleValue(NOISE_BOUND));

    ue_phy->SetNoiseFigure(awgn->GetValue());

    /*
    Simulator::Schedule(MilliSeconds(1000),
                        &NrPhy::SetNoiseFigure,
                        ue_phy,
                        awgn->GetValue()); // Default ns3 noise: 5 dB
    */
}

/**
 * Prints info about the nodes, including:
 *  - SystemID (it assumed that 32uint_t <=> 4 chars)
 *  - NodeID
 *  - NetDevices ID
 *  - Address of each NetDevice
 * 
 * \param ignore_localh     if it ignores the LocalHost Adresses
*/
static void PrintNodeAddressInfo(bool ignore_localh)
{
    std::clog << "Debug info" << std::endl;
    if (ignore_localh) 
    {
        std::clog << "\tLocalhosts addresses were excluded." << std::endl;
    }

    for (uint32_t u = 0; u < NodeList::GetNNodes(); ++u) 
    {   
        Ptr<Node> node = NodeList::GetNode(u);
        uint32_t id = node->GetId();
        uint32_t sysid = node->GetSystemId();
        Ptr<Ipv4> node_ip = node->GetObject<Ipv4>();
        uint32_t ieN = node_ip->GetNInterfaces();  // interface number

        uint32_t a = (uint8_t)ignore_localh;     // Asumes that the 1st interface is localhost
        for (; a < ieN; ++a)
        {   
            uint32_t num_address = node_ip->GetNAddresses(a);

            for (uint32_t b = 0; b < num_address; b++)
            {
                Ipv4Address IeAddres = node_ip->GetAddress(a, b).GetAddress();
                std::clog << "\t " << (uint8_t)sysid << (uint8_t)(sysid >> 8) << (uint8_t)(sysid >> 16) << (uint8_t)(sysid >> 24)
                          << " id: " << id << " netdevice: " << +a << " addr: " << IeAddres << std::endl;
            }

            
        }
        
    }
}

static void processFlowMonitor(Ptr<FlowMonitor> monitor, Ptr<ns3::FlowClassifier> flowClassifier, double AppStartTime)
{
    std::cout << "FLOW MONITOR: VOY A ESCRIBIR :) " << Simulator::Now ().GetSeconds ()  <<std::endl;
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowClassifier);
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;

    std::ofstream outFile;
    std::string filename = "FlowOutput.txt";
    outFile.open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!outFile.is_open())
    {
        std::cerr << "Can't open file " << filename << std::endl;
        return;
    }

    outFile.setf(std::ios_base::fixed);

    for (auto i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

        std::stringstream protoStream;
        protoStream << (uint16_t)t.protocol;
        if (t.protocol == 6)
        {
            protoStream.str("TCP");
        }
        if (t.protocol == 17)
        {
            protoStream.str("UDP");
        }

        outFile << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> "
                << t.destinationAddress << ":" << t.destinationPort << ") proto "
                << protoStream.str() << "\n";

        // Obtener el rxBytes actual
        uint64_t rxBytesCurrent = i->second.rxBytes;

        // Obtener el rxBytes anterior para este FlowId
        uint64_t rxBytesOld = rxBytesOldMap[i->first];

        // Calcular instantThroughput
        double interval = 0.5; // Intervalo entre llamadas a esta función (en segundos)
        double instantThroughput = (rxBytesCurrent - rxBytesOld) * 8.0 / interval / 1000 / 1000; // Mbps

        // Actualizar el valor antiguo
        rxBytesOldMap[i->first] = rxBytesCurrent;

        outFile << "  Rx Bytes:   " << rxBytesCurrent << "\n";
        outFile << "  instantThroughput: " << instantThroughput << " Mbps\n";

        if (i->second.rxPackets > 0)
        {
            averageFlowThroughput += instantThroughput;
            averageFlowDelay += 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets;

            outFile << "  Mean delay:  "
                    << 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets << " ms\n";
            outFile << "  Mean jitter:  "
                    << 1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets << " ms\n";
        }
        else
        {
            outFile << "  Throughput:  0 Mbps\n";
            outFile << "  Mean delay:  0 ms\n";
            outFile << "  Mean jitter: 0 ms\n";
        }
        outFile << "  Rx Packets: " << i->second.rxPackets << "\n";
        outFile << "  Lost Packets: " <<  (i->second.txPackets - i->second.rxPackets) << "\n";
    }

    outFile << "\n\n  Mean flow throughput: " << averageFlowThroughput / stats.size() << "\n";
    outFile << "  Mean flow delay: " << averageFlowDelay / stats.size() << "\n";
    outFile.close();
    Simulator::Schedule(Seconds(0.5), &ProcessFlowMonitorCallback);
}
