#include "streaming-receiver.h"
#include "ns3/internet-module.h"
#include <ns3/ipv4-address.h>

NS_LOG_COMPONENT_DEFINE("StreamingReceiverApp");

StreamingReceiverApp::StreamingReceiverApp()
    : m_socket(nullptr),
      m_interfaceIndex(0),
      m_packetsReceived(0) {
}

StreamingReceiverApp::~StreamingReceiverApp() {
    m_socket = nullptr;
}

void StreamingReceiverApp::SetAddress(Ipv4Address multicastGroup, uint16_t port) {
    m_multicastGroup = multicastGroup;
    m_port = port;
}

void StreamingReceiverApp::SetInterfaceIndex(uint32_t interfaceIndex) {
    m_interfaceIndex = interfaceIndex;
}

void StreamingReceiverApp::StartApplication() {
    NS_LOG_INFO("StreamingReceiverApp starting on node " << GetNode()->GetId() << ", joining multicast group " << m_multicastGroup << " on interface " << m_interfaceIndex);
    if (!m_socket) {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        if (m_socket->Bind(local) == -1) {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        
        // IGMP Join Group
        Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket>(m_socket);
        if (udpSocket) {
            uint32_t interfaceIndex = m_interfaceIndex;
            udpSocket->MulticastJoinGroup(interfaceIndex, Address(m_multicastGroup));
        } else {
            NS_FATAL_ERROR("Failed to cast socket to UdpSocket");
        }
    }

    m_socket->SetRecvCallback(MakeCallback(&StreamingReceiverApp::HandleRead, this));
}

void StreamingReceiverApp::StopApplication() {
    if (m_socket) {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_socket = nullptr;
    }
    
    NS_LOG_INFO("Receiver on node " << GetNode()->GetId() 
              << " | Total Packets Received: " << m_packetsReceived);
}

void StreamingReceiverApp::HandleRead(Ptr<Socket> socket) {
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from))) {
        if (packet->GetSize() > 0) {
            m_packetsReceived++;
            NS_LOG_INFO("Node " << GetNode()->GetId() << ": Received " << packet->GetSize() 
                      << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                      << " | Total: " << m_packetsReceived);
        }
    }
}