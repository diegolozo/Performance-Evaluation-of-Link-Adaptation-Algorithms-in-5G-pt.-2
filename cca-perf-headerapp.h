#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

class MyHeader : public Header
{
    public:
        // Default constructor
        MyHeader () 
            : m_frameNumber(0), 
            m_frameLength(0)
        {

        }
        // Parameterized constructor
        MyHeader (uint32_t frameNumber, uint32_t frameLength)
            : m_frameNumber(frameNumber),
            m_frameLength(frameLength)
        {

        }

        // Setters for frame number and frame length
        void SetFrameNumber (uint32_t frameNumber)
        {
            m_frameNumber = frameNumber;
        }
        void SetFrameLength (uint32_t frameLength)
        {
            m_frameLength = frameLength;
        }

        // Getters for frame number and frame length
        uint32_t GetFrameNumber () const 
        {
            return m_frameNumber;
        }
        uint32_t GetFrameLength () const
        { 
            return m_frameLength;
        }

        // Required methods to implement for ns3::Header
        virtual uint32_t GetSerializedSize () const override
        {
            return sizeof(uint32_t) + sizeof(uint32_t);  // 4 bytes for frame number, 4 bytes for frame length
        }

        // Serialize the frame number and frame length into a buffer
        virtual void Serialize (Buffer::Iterator start) const override
        {
            start.WriteHtonU32 (m_frameNumber);   // Write the frame number in network byte order
            start.WriteHtonU32 (m_frameLength);   // Write the frame length in network byte order
        }

        // Deserialize the frame number and frame length from the buffer
        virtual uint32_t Deserialize (Buffer::Iterator start) override
        {
            m_frameNumber = start.ReadNtohU32 ();   // Read the frame number in network byte order
            m_frameLength = start.ReadNtohU32 ();   // Read the frame length in network byte order
            return GetSerializedSize ();  // Return the number of bytes read
        }

        // Print method for debugging/logging purposes
        virtual void Print (std::ostream &os) const override
        {
            os << "FrameNumber=" << m_frameNumber << ", FrameLength=" << m_frameLength;
        }

        // Required type ID for ns-3 object system
        static TypeId GetTypeId (void)
        {
            static TypeId tid = TypeId ("MyHeader")
                    .SetParent<Header> ()
                    .AddConstructor<MyHeader> ();
            return tid;
        }

        // GetInstanceTypeId for ns-3 header system
        virtual TypeId GetInstanceTypeId () const override
        {
            return GetTypeId ();
        }
    private:
        uint32_t m_frameNumber;  // Frame number (4 bytes)
        uint32_t m_frameLength;  // Frame length (4 bytes)
};
