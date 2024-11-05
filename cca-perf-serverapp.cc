#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

#include "cmdline-colors.h"
#include "cca-perf-serverapp.h"
#include "cca-perf-headerapp.h"
#include <random>
#include <iomanip>
#include <stdexcept>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MyServerApp");

MyServerApp::MyServerApp ()
  : m_socket (nullptr), // 0
    m_serverAddress(),
    m_serverPort(0),
    m_serverID(0),
    m_clientID(0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent(0),
    m_fps(1),
    m_wbtState(DEFAULT),
    m_embededRemaining(0),
    m_transmitInBatches(true),
    m_sentSize(1)
{
    NS_LOG_FUNCTION (this);
}


MyServerApp::~MyServerApp ()
{
    m_socket = nullptr; // 0
}

TypeId
MyServerApp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MyServerApp")
            .SetParent<Application>()
            .SetGroupName("MyAppCCA")
            .AddAttribute ("DataRate",
                          "DataRate of the app",
                          DataRateValue(0),
                          MakeDataRateAccessor(&MyServerApp::m_dataRate),
                          MakeDataRateChecker())
            .AddAttribute ("Port",
                           "Port on which we listen for incoming packets.",
                           UintegerValue (8000),
                           MakeUintegerAccessor (&MyServerApp::m_serverPort),
                           MakeUintegerChecker<uint16_t> ())
                   
    ;
    return tid;
}

void
MyServerApp::SetAttribute(std::string name, const AttributeValue &value)
{
    m_factory.Set (name, value);
}

void
MyServerApp::Setup (Ptr<Socket> socket, Address address, uint16_t serverPort, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, std::string TType, bool transmitInBatches)
{
    NS_LOG_FUNCTION(this << socket << address << packetSize << nPackets << dataRate << TType);
    m_socket = socket;
    m_peer = address;
    MyHeader header;
    m_packetSize = packetSize - header.GetSerializedSize();
    m_nPackets = nPackets;
    m_dataRate = dataRate;
    m_transmitInBatches = transmitInBatches;
    m_serverPort = serverPort;

    if (TType == "VBR1"){
        m_fps = 60.0;
        m_ttype = "VBR";
    } else if (TType == "VBR2") {
        m_fps = 120.0;
        m_ttype = "VBR";
    } else if (TType == "VBR3") {
        m_fps = 30.0;
        m_ttype = "VBR";
    } else if (TType == "CBR") {
        m_ttype = "CBR";
    } else if (TType == "WBT") {
        m_ttype = "WBT";
    } else {
        throw std::invalid_argument("Received a non-valid bitrate type. Check documentation.");
    }

  if (m_ttype == "VBR") {
    // 3GPP TSG RAN W61 #104-e
    // FrameSize Truncated Gaussian mean: datarate/fps/8, std: 15%mean, range: [67,1.5*mean] bytes
    m_frameSize = CreateObject<NormalRandomVariable> ();
    m_frameSize->SetAttribute ("Mean", DoubleValue ((static_cast<double> (m_dataRate.GetBitRate ()))/ (double) m_fps / 8.0));
    m_frameSize->SetAttribute ("Variance", DoubleValue ((static_cast<double> (m_dataRate.GetBitRate ()))/ (double) m_fps / 8.0 * 0.15)); 
    m_frameSize_min = 67;
    m_frameSize_max = 1.5 *  (static_cast<double> (m_dataRate.GetBitRate ()))/ (double) m_fps / 8.0;

    // Jitter. Truncated Gaussian mean: zero, std: 2ms, range: [-4,4] ms
    m_jitter = CreateObject<NormalRandomVariable> ();
    m_jitter->SetAttribute ("Mean", DoubleValue (0));
    m_jitter->SetAttribute ("Variance", DoubleValue (2));
    m_jitter_min = -4;
    m_jitter_max = 4;

  } else if (m_ttype == "WBT") {
    // Size of page
    m_frameSize = CreateObject<NormalRandomVariable> ();
    m_frameSize->SetAttribute("Mean", DoubleValue(10710));
    m_frameSize->SetAttribute("Variance", DoubleValue(25032));
    m_frameSize_min = 1;

    // Web browser parsing time
    m_parsingTime = CreateObject<ExponentialRandomVariable>();
    m_parsingTime->SetAttribute("Mean", DoubleValue(0.5)); // 0.5 seconds
    m_parsingTime->SetAttribute("Bound", DoubleValue(3));

    // Web page read time
    m_pageStayTime = CreateObject<ExponentialRandomVariable>();
    m_pageStayTime->SetAttribute("Mean", DoubleValue(5)); // 5 seconds
    m_pageStayTime->SetAttribute("Bound", DoubleValue(7));

    // Embedded objects' size
    m_embeddedObjectSize = CreateObject<NormalRandomVariable>();
    m_embeddedObjectSize->SetAttribute("Mean", DoubleValue(7758));
    m_embeddedObjectSize->SetAttribute("Variance", DoubleValue(126168));

    // Number of embedded objects to send
    m_numberEmbeddedObjects = CreateObject<ParetoRandomVariable>();
    m_numberEmbeddedObjects->SetAttribute("Shape", DoubleValue(3.1));
    // Location == Scale ; https://pj.freefaculty.org/guides/stat/Distributions/DistributionWriteups/Pareto/Pareto-02.pdf
    m_numberEmbeddedObjects->SetAttribute("Scale", DoubleValue(15));

  }

}


void
MyServerApp::ChangeDataRate (DataRate rate)
{
    NS_LOG_FUNCTION(this << rate);
    m_dataRate = rate;
}

void
MyServerApp::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO(">> MyServerApp::StartApplication" );
    
    m_running = true;
    m_packetsSent = 0;
    m_lastJitter = 0;

    NS_LOG_INFO( "Init Server socket: " << m_socket ); 
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_serverPort);
    m_socket->Bind (local);
    m_socket->Listen ();

    // Declare a variable of type Address to hold the bound address
    ns3::Address boundAddress;

    // Retrieve the local address and port after binding
    m_socket->GetSockName(boundAddress); // Use GetSockName to retrieve the bound address

    // Now cast the boundAddress to InetSocketAddress to extract the IP and port
    InetSocketAddress inetBoundAddress = InetSocketAddress::ConvertFrom(boundAddress);

    Ipv4Address serverAddress = inetBoundAddress.GetIpv4(); // Get the IP address
    uint16_t serverPort = inetBoundAddress.GetPort(); // Get the port number

    // Now you can use serverAddress and serverPort
    NS_LOG_INFO("Server is listening on: " << serverAddress << ":" << serverPort);

    // Now you can use serverAddress and serverPort
    std::cout << "\n--------------------> Server is listening on: " << serverAddress << ":" << serverPort << "\n";
    NS_LOG_INFO("Server is listening on: " << serverAddress << ":" << serverPort);

    Ptr<Node> node = m_socket->GetNode();  // Get the node that owns the socket
    uint32_t nodeId = node->GetId();  // Get the NodeId
    NS_LOG_INFO( "Server Node ID: " << nodeId );
    m_serverID = nodeId;
    
    // Accept connection requests from remote hosts.
    m_socket->SetAcceptCallback (MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                               MakeCallback (&MyServerApp::HandleAccept, this));

    // Receive data requests from remote hosts.
    m_socket->SetRecvCallback (MakeCallback (&MyServerApp::HandleRead, this));

}

void
MyServerApp::StopApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO( ">> MyServerApp::StopApplication" );
    m_running = false;

    if (m_sendEvent.IsRunning ())
    {
    //   std::cout  <<  red << "CANCEL SOCKET" << clear << std::endl;
    Simulator::Cancel (m_sendEvent);
    }

    if (m_socket)
    {
    //   std::cout  <<  red << "CLOSE SOCKET" << clear << std::endl;
    m_socket->Close ();
    }
    
    CloseLogFiles();
    
}

void
MyServerApp::HandleAccept (Ptr<Socket> s, const Address& from)
{
    NS_LOG_FUNCTION (this << s << from);
    NS_LOG_INFO( ">> MyServerApp::HandleAccept" );
    NS_LOG_INFO( "Accept Server socket: " << m_socket ); 
    NS_LOG_INFO( "Accept from peer socket: " << s ); 
    NS_LOG_INFO( "Address: " << from );
    InetSocketAddress inetSocketAddress = InetSocketAddress::ConvertFrom(from);
    Ipv4Address ipv4Addr = inetSocketAddress.GetIpv4(); // Extract the IPv4 address
    uint16_t port = inetSocketAddress.GetPort();         // Extract port
    NS_LOG_INFO( "IPv4 Address: " << ipv4Addr << ", Port: " << port );

    for (uint32_t u = 0; u < NodeList::GetNNodes(); ++u) 
    {
        Ptr<Node> node = NodeList::GetNode(u);
        uint32_t nodeid = node->GetId();
        Ptr<Ipv4> node_ipv4 = node->GetObject<Ipv4>();
        uint32_t ieNv4 = node_ipv4->GetNInterfaces();  // interface number
        for (uint32_t a = 1; a < ieNv4; ++a)
        {   
            uint32_t num_address_v4 = node_ipv4->GetNAddresses(a);

            for (uint32_t b = 0; b < num_address_v4; b++)
            {
                Ipv4Address IeAddresv4 = node_ipv4->GetAddress(a, b).GetAddress();
                if (ipv4Addr == IeAddresv4)
                    m_clientID = nodeid;
                    break;
            }
            if (m_clientID > 0) {
                break;
            }
        }
        if (m_clientID > 0) {
            break;
        }
 
    }

    NS_LOG_INFO( "m_clientID: " << m_clientID << ", Port: " << port );

    InitializeLogFiles ();

    callbackData cbd;
    cbd.currentTxBytes = 0;
    cbd.packetSizeToReturn = 0;
    cbd.send = false;
    m_callbackData [from] = cbd;
    m_connectedClients.push_back (from);
    s->SetRecvCallback (MakeCallback (&MyServerApp::HandleRead, this));


}

void 
MyServerApp::HandleRead (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_INFO( ">> MyServerApp::HandleRead" );
    NS_LOG_INFO( "HandleRead Server socket: " << m_socket ); 
    NS_LOG_INFO( "HandleRead from peer socket: " << socket ); 


  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
  {
    NS_LOG_INFO( "Read While HandleRead!" );
    SendPacket( socket);

    socket->GetSockName (localAddress);
    if (InetSocketAddress::IsMatchingType (from))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s server received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());

    }
  }
}

void
MyServerApp::SendPacket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO ( ">> MyServerApp::SendPacket" );

    uint32_t frameSize = 0;
    
    if (m_ttype == "VBR"){

        frameSize = m_frameSize->GetValue();
        while (frameSize < m_frameSize_min or frameSize > m_frameSize_max){
            // std::cout << "frameSize: " << std::setprecision(10) << frameSize <<  "[ms]\n";
            frameSize = m_frameSize->GetValue();

        }

    }
    else if (m_ttype == "WBT") {
      switch (m_wbtState)
      {
      case DEFAULT:
        m_wbtState = SENT_WEBPAGE;
        // We generate the webpage size
        frameSize = m_frameSize->GetValue();
        while (frameSize < m_frameSize_min){
          // std::cout << "frameSize: " << std::setprecision(10) << frameSize <<  "[ms]\n";
          frameSize = m_frameSize->GetValue();
          
        }
        NS_LOG_DEBUG("[WBT] About to send webpage index of size " << frameSize << " bytes.");
        m_embededRemaining = m_numberEmbeddedObjects->GetValue();
        // We want atleast 1
        while (m_embededRemaining < 1)
        {
          m_embededRemaining = m_numberEmbeddedObjects->GetValue();
        }
        NS_LOG_DEBUG("[WBT] Queued " << m_embededRemaining << " embedded objects to send.");
        break;
      
      case SENT_WEBPAGE:
      case EMBEDDED_REMAINING:
        frameSize = m_embeddedObjectSize->GetValue();
        while (frameSize < 1)
        {
          frameSize = m_embeddedObjectSize->GetValue();
        };

        m_embededRemaining--;
        NS_LOG_DEBUG("[WBT] About to send embedded object of size " << frameSize << " bytes. Number of embedded remaining: " << m_embededRemaining);

        if (m_embededRemaining == 0) {
          m_wbtState = SENT_ALL_EMBEDDED;
          NS_LOG_DEBUG("[WBT] Sent all embedded objects. Next step is to wait for a new website request.");
        } else {
          m_wbtState = EMBEDDED_REMAINING;
        }
        break;
      
      case SENT_ALL_EMBEDDED:
      default:
        NS_LOG_DEBUG("[WBT] You shouldn't be here...");
        m_wbtState = DEFAULT;
        
      }
      
      
    }
    else
    {
      frameSize = m_packetSize;
    }

    m_sentSize = frameSize;

    // std::cout << "socket: " << socket << ", time: " << std::setprecision(10) << (double) Simulator::Now().GetSeconds() << ", frameSize: " << std::setprecision(2) << frameSize << ", m_packetSize: " << std::setprecision(2) << m_packetSize << std::endl;;
    // std::cout << "." << frameSize << "." ; 

    serverLog();

    // to prevent tcp fragmentation
    while (m_transmitInBatches && (frameSize > m_packetSize)) {
        // uint8_t dataBuffer[m_packetSize];
        // sprintf ((char *) dataBuffer, "%u;%u", m_packetsSent, m_sentSize);

        // Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t*>(dataBuffer), m_packetSize);

        // Create a packet with m_packetSize bytes of payload
        Ptr<Packet> packet = Create<Packet>(m_packetSize);
        // Create and set a custom header
        MyHeader header;
        header.SetFrameNumber (m_packetsSent);  // Set the frame number
        header.SetFrameLength (m_sentSize);     // Set the frame length (matching the payload size)

        // std::cout << "Packet size before adding header: " << packet->GetSize() << std::endl;
        // Add the custom header to the packet
        packet->AddHeader (header);

        NS_LOG_DEBUG("txBuff before: " << m_socket->GetTxAvailable());
        NS_LOG_DEBUG("Will send a packet \"fragment\" of size " << +m_packetSize << " bytes at " << Simulator::Now().GetSeconds());
        // NS_LOG_DEBUG("Packet sent " << packet->ToString());
        socket->Send (packet);
        // std::cout << "Packet size after adding header: " << packet->GetSize() << std::endl;


        // Log the sent header values
        NS_LOG_INFO( "At time " << Simulator::Now ().GetSeconds ()
                  << " Send a packet with frame number: " << header.GetFrameNumber ()
                  << " and frame length: " << header.GetFrameLength () );

        frameSize = frameSize - m_packetSize;
    }
    // Create last packet 
    Ptr<Packet> packet = Create<Packet>(frameSize);
    MyHeader header;
    header.SetFrameNumber (m_packetsSent);  // Set the frame number
    header.SetFrameLength (m_sentSize);     // Set the frame length (matching the payload size)

    // Add the custom header to the packet
    packet->AddHeader (header);

    // MyServerAppTag tag (Simulator::Now ()); // comment¿?
    // packet->EnablePrinting();
    NS_LOG_DEBUG("txBuff before: " << m_socket->GetTxAvailable());
    NS_LOG_DEBUG("Will send a packet of size " << +frameSize << " bytes at " << Simulator::Now().GetSeconds());
    // NS_LOG_DEBUG("Packet sent " << packet->ToString());
    // m_socket->Send (packet);
    // m_socket->SendTo (packet, 0, from );
    socket->Send (packet);

    // Log the sent header values
    NS_LOG_INFO( "At time " << Simulator::Now ().GetSeconds ()
                << " Send a packet with frame number: " << header.GetFrameNumber ()
                << " and frame length: " << header.GetFrameLength () );


    if (++m_packetsSent < m_nPackets)
    {
        ScheduleTx ( socket);
    }
    else
    {
        std::cout  <<  TXT_RED << "APP END" << TXT_CLEAR << std::endl;

    }

}


void
MyServerApp::ScheduleTx(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this);
  double min_tNext = 0.00001;
  
  if (m_running){

    double j = 0;
    double t_next;
    if (m_ttype == "VBR"){
      // Jitter. Truncated Gaussian mean: zero, std: 2ms, range: [-4,4] ms
      // 3GPP TSG RAN W61 #104-e

      j = m_jitter->GetValue();
      // std::cout << "j: " << std::setprecision(10) << j <<  "[ms]\n";
      while (j < m_jitter_min or j > m_jitter_max ){
        // std::cout << "j: " << std::setprecision(10) << j <<  "[ms]\n";
        j = m_jitter->GetValue();
      }
      t_next = (double) (1 / (double) m_fps) + (double) ( (double) j / (double) 1000) - (double) ( (double) m_lastJitter / 1000);
    
    } else if (m_ttype == "WBT") {

      switch (m_wbtState)
      {
      
      case SENT_ALL_EMBEDDED:
        t_next = m_parsingTime->GetValue() + m_pageStayTime->GetValue();
        NS_LOG_DEBUG("[WBT] Next website request in " << t_next << " seconds.");
        m_wbtState = DEFAULT;
        break;

      case SENT_WEBPAGE:
      case EMBEDDED_REMAINING:
      default:
        t_next = m_sentSize * 8 / static_cast<double> (m_dataRate.GetBitRate ());
        j=0;
        break;
      }

    } else {
      j = 0;
      t_next = m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ());
    }
    
    // t_next = (double) (1 / (double) m_fps) + (double) ( (double) j / (double) 1000) - (double) ( (double) m_lastJitter / 1000);

    // std::cout << "Peer: " << m_peer << "jitter: " << std::setprecision(10) << j << ", tNext: " << (double) (1 / (double) m_fps) + (double)((double) j / 1000)<< "\n";

    if ( t_next < min_tNext){
      // std::cout << "fps: " << std::setprecision(10) << m_fps ;
      // std::cout << ", f: " << std::setprecision(10) << (double) (1 / (double) m_fps) ;
      // std::cout << ", j: " << std::setprecision(10) << (double) ( (double) j / (double) 1000) ;
      // std::cout << ", m_lastJitter: " << std::setprecision(10) << (double) ( (double) m_lastJitter / 1000) ;
      // std::cout << ", t_next: " << t_next  <<  "\n";

      Time tNext (Seconds (min_tNext));
      m_sendEvent = Simulator::Schedule (tNext, &MyServerApp::SendPacket, this, socket);
    } else {
      Time tNext (Seconds ( t_next));
      m_sendEvent = Simulator::Schedule (tNext, &MyServerApp::SendPacket, this, socket);
    }

    m_lastJitter = j;

    // tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ()) +(double) j / 1000));
    // m_sendEvent = Simulator::Schedule (tNext, &MyServerApp::SendPacket, this);
  }
  else
  {
      std::cout  <<  TXT_RED << "APP STOPPED" << TXT_CLEAR << std::endl;
  }
}

void
MyServerApp::InitializeLogFiles ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO( "Initialize Log Files" );
  std::string dLog = "app_server_log_" + std::to_string(m_serverID) + "_" + std::to_string(m_clientID) +".txt";
  serverLogFile.open (dLog.c_str ());
  serverLogFile << "Time"
              << "\t" << "UE"
              << "\t" << "FrameNumber"
              << "\t" << "FrameSize"
              << "\t" << "PacketNumber"
              << std::endl;
  serverLogFile.flush ();

}

void
MyServerApp::CloseLogFiles ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO( "Closing Log Files" );
  serverLogFile.close ();

}

void
MyServerApp::serverLog()
{
  NS_LOG_FUNCTION (this);
  serverLogFile << Simulator::Now().GetSeconds() 
              // << "\t" << InetSocketAddress::ConvertFrom(m_peer).GetIpv4()
              << "\t" << static_cast<uint32_t>( InetSocketAddress::ConvertFrom(m_peer).GetIpv4().Get() & 0xFF) - 1
              << "\t" << m_packetsSent
              << "\t" << m_sentSize
              << "\t" << m_sentSize / m_packetSize + 1
              << std::endl;
  serverLogFile.flush ();
}
