#ifndef STREAMING_RECEIVER_H
#define STREAMING_RECEIVER_H

#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"

using namespace ns3;

class StreamingReceiverApp : public Application {
public:
    StreamingReceiverApp();
    virtual ~StreamingReceiverApp();

    void SetAddress(Ipv4Address multicastGroup, uint16_t port);
    void SetInterfaceIndex(uint32_t interfaceIndex);

private:
    virtual void StartApplication();
    virtual void StopApplication();
    
    void HandleRead(Ptr<Socket> socket);

    Ptr<Socket> m_socket;
    Ipv4Address m_multicastGroup;
    uint16_t m_port;
    uint32_t m_interfaceIndex;
    uint32_t m_packetsReceived;
};

#endif // STREAMING_RECEIVER_H