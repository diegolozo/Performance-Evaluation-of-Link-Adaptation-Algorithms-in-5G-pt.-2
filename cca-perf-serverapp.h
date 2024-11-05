#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

class MyServerApp : public Application
{
    public:
        MyServerApp ();
        ~MyServerApp() override;
        static TypeId GetTypeId();
        void ChangeDataRate (DataRate rate);
        void Setup (Ptr<Socket> socket, Address address, uint16_t serverPort, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, std::string TType, bool avoidFragmentation);
        /**
         * @brief Record an attribute to be set in each application after it is created.
         * 
         * @param name the name of the attribute to set
         * @param value the value of the attribute to set
         */
        void SetAttribute (std::string name, const AttributeValue &value);
            /**
         * \ingroup tcpStream
         * \brief data strucute the server uses to manage the following data for every client separately.
         */
        struct callbackData
        {
            uint32_t currentTxBytes;//!< already sent bytes for this particular segment, set to 0 if sent bytes == packetSizeToReturn, so transmission for this segment is over
            uint32_t packetSizeToReturn;//!< total amount of bytes that have to be returned to the client
            bool send;//!< true as long as there are still bytes left to be sent for the current segment
        };
        
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

        void HandleAccept (Ptr<Socket> s, const Address& from);
        void HandleRead (Ptr<Socket> socket);


        void ScheduleTx(Ptr<Socket> socket);
        void SendPacket(Ptr<Socket> socket);

        void InitializeLogFiles();
        void CloseLogFiles();
        void serverLog();
        std::ofstream serverLogFile; //!< Output stream for logging server information


        Ptr<Socket>     m_socket;
        Ipv4Address     m_serverAddress; //!< Remote peer address
        uint16_t        m_serverPort; //!< Remote peer port
        uint16_t        m_serverID;
        uint16_t        m_clientID;
        Address         m_peer;
        uint32_t        m_packetSize;
        uint32_t        m_nPackets;
        DataRate        m_dataRate;
        EventId         m_sendEvent;
        bool            m_running;
        uint32_t        m_packetsSent;
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

        std::map <Address, callbackData> m_callbackData; //!< With this it is possible to access the currentTxBytes, the packetSizeToReturn and the send boolean through the from value of the client.
        std::vector<Address> m_connectedClients; //!< Vector which holds the list of currently connected clients.

};



