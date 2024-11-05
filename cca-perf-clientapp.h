#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

class MyClientApp : public Application
{
    public:
        MyClientApp ();
        ~MyClientApp() override;
        static TypeId GetTypeId();
        void ChangeDataRate (DataRate rate);
        void Setup (Ptr<Socket> clientSocket, Ptr<Node> serverNode, uint16_t serverPort);
        /**
         * @brief Record an attribute to be set in each application after it is created.
         * 
         * @param name the name of the attribute to set
         * @param value the value of the attribute to set
         */
        void SetAttribute (std::string name, const AttributeValue &value);
    
    private:
        ObjectFactory m_factory;
        enum wbt_state {
            DEFAULT,
            SENT_WEBPAGE,
            SENT_ALL_EMBEDDED,
            EMBEDDED_REMAINING
        };

        void StartApplication() override;
        void StopApplication() override;

        void HandleRead (Ptr<Socket> socket);
        void Send (void);

        void ScheduleTx();
        void SendPacket();

        void InitializeLogFiles();
        void CloseLogFiles();
        std::ofstream clientLogFile; //!< Output stream for logging client information

        Ptr<Socket>     m_socket;
        Ipv4Address     m_serverAddress; //!< Remote peer address
        uint16_t        m_serverPort; //!< Remote peer port
        uint32_t        m_clientID ;
        uint32_t        m_serverID ;
        Address         m_peer;
        uint32_t        m_packetSize;
        uint32_t        m_nPackets;
        DataRate        m_dataRate;
        EventId         m_sendEvent;
        bool            m_running;
        uint32_t        m_packetsRecived;
        double          m_lastJitter;
        std::string     m_ttype;
        uint32_t        m_fps;
        Ptr<NormalRandomVariable> m_frameSize;
        double          m_frameSize_min;
        double          m_frameSize_max;
        Ptr<NormalRandomVariable> m_jitter;
        double          m_jitter_min;
        double          m_jitter_max;
        Ptr<ExponentialRandomVariable> m_pageStayTime;
        Ptr<ExponentialRandomVariable> m_parsingTime;
        Ptr<NormalRandomVariable>      m_embeddedObjectSize;
        Ptr<ParetoRandomVariable>      m_numberEmbeddedObjects;
        wbt_state                      m_wbtState;
        uint32_t                       m_embededRemaining;
        bool                           m_transmitInBatches;
        uint32_t                       m_sentSize;

        struct RxRecord
        {
            RxRecord(const uint32_t& frameNumber, const uint32_t& frameLength, const uint32_t& bytesArrived)
                : m_frameNumber(frameNumber),
                m_frameLength(frameLength),
                m_bytesArrived(bytesArrived)

            {
            }

            RxRecord() = delete;

            // Method to modify m_bytesArrived
            void SetBytesArrived(const uint32_t& bytesArrived)
            {
                m_bytesArrived = bytesArrived;
            }

            // Method to add bytes to m_bytesArrived
            void AddBytesArrived(const uint32_t& bytesArrived)
            {
                m_bytesArrived += bytesArrived;
            }

            uint32_t m_frameNumber;
            uint32_t m_frameLength;
            uint32_t m_bytesArrived;
        };
        std::list<RxRecord> m_rxFrames;              ///< Reception Frames

        void printFramesArrived();
        void addOrUpdateRecord(const MyClientApp::RxRecord& newRecord);
        std::optional<MyClientApp::RxRecord> getAndRemoveRecord();

        void clientLog(const MyClientApp::RxRecord& newFrame);

};