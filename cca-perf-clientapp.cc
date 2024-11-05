#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

#include "cmdline-colors.h"
#include "cca-perf-clientapp.h"
#include "cca-perf-headerapp.h"
#include <random>
#include <iomanip>
#include <stdexcept>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MyClientApp");

MyClientApp::MyClientApp ()
  : m_socket (nullptr), // 0
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsRecived(0),
    m_fps(1),
    m_wbtState(DEFAULT),
    m_embededRemaining(0),
    m_transmitInBatches(true),
    m_sentSize(1)
{
    NS_LOG_FUNCTION (this);
}


MyClientApp::~MyClientApp ()
{
    m_socket = nullptr; // 0
}

TypeId
MyClientApp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MyClientApp")
            .SetParent<Application>()
            .SetGroupName("MyAppCCA")
            .AddAttribute("DataRate",
                          "DataRate of the app",
                          DataRateValue(0),
                          MakeDataRateAccessor(&MyClientApp::m_dataRate),
                          MakeDataRateChecker());
    return tid;
}

void
MyClientApp::SetAttribute(std::string name, const AttributeValue &value)
{
    m_factory.Set (name, value);
}

void
MyClientApp::Setup (Ptr<Socket> clientSocket, Ptr<Node> serverNode, uint16_t serverPort)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO ( ">> MyClientApp::Setup" );
    uint32_t server_id = serverNode->GetId();
    Ptr<Ipv4> node_ipv4 = serverNode->GetObject<Ipv4>();
    uint32_t ieNv4 = node_ipv4->GetNInterfaces();
    Ipv4Address IeAddresv4 = node_ipv4->GetAddress(1, 0).GetAddress();
    NS_LOG_INFO ( ">> MyClientApp::Setup"
              << "\t server_id: " << server_id
              << "\t interfaces: " << ieNv4 
              << "\t ip: " << IeAddresv4 );
    m_serverAddress = serverNode->GetObject<Ipv4>()->GetAddress(1, 0).GetAddress();
    m_serverPort = serverPort;
    m_serverID = serverNode->GetId();
    m_socket = clientSocket;

    // Get the local address of the socket
    Address localAddr;
    clientSocket->GetSockName(localAddr);

    // Convert to InetSocketAddress to extract IP and port
    InetSocketAddress inetAddress = InetSocketAddress::ConvertFrom(localAddr);
    Ipv4Address ipAddress = inetAddress.GetIpv4();
    uint16_t port = inetAddress.GetPort();

    // Print the IP address and port
    NS_LOG_INFO ( "Client IP Address: " << ipAddress << ", Port: " << port );
    

}

void 
MyClientApp::StartApplication (void)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO ( ">> MyClientApp::StartApplication" );
    m_running = true;
    m_packetsRecived = 0;
    m_lastJitter = 0;

    Ptr<Node> clientNode = GetNode();  // Get the client node
    uint32_t clientNodeId = clientNode->GetId();  // Get the client NodeId
    m_clientID = clientNodeId;
  
    InitializeLogFiles ();
    m_socket->Bind ();
    m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_serverAddress), m_serverPort));

    // Get the local address of the socket
    Address localAddr;
    m_socket->GetSockName(localAddr);

    // Convert to InetSocketAddress to extract IP and port
    InetSocketAddress inetAddress = InetSocketAddress::ConvertFrom(localAddr);
    Ipv4Address ipAddress = inetAddress.GetIpv4();
    m_peer = inetAddress;
    uint16_t port = inetAddress.GetPort();

    // Print the IP address and port
    NS_LOG_INFO ( "Client IP Address: " << ipAddress << ", Port: " << port );

    m_socket->SetRecvCallback (MakeCallback (&MyClientApp::HandleRead, this));
    Simulator::Schedule (MilliSeconds (1.0), &MyClientApp::Send, this);
}

void 
MyClientApp::StopApplication (void)
{
    NS_LOG_FUNCTION (this);
    NS_LOG_INFO ( ">> MyClientApp::StopApplication" );

    if (m_socket != nullptr)
    {
        m_socket->Close ();
        m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
        m_socket = nullptr;
    }

    CloseLogFiles();
}

void
MyClientApp::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  uint8_t dataBuffer[10];
  sprintf((char *) dataBuffer, "%hu", 0); // default 0 
  Ptr<Packet> firstPacket = Create<Packet> (dataBuffer, 10);
  m_socket->Send (firstPacket);

  if (Ipv4Address::IsMatchingType (m_serverAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent 10 bytes to " <<
                  Ipv4Address::ConvertFrom (m_serverAddress) << " port " << m_serverPort);
  }
  else if (Ipv6Address::IsMatchingType (m_serverAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent 10 bytes to " <<
                  Ipv6Address::ConvertFrom (m_serverAddress) << " port " << m_serverPort);
  }
  else if (InetSocketAddress::IsMatchingType (m_serverAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent 10 bytes to " <<
                  InetSocketAddress::ConvertFrom (m_serverAddress).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_serverAddress).GetPort ());
  }
  else if (Inet6SocketAddress::IsMatchingType (m_serverAddress))
  {
    NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent 10 bytes to " <<
                  Inet6SocketAddress::ConvertFrom (m_serverAddress).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (m_serverAddress).GetPort ());
  }
}



void
MyClientApp::HandleRead (Ptr<Socket> socket)
{
    NS_LOG_FUNCTION (this << socket);
    NS_LOG_INFO( ">> MyClientApp::HandleRead" );
    Ptr<Packet> packet;

    uint32_t packetSize;
    while ( (packet = socket->Recv ()) )
    {
        Ptr<Packet> copiedPacket = packet->Copy ();
        // Create a new header to receive the extracted data

        NS_LOG_INFO( "Packet size before remove header: " << copiedPacket->GetSize() );

        MyHeader header;
        copiedPacket->RemoveHeader (header);  // Remove and extract the header
        NS_LOG_INFO( "Packet size after remove header: " << copiedPacket->GetSize() );

        // Log the received header values
        NS_LOG_INFO( "At time " << Simulator::Now ().GetSeconds ()
                  << " Received header with frame number: " << header.GetFrameNumber ()
                  << " and frame length: " << header.GetFrameLength () );


        MyClientApp::RxRecord newFrame(header.GetFrameNumber (), header.GetFrameLength (), copiedPacket->GetSize());
        addOrUpdateRecord(newFrame);
        // for debug propouse
        // printFramesArrived();

        auto readyFrame = MyClientApp::getAndRemoveRecord();
        
        if (readyFrame){
            NS_LOG_INFO( "At time " << Simulator::Now ().GetSeconds ()
                  << " Frame number: " << header.GetFrameNumber ()
                  << " is Ready");

            clientLog(*readyFrame);
        }

    }
}


void
MyClientApp::InitializeLogFiles ()
{
    NS_LOG_FUNCTION (this);
    NS_LOG_INFO( "Initialize Log Files" );
    NS_LOG_INFO( ">> MyClientApp::InitializeLogFiles" );
    std::string dLog = "app_client_log_" + std::to_string(m_serverID) + "_" + std::to_string(m_clientID) +".txt";

    clientLogFile.open (dLog.c_str ());
    clientLogFile << "Time"
                << "\t" << "UE"
                << "\t" << "FrameNumber"
                << "\t" << "FrameSize"
                << std::endl;
    clientLogFile.flush ();

}

void
MyClientApp::CloseLogFiles ()
{
    NS_LOG_FUNCTION (this);
    NS_LOG_INFO( ">> MyClientApp::CloseLogFiles" );
    NS_LOG_INFO( "Closing Log Files" );
    clientLogFile.close ();

}

void
MyClientApp::clientLog(const MyClientApp::RxRecord& newFrame)
{
    NS_LOG_FUNCTION (this);

    clientLogFile << Simulator::Now().GetSeconds() 
                // << "\t" << InetSocketAddress::ConvertFrom(m_peer).GetIpv4()
                << "\t" << static_cast<uint32_t>( InetSocketAddress::ConvertFrom(m_peer).GetIpv4().Get() & 0xFF) - 1
                << "\t" << newFrame.m_frameNumber
                << "\t" << newFrame.m_frameLength
                << "\t" << std::endl;
    clientLogFile.flush ();
}


// Function to print the list for testing purposes
void MyClientApp::printFramesArrived()
{
    for (const auto& record : m_rxFrames)
    {
        std::cout<< "Frame Number: " << record.m_frameNumber
                  << ", Frame Length: " << record.m_frameLength
                  << ", Bytes Arrived: " << record.m_bytesArrived << std::endl;
    }
}

// Function to add a record if it doesn't exist, or add bytes if it does exist
void MyClientApp::addOrUpdateRecord(const MyClientApp::RxRecord& newRecord)
{
    // Check if a record with the same frame number already exists
    auto it = std::find_if(m_rxFrames.begin(), m_rxFrames.end(), [newRecord](const MyClientApp::RxRecord& record) {
        return record.m_frameNumber == newRecord.m_frameNumber;
    });

    // If the record exists, add the bytes
    if (it != m_rxFrames.end())
    {
        it->AddBytesArrived(newRecord.m_bytesArrived);
        NS_LOG_INFO( "Added " << newRecord.m_bytesArrived << " bytes to existing record with Frame Number: " << newRecord.m_frameNumber );
    }
    // If the record doesn't exist, add a new record
    else
    {
        m_rxFrames.emplace_back(newRecord.m_frameNumber, newRecord.m_frameLength, newRecord.m_bytesArrived);
        NS_LOG_INFO( "Added new record with Frame Number: " << newRecord.m_frameNumber );
    }
}

// Function to get and remove a record if m_frameLength == m_bytesArrived
std::optional<MyClientApp::RxRecord> MyClientApp::getAndRemoveRecord()
{
    // Find the record where m_frameLength equals m_bytesArrived
    auto it = std::find_if(m_rxFrames.begin(), m_rxFrames.end(), [](const RxRecord& record) {
        return record.m_frameLength == record.m_bytesArrived;
    });

    // If such a record is found, remove and return it
    if (it != m_rxFrames.end())
    {
        RxRecord foundRecord = *it;  // Copy the found record
        m_rxFrames.erase(it);        // Remove the record from the list
        NS_LOG_INFO( "Removed record with Frame Number: " << foundRecord.m_frameNumber );
        return foundRecord;          // Return the found record
    }
    else
    {
        NS_LOG_INFO( "No record found where m_frameLength equals m_bytesArrived." );
        return std::nullopt;         // Return an empty optional
    }
}
