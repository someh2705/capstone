#include "basic-amt.h"

#include "setup.h"

#include <ns3/application.h>
#include <ns3/boolean.h>
#include <ns3/callback.h>
#include <ns3/fatal-error.h>
#include <ns3/inet-socket-address.h>
#include <ns3/ipv4-address.h>
#include <ns3/ipv4-header.h>
#include <ns3/ptr.h>
#include <ns3/simulator.h>
#include <ns3/socket.h>
#include <ns3/type-id.h>
#include <ns3/udp-header.h>
#include <ns3/udp-socket.h>
#include <ns3/uinteger.h>

#include <cstdint>

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED(RelayApp);
NS_OBJECT_ENSURE_REGISTERED(GatewayApp);

TypeId
RelayApp::GetTypeId()
{
    static TypeId id = TypeId("RelayApp")
                           .SetParent<Application>()
                           .SetGroupName("Applications")
                           .AddConstructor<RelayApp>();
    return id;
}

RelayApp::RelayApp()
    : m_recvSocket(nullptr),
      m_sendSocket(nullptr),
      m_gatewayPort(0),
      m_multicastPort(0)
{
}

void
RelayApp::Setup(Ipv4Address gatewayAddress,
                uint16_t gatewayPort,
                Ipv4Address multicastGroup,
                uint16_t multicastPort)
{
    m_gatewayAddress = gatewayAddress;
    m_gatewayPort = gatewayPort;
    m_multicastGroup = multicastGroup;
    m_multicastPort = multicastPort;
}

RelayApp::~RelayApp()
{
}

void
RelayApp::DoDispose()
{
    m_recvSocket = nullptr;
    m_sendSocket = nullptr;
    Application::DoDispose();
}

void
RelayApp::StartApplication()
{
    if (!m_recvSocket)
    {
        auto id = TypeId::LookupByName("ns3::Ipv4RawSocketFactory");
        m_recvSocket = Socket::CreateSocket(GetNode(), id);
        auto local = InetSocketAddress(Ipv4Address::GetAny(), 0);
        if (m_recvSocket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind multicast socket");
        }
        m_recvSocket->SetAttribute("Protocol", UintegerValue(17));
    }
    m_recvSocket->SetRecvCallback(MakeCallback(&RelayApp::HandleRead, this));

    if (!m_sendSocket)
    {
        auto id = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_sendSocket = Socket::CreateSocket(GetNode(), id);
    }
}

void
RelayApp::StopApplication()
{
    if (m_recvSocket)
    {
        m_recvSocket->Close();
        m_recvSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    if (m_sendSocket)
    {
        m_sendSocket->Close();
    }
}

void
RelayApp::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        Ipv4Header ipv4Header;
        packet->PeekHeader(ipv4Header);
        if (ipv4Header.GetDestination() == m_multicastGroup)
        {
            uint8_t header[4] = {0x06, 0x00, 0x00, 0x00};
            auto encapsulatedPacket = Create<Packet>(header, 4);
            encapsulatedPacket->AddAtEnd(packet);

            m_sendSocket->SendTo(encapsulatedPacket,
                                 0,
                                 InetSocketAddress(m_gatewayAddress, m_gatewayPort));
        }
    }
}

TypeId
GatewayApp::GetTypeId()
{
    static TypeId tid = TypeId("GatewayApp")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<GatewayApp>();
    return tid;
}

GatewayApp::GatewayApp()
    : m_recvSocket(nullptr),
      m_sendSocket(nullptr)
{
}

GatewayApp::~GatewayApp()
{
}

void
GatewayApp::DoDispose()
{
    m_recvSocket = nullptr;
    m_sendSocket = nullptr;
    Application::DoDispose();
}

void
GatewayApp::Setup(uint16_t unicastPort)
{
    m_unicastPort = unicastPort;
}

void
GatewayApp::StartApplication()
{
    if (!m_recvSocket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_recvSocket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_unicastPort);
        if (m_recvSocket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind unicast socket");
        }
    }
    m_recvSocket->SetRecvCallback(MakeCallback(&GatewayApp::HandleRead, this));

    if (!m_sendSocket)
    {
        TypeId id = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_sendSocket = Socket::CreateSocket(GetNode(), id);
    }
}

void
GatewayApp::StopApplication()
{
    if (m_recvSocket)
    {
        m_recvSocket->Close();
        m_recvSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    if (m_sendSocket)
    {
        m_sendSocket->Close();
    }
}

void
GatewayApp::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        packet->RemoveAtStart(4);

        Ipv4Header ipv4Header;
        packet->RemoveHeader(ipv4Header);

        UdpHeader udpHeader;
        packet->RemoveHeader(udpHeader);

        Ipv4Address multicastGroup = ipv4Header.GetDestination();
        uint16_t multicastPort = udpHeader.GetDestinationPort();

        m_sendSocket->SendTo(packet, 0, InetSocketAddress(multicastGroup, multicastPort));
    }
}

BasicAmt::BasicAmt(int argc, char* argv[])
{
    using namespace ns3;
    std::string filename{"../resources/basic-amt.yaml"};

    std::cout << "topology setup: " << filename << std::endl;
    Topology topology(filename);

    Simulator::Stop(Seconds(21.0));
    Simulator::Run();
    Simulator::Destroy();
}
