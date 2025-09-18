#ifndef IGMP_BASIC_SCENARIO_H
#define IGMP_BASIC_SCENARIO_H

#include "shared/scenario-helper.h"

using namespace ns3;

class MulticastBasicScenario {
public:
    MulticastBasicScenario(uint32_t numReceivers);
    void Setup();

    NodeContainer GetSenderNode() const;
    NodeContainer GetRouterNode() const;
    NodeContainer GetReceiverNodes() const;

private:
    void ConfigureMulticastRouting();
    
    ScenarioHelper m_helper;
    Ipv4Address m_multicastGroup;
    uint16_t m_multicastPort;
};

#endif // IGMP_BASIC_SCENARIO_H