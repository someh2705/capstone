#include "scenario-helper.h"
#include "ns3/applications-module.h"
#include <ns3/internet-stack-helper.h>

ScenarioHelper::ScenarioHelper(uint32_t numReceivers)
    : m_numReceivers(numReceivers) {}

void ScenarioHelper::CreateTopology() {
    // 1개의 송신자, 1개의 라우터, N개의 수신자 생성
    m_senderNode.Create(1);
    m_routerNode.Create(1);
    m_receiverNodes.Create(m_numReceivers);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));

    // 송신자 - 라우터 연결
    m_senderRouterDevices = p2p.Install(m_senderNode.Get(0), m_routerNode.Get(0));

    // 라우터 - 수신자들 연결
    for (uint32_t i = 0; i < m_numReceivers; ++i) {
        NetDeviceContainer devices = p2p.Install(m_routerNode.Get(0), m_receiverNodes.Get(i));
        m_routerReceiverDevices.Add(devices);
    }

    // Configure tracing
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream("multicast-trace.tr");
    p2p.EnableAsciiAll(stream);
    p2p.EnablePcapAll("multicast", false);
}

void ScenarioHelper::AssignIpAddresses() {
    InternetStackHelper stack;
    stack.Install(m_senderNode);
    stack.Install(m_routerNode);
    stack.Install(m_receiverNodes);
    
    Ipv4AddressHelper address;
    
    // 송신자 - 라우터망 IP 할당
    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(m_senderRouterDevices);

    // 라우터 - 수신자망 IP 할당
    for (uint32_t i = 0; i < m_numReceivers; ++i) {
        // 각 수신자 링크마다 다른 서브넷 할당
        std::string baseIp = "10.1." + std::to_string(i + 2) + ".0";
        address.SetBase(Ipv4Address(baseIp.c_str()), "255.255.255.0");
        
        NetDeviceContainer devices;
        devices.Add(m_routerReceiverDevices.Get(i*2)); // Router-side device
        devices.Add(m_routerReceiverDevices.Get(i*2 + 1)); // Receiver-side device
        address.Assign(devices);
    }
}

void ScenarioHelper::InstallApplications(Ipv4Address multicastGroup, uint16_t multicastPort) {
    // 송신 애플리케이션 설치
    OnOffHelper senderHelper("ns3::UdpSocketFactory", InetSocketAddress(multicastGroup, multicastPort));
    senderHelper.SetConstantRate(DataRate("256kbps"), 512); // 256kbps, 512-byte packets
    ApplicationContainer senderApps = senderHelper.Install(m_senderNode.Get(0));
    senderApps.Start(Seconds(1.0));
    senderApps.Stop(Seconds(10.0));

    // 수신 애플리케이션 설치
    for (uint32_t i = 0; i < m_numReceivers; ++i) {
        Ptr<StreamingReceiverApp> receiverApp = CreateObject<StreamingReceiverApp>();
        receiverApp->SetAddress(multicastGroup, multicastPort);
        
        Ptr<Node> receiverNode = m_receiverNodes.Get(i);
        receiverNode->AddApplication(receiverApp);

        // Make receiver join the multicast group
        Ptr<Ipv4> ipv4 = receiverNode->GetObject<Ipv4>();
        // Find the interface connected to the router
        // The receiver's device connected to the router is at index i*2 + 1 in m_routerReceiverDevices
        Ptr<NetDevice> receiverSideDevice = m_routerReceiverDevices.Get(i * 2 + 1);
        uint32_t interfaceIndex = ipv4->GetInterfaceForDevice(receiverSideDevice);
        receiverApp->SetInterfaceIndex(interfaceIndex);
        
        receiverApp->SetStartTime(Seconds(0.5));
        receiverApp->SetStopTime(Seconds(10.5));
    }
}

NodeContainer ScenarioHelper::GetSenderNode() const { return m_senderNode; }
NodeContainer ScenarioHelper::GetRouterNode() const { return m_routerNode; }
NodeContainer ScenarioHelper::GetReceiverNodes() const { return m_receiverNodes; }
NetDeviceContainer ScenarioHelper::GetSenderRouterDevices() const { return m_senderRouterDevices; }
NetDeviceContainer ScenarioHelper::GetRouterReceiverDevices() const { return m_routerReceiverDevices; }