#ifndef INCLUDE_SETUP_H
#define INCLUDE_SETUP_H

#include <ns3/ipv4-address.h>
#include <ns3/ipv4-interface-container.h>
#include <ns3/net-device-container.h>
#include <ns3/node-container.h>
#include <ns3/node.h>
#include <ns3/ptr.h>

#include <cstdint>
#include <string>
#include <unordered_map>

struct Link
{
    std::string subnect;
    ns3::NetDeviceContainer devices;
    ns3::Ipv4InterfaceContainer interfaces;
};

struct McRoute
{
    std::string node;
    std::string inner;
    std::vector<std::string> outers;
};

struct AppConfig
{
    std::string type;
    std::string node;
    uint16_t port;
    double start;
    double stop;

    // For OnOff
    std::string target;
    std::string rate;
    uint32_t packetSize;

    // For Relay
    std::string gateway;
    uint16_t unicast;
    std::string link;

    // For Gateway
    std::string relay;
};

class Topology
{
  public:
    Topology(std::string&);

    ns3::NodeContainer GetNodes() const
    {
        return m_nodes;
    }

    ns3::Ptr<ns3::Node> GetNode(std::string name) const
    {
        return m_nodeMap.at(name);
    }

    std::string GetMcSource() const
    {
        return m_mcSource;
    }

    std::string GetMcGroup() const
    {
        return m_mcGroup;
    }

    const std::vector<McRoute>& GetMcRoutes() const
    {
        return m_mcRoutes;
    }

    const std::vector<AppConfig>& GetAppConfigs() const
    {
        return m_apps;
    }

  private:
    ns3::NodeContainer m_nodes;
    std::unordered_map<std::string, ns3::Ptr<ns3::Node>> m_nodeMap;
    std::unordered_map<std::string, Link> m_linkMap;

    std::string m_mcSource;
    std::string m_mcGroup;
    std::vector<McRoute> m_mcRoutes;

    std::vector<AppConfig> m_apps;

    std::string m_pcap;

    uint32_t FindInterfaceIndex(ns3::Ptr<ns3::Node>, const std::string&);
    ns3::Ipv4Address GetAddressOnLink(ns3::Ptr<ns3::Node>, const std::string&);
};

#endif
