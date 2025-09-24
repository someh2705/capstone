#ifndef INCLUDE_SETUP_H
#define INCLUDE_SETUP_H

#include <ns3/ipv4-interface-container.h>
#include <ns3/net-device-container.h>
#include <ns3/node-container.h>
#include <ns3/node.h>
#include <ns3/ptr.h>

#include <string>
#include <unordered_map>

struct Link
{
    std::string subnect;
    ns3::NetDeviceContainer devices;
    ns3::Ipv4InterfaceContainer interfaces;
};

struct Topology
{
    ns3::NodeContainer nodes;
    std::unordered_map<std::string, ns3::Ptr<ns3::Node>> nodeMap;
    std::vector<Link> links;
};

Topology SetupTopology(std::string& filename);

#endif
