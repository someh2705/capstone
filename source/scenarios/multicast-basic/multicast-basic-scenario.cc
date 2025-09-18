#include "multicast-basic-scenario.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE("MulticastBasicScenario");

MulticastBasicScenario::MulticastBasicScenario(uint32_t numReceivers)
    : m_helper(numReceivers),
      m_multicastGroup("239.255.1.1"),
      m_multicastPort(9) {}

void MulticastBasicScenario::Setup() {
    NS_LOG_INFO("Setting up network topology and IP addresses...");
    m_helper.CreateTopology();
    m_helper.AssignIpAddresses();

    NS_LOG_INFO("Configuring Multicast Routing...");
    ConfigureMulticastRouting();

    NS_LOG_INFO("Installing applications...");
    m_helper.InstallApplications(m_multicastGroup, m_multicastPort);

    NS_LOG_INFO("Scenario setup complete.");
}

void MulticastBasicScenario::ConfigureMulticastRouting() {
    Ptr<Node> sender = m_helper.GetSenderNode().Get(0);
    Ptr<Node> router = m_helper.GetRouterNode().Get(0);

    Ptr<Ipv4> senderIpv4 = sender->GetObject<Ipv4>();
    Ptr<Ipv4> routerIpv4 = router->GetObject<Ipv4>();

    routerIpv4->SetAttribute("IpForward", BooleanValue(true));

    // 정적 멀티캐스트 라우팅 설정
    Ipv4StaticRoutingHelper multicast;

    Ipv4ListRoutingHelper listRouting;
    listRouting.Add(multicast, 10);

    Ptr<Ipv4RoutingProtocol> routerProto = listRouting.Create(router);
    router->AggregateObject(routerProto);

    Ptr<Ipv4ListRouting> listRoutingProto = DynamicCast<Ipv4ListRouting>(routerProto);
    if (!listRoutingProto) {
        NS_FATAL_ERROR("Failed to get Ipv4ListRouting protocol from router.");
        return;
    }

    Ptr<Ipv4StaticRouting> staticRouting = 0;
    int16_t priority; // Dummy variable for priority
    for (uint32_t i = 0; i < listRoutingProto->GetNRoutingProtocols(); ++i) {
        Ptr<Ipv4RoutingProtocol> rp = listRoutingProto->GetRoutingProtocol(i, priority);
        staticRouting = DynamicCast<Ipv4StaticRouting>(rp);
        if (staticRouting) {
            break; // Found it
        }
    }

    if (!staticRouting) {
        NS_FATAL_ERROR("Failed to get Ipv4StaticRouting protocol from Ipv4ListRouting.");
        return;
    }

    NetDeviceContainer senderRouterDevices = m_helper.GetSenderRouterDevices();
    NetDeviceContainer routerReceiverDevices = m_helper.GetRouterReceiverDevices();

    uint32_t senderRouterIfIndex = routerIpv4->GetInterfaceForDevice(senderRouterDevices.Get(1));
    Ipv4Address senderAddress = senderIpv4->GetAddress(0, 0).GetLocal(); // Sender's IP is on interface 0

    uint32_t numReceivers = m_helper.GetReceiverNodes().GetN();

    for(uint32_t i = 0; i < numReceivers; ++i) {
        uint32_t routerReceiverIfIndex = routerIpv4->GetInterfaceForDevice(routerReceiverDevices.Get(i*2));

        std::vector<uint32_t> outputIfIndices;
        outputIfIndices.push_back(routerReceiverIfIndex);

        staticRouting->AddMulticastRoute(senderAddress,
                                         m_multicastGroup,
                                         senderRouterIfIndex, // Input interface index (router's interface connected to sender)
                                         outputIfIndices);
    }
}

NodeContainer MulticastBasicScenario::GetSenderNode() const {
    return m_helper.GetSenderNode();
}

NodeContainer MulticastBasicScenario::GetRouterNode() const {
    return m_helper.GetRouterNode();
}

NodeContainer MulticastBasicScenario::GetReceiverNodes() const {
    return m_helper.GetReceiverNodes();
}