#include "setup.h"

#include <ns3/boolean.h>
#include <ns3/csma-helper.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/ipv4-global-routing-helper.h>
#include <ns3/ipv4-interface-container.h>
#include <ns3/ipv4.h>
#include <ns3/names.h>
#include <ns3/net-device-container.h>
#include <ns3/node-container.h>
#include <ns3/node.h>
#include <ns3/nstime.h>
#include <ns3/object.h>
#include <ns3/ptr.h>
#include <ns3/string.h>

#include <string>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

Topology
SetupTopology(std::string& filename)
{
    using namespace ns3;

    YAML::Node config = YAML::LoadFile(filename);

    std::cout << "config: " << config << std::endl;

    NodeContainer nodes;
    std::unordered_map<std::string, Ptr<Node>> nodeMap;

    for (auto n : config["nodes"])
    {
        auto name = n["name"].as<std::string>();
        auto node = CreateObject<Node>();

        nodes.Add(node);
        nodeMap[name] = node;
        Names::Add(name, node);

        std::cout << "nodeMap add " << Names::FindName(node) << std::endl;
    }

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;

    std::vector<Link> links;

    for (auto l : config["links"])
    {
        auto subnet = l["subnet"].as<std::string>();
        auto mask = l["mask"].as<std::string>();
        auto members = l["nodes"].as<std::vector<std::string>>();

        NodeContainer link;
        std::cout << "link add: ";
        for (auto& m : members)
        {
            link.Add(nodeMap[m]);
            std::cout << m << " ";
        }

        std::cout << std::endl;

        NetDeviceContainer devices = csma.Install(link);

        ipv4.SetBase(subnet.c_str(), mask.c_str());
        auto interface = ipv4.Assign(devices);

        links.push_back({subnet, devices, interface});
    }

    for (auto& kv : nodeMap)
    {
        kv.second->GetObject<Ipv4>()->SetAttribute("IpForward", BooleanValue(true));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    return {nodes, nodeMap, links};
}
