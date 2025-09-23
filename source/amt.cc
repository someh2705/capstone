#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include <ns3/inet-socket-address.h>
#include <ns3/ipv4-address.h>
#include <ns3/ipv4.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AmtCsmaOnlySimulation");

class RelayApp : public Application
{
  public:
    RelayApp()
        : m_recvSocket(nullptr),
          m_sendSocket(nullptr),
          m_gatewayAddress(Ipv4Address::GetAny(), 0)
    {
    }

    void Setup(Ptr<Socket> recvSocket,
               Ptr<Socket> sendSocket,
               Ipv4Address gatewayAddr,
               uint16_t gatewayPort)
    {
        m_recvSocket = recvSocket;
        m_sendSocket = sendSocket;
        m_gatewayAddress = InetSocketAddress(gatewayAddr, gatewayPort);
    }

  private:
    void StartApplication() override
    {
        m_recvSocket->SetRecvCallback(MakeCallback(&RelayApp::HandleRead, this));
    }

    void StopApplication() override
    {
        if (m_recvSocket)
        {
            m_recvSocket->Close();
        }
        if (m_sendSocket)
        {
            m_sendSocket->Close();
        }
    }

    void HandleRead(Ptr<Socket> socket)
    {
        Ptr<Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom(from)))
        {
            m_sendSocket->SendTo(packet->Copy(), 0, m_gatewayAddress);
        }
    }

    Ptr<Socket> m_recvSocket;
    Ptr<Socket> m_sendSocket;
    InetSocketAddress m_gatewayAddress;
};

class GatewayApp : public Application
{
  public:
    GatewayApp()
        : m_recvSocket(nullptr),
          m_sendSocket(nullptr),
          m_multicastGroup(Ipv4Address::GetAny(), 0)
    {
    }

    void Setup(Ptr<Socket> recvSocket,
               Ptr<Socket> sendSocket,
               Ipv4Address multicastGroup,
               uint16_t multicastPort)
    {
        m_recvSocket = recvSocket;
        m_sendSocket = sendSocket;
        m_multicastGroup = InetSocketAddress(multicastGroup, multicastPort);
    }

  private:
    void StartApplication() override
    {
        m_recvSocket->SetRecvCallback(MakeCallback(&GatewayApp::HandleRead, this));
    }

    void StopApplication() override
    {
        if (m_recvSocket)
        {
            m_recvSocket->Close();
        }
        if (m_sendSocket)
        {
            m_sendSocket->Close();
        }
    }

    void HandleRead(Ptr<Socket> socket)
    {
        Ptr<Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom(from)))
        {
            m_sendSocket->SendTo(packet->Copy(), 0, m_multicastGroup);
        }
    }

    Ptr<Socket> m_recvSocket;
    Ptr<Socket> m_sendSocket;
    InetSocketAddress m_multicastGroup;
};

int
main(int argc, char* argv[])
{
    LogComponentEnable("AmtCsmaOnlySimulation", LOG_LEVEL_INFO);
    Config::SetDefault("ns3::CsmaNetDevice::EncapsulationMode", StringValue("Dix"));

    // --- 1. 노드 생성 ---
    NS_LOG_INFO("Creating nodes for the final topology...");
    NodeContainer nodes;
    nodes.Create(9);
    Ptr<Node> host = nodes.Get(0);
    Ptr<Node> router1 = nodes.Get(1);
    Ptr<Node> client1 = nodes.Get(2);
    Ptr<Node> relay = nodes.Get(3);
    Ptr<Node> router2 = nodes.Get(4);
    Ptr<Node> gateway = nodes.Get(5);
    Ptr<Node> router3 = nodes.Get(6);
    Ptr<Node> client2 = nodes.Get(7);
    Ptr<Node> client3 = nodes.Get(8);

    // --- 2. 링크 생성 및 NetDevice 설치 ---
    NS_LOG_INFO("Creating links and installing devices using CSMA...");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));

    NetDeviceContainer nd_host_r1 = csma.Install(NodeContainer(host, router1));
    NetDeviceContainer nd_r1_c1_relay = csma.Install(NodeContainer(router1, client1, relay));
    NetDeviceContainer nd_relay_r2 = csma.Install(NodeContainer(relay, router2));
    NetDeviceContainer nd_r2_gw = csma.Install(NodeContainer(router2, gateway));
    NetDeviceContainer nd_gw_r3 = csma.Install(NodeContainer(gateway, router3));
    NetDeviceContainer nd_r3_c2_c3 = csma.Install(NodeContainer(router3, client2, client3));

    // --- 3. 인터넷 스택 설치 ---
    InternetStackHelper internet;
    internet.Install(nodes);

    // --- 4. IP 주소 할당 ---
    NS_LOG_INFO("Assigning IP addresses...");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer if_host_r1 = ipv4.Assign(nd_host_r1);
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer if_r1_c1_relay = ipv4.Assign(nd_r1_c1_relay);
    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer if_relay_r2 = ipv4.Assign(nd_relay_r2);
    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer if_r2_gw = ipv4.Assign(nd_r2_gw);
    ipv4.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer if_gw_r3 = ipv4.Assign(nd_gw_r3);
    ipv4.SetBase("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer if_r3_c2_c3 = ipv4.Assign(nd_r3_c2_c3);

    Ipv4Address hostAddr = if_host_r1.GetAddress(0);
    Ipv4Address gatewayAddr = if_r2_gw.GetAddress(1);
    Ipv4Address gatewayMcastSourceAddr = if_gw_r3.GetAddress(0);

    // --- 5. 라우팅 설정 ---
    NS_LOG_INFO("Enabling IP forwarding and configuring routing...");
    router1->GetObject<Ipv4>()->SetAttribute("IpForward", BooleanValue(true));
    router2->GetObject<Ipv4>()->SetAttribute("IpForward", BooleanValue(true));
    router3->GetObject<Ipv4>()->SetAttribute("IpForward", BooleanValue(true));
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // --- 6. 정적 멀티캐스트 라우팅 설정 ---
    Ipv4Address multicastGroup("239.255.0.1");
    Ipv4StaticRoutingHelper multicast;

    multicast.GetStaticRouting(host->GetObject<Ipv4>())->SetDefaultMulticastRoute(1);

    Ptr<Ipv4> r1Ipv4 = router1->GetObject<Ipv4>();
    uint32_t r1_if_in = r1Ipv4->GetInterfaceForDevice(nd_host_r1.Get(1));
    uint32_t r1_if_out = r1Ipv4->GetInterfaceForDevice(nd_r1_c1_relay.Get(0));
    multicast.GetStaticRouting(r1Ipv4)->AddMulticastRoute(hostAddr,
                                                          multicastGroup,
                                                          r1_if_in,
                                                          {r1_if_out});

    Ptr<Ipv4> gwIpv4 = gateway->GetObject<Ipv4>();
    uint32_t gw_if_out = gwIpv4->GetInterfaceForDevice(nd_gw_r3.Get(0));
    multicast.GetStaticRouting(gwIpv4)->SetDefaultMulticastRoute(gw_if_out);

    Ptr<Ipv4> r3Ipv4 = router3->GetObject<Ipv4>();
    uint32_t r3_if_in = r3Ipv4->GetInterfaceForDevice(nd_gw_r3.Get(1));
    uint32_t r3_if_out = r3Ipv4->GetInterfaceForDevice(nd_r3_c2_c3.Get(0));
    multicast.GetStaticRouting(r3Ipv4)->AddMulticastRoute(gatewayMcastSourceAddr,
                                                          multicastGroup,
                                                          r3_if_in,
                                                          {r3_if_out});

    // --- 7. 애플리케이션 설정 ---
    NS_LOG_INFO("Configuring applications...");
    uint16_t dataPort = 9;
    uint16_t tunnelPort = 4341;

    OnOffHelper source("ns3::UdpSocketFactory", InetSocketAddress(multicastGroup, dataPort));
    source.SetConstantRate(DataRate("500kbps"));
    source.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer sourceApp = source.Install(host);
    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(10.0));

    TypeId tid = UdpSocketFactory::GetTypeId();

    // Relay 설정
    Ptr<Socket> relayRecvSocket = Socket::CreateSocket(relay, tid);
    InetSocketAddress local(Ipv4Address::GetAny(), dataPort);
    relayRecvSocket->Bind(local);

    Ptr<Socket> relaySendSocket = Socket::CreateSocket(relay, tid);
    auto relayApp = CreateObject<RelayApp>();
    relayApp->Setup(relayRecvSocket, relaySendSocket, gatewayAddr, tunnelPort);
    relay->AddApplication(relayApp);
    relayApp->SetStartTime(Seconds(0.5));
    relayApp->SetStopTime(Seconds(10.5));

    // Gateway 설정
    Ptr<Socket> gwRecvSocket = Socket::CreateSocket(gateway, tid);
    gwRecvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), tunnelPort));
    Ptr<Socket> gwSendSocket = Socket::CreateSocket(gateway, tid);
    gwSendSocket->Bind(InetSocketAddress(gatewayMcastSourceAddr, 0));
    auto gwApp = CreateObject<GatewayApp>();
    gwApp->Setup(gwRecvSocket, gwSendSocket, multicastGroup, dataPort);
    gateway->AddApplication(gwApp);
    gwApp->SetStartTime(Seconds(0.5));
    gwApp->SetStopTime(Seconds(10.5));

    // 클라이언트들 설정
    PacketSinkHelper clientSinks("ns3::UdpSocketFactory",
                                 InetSocketAddress(Ipv4Address::GetAny(), dataPort));
    ApplicationContainer sinkApps;
    sinkApps.Add(clientSinks.Install(client1));
    sinkApps.Add(clientSinks.Install(client2));
    sinkApps.Add(clientSinks.Install(client3));
    sinkApps.Start(Seconds(0.5));
    sinkApps.Stop(Seconds(10.5));

    // --- 8. PCAP 트레이싱 활성화 ---
    csma.EnablePcapAll("amt-csma-only-sim", false);

    // --- 9. 시뮬레이션 실행 ---
    Simulator::Stop(Seconds(11.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
