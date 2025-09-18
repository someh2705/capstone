#ifndef SCENARIO_HELPER_H
#define SCENARIO_HELPER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/trace-helper.h"
#include "applications/streaming-sender.h"
#include "applications/streaming-receiver.h"

using namespace ns3;

class ScenarioHelper {
public:
    ScenarioHelper(uint32_t numReceivers);

    void CreateTopology();
    void AssignIpAddresses();
    void InstallApplications(Ipv4Address multicastGroup, uint16_t multicastPort);
    
    NodeContainer GetSenderNode() const;
    NodeContainer GetRouterNode() const;
    NodeContainer GetReceiverNodes() const;
    NetDeviceContainer GetSenderRouterDevices() const;
    NetDeviceContainer GetRouterReceiverDevices() const;

private:
    uint32_t m_numReceivers;
    NodeContainer m_senderNode;
    NodeContainer m_routerNode;
    NodeContainer m_receiverNodes;

    NetDeviceContainer m_senderRouterDevices;
    NetDeviceContainer m_routerReceiverDevices;
};

#endif // SCENARIO_HELPER_H