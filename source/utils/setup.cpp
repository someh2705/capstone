#include "setup.h"

#include "basic-amt.h"

#include <ns3/application-container.h>
#include <ns3/assert.h>
#include <ns3/boolean.h>
#include <ns3/channel.h>
#include <ns3/config.h>
#include <ns3/csma-helper.h>
#include <ns3/fatal-error.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/ipv4-address.h>
#include <ns3/ipv4-global-routing-helper.h>
#include <ns3/ipv4-interface-container.h>
#include <ns3/ipv4-static-routing-helper.h>
#include <ns3/ipv4-static-routing.h>
#include <ns3/ipv4.h>
#include <ns3/log.h>
#include <ns3/names.h>
#include <ns3/net-device-container.h>
#include <ns3/net-device.h>
#include <ns3/node-container.h>
#include <ns3/node.h>
#include <ns3/nstime.h>
#include <ns3/object-factory.h>
#include <ns3/object.h>
#include <ns3/on-off-helper.h>
#include <ns3/packet-sink-helper.h>
#include <ns3/ptr.h>
#include <ns3/string.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

using namespace ns3;

Topology::Topology(std::string& filename)
{
    std::cout << "parsing yaml: " << filename << std::endl;
    YAML::Node config = YAML::LoadFile(filename);

    Config::SetDefault("ns3::CsmaNetDevice::EncapsulationMode", StringValue("Dix"));

    NodeContainer nodes;
    std::unordered_map<std::string, Ptr<Node>> nodeMap;

    std::cout << "node setup" << std::endl;
    for (auto n : config["nodes"])
    {
        auto name = n["name"].as<std::string>();
        auto node = CreateObject<Node>();

        nodes.Add(node);
        nodeMap[name] = node;
        Names::Add(name, node);
    }

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;

    std::unordered_map<std::string, Link> linkMap;

    std::cout << "link setup" << std::endl;
    for (auto l : config["links"])
    {
        auto name = l["name"].as<std::string>();
        auto subnet = l["subnet"].as<std::string>();
        auto mask = l["mask"].as<std::string>();
        auto members = l["nodes"].as<std::vector<std::string>>();

        NodeContainer link;
        for (auto& m : members)
        {
            link.Add(nodeMap[m]);
        }

        NetDeviceContainer devices = csma.Install(link);

        ipv4.SetBase(subnet.c_str(), mask.c_str());
        auto interface = ipv4.Assign(devices);

        linkMap[name] = Link{subnet, devices, interface};

        for (uint32_t i = 0; i < devices.GetN(); ++i)
        {
            std::string neighbor = Names::FindName(link.Get(i));
            Names::Add(neighbor + "-" + name, devices.Get(i));
        }
    }

    m_nodes = nodes;
    m_nodeMap = nodeMap;
    m_linkMap = linkMap;

    std::cout << "ip forward setting" << std::endl;

    for (auto& kv : nodeMap)
    {
        kv.second->GetObject<Ipv4>()->SetAttribute("IpForward", BooleanValue(true));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::cout << "multicast route setup" << std::endl;
    if (config["multicast"])
    {
        m_mcSource = config["multicast"]["source"].as<std::string>();
        m_mcGroup = config["multicast"]["group"].as<std::string>();

        for (auto r : config["multicast"]["routes"])
        {
            McRoute route;
            route.node = r["node"].as<std::string>();
            if (r["in"])
            {
                route.inner = r["in"].as<std::string>();
            }
            if (r["out"])
            {
                if (r["out"].IsSequence())
                {
                    route.outers = r["out"].as<std::vector<std::string>>();
                }
                else
                {
                    route.outers.push_back(r["out"].as<std::string>());
                }
            }
            m_mcRoutes.push_back(route);
        }
    }

    if (config["applications"])
    {
        std::cout << "application setup" << std::endl;

        for (auto a : config["applications"])
        {
            AppConfig app;
            app.type = a["type"].as<std::string>();
            app.node = a["node"].as<std::string>();
            if (a["target"])
            {
                app.target = a["target"].as<std::string>();
            }
            if (a["port"])
            {
                app.port = a["port"].as<uint16_t>();
            }
            if (a["rate"])
            {
                app.rate = a["rate"].as<std::string>();
            }
            if (a["packetSize"])
            {
                app.packetSize = a["packetSize"].as<uint32_t>();
            }
            if (a["start"])
            {
                app.start = a["start"].as<double>();
            }
            if (a["stop"])
            {
                app.stop = a["stop"].as<double>();
            }
            // For Relay
            if (a["gateway"])
            {
                app.gateway = a["gateway"].as<std::string>();
            }
            if (a["unicast"])
            {
                app.unicast = a["unicast"].as<uint16_t>();
            }
            if (a["link"])
            {
                app.link = a["link"].as<std::string>();
            }
            // For Gateway
            if (a["relay"])
            {
                app.relay = a["relay"].as<std::string>();
            }
            m_apps.push_back(app);
        }
    }

    std::cout << "multicast route setting" << std::endl;
    Ipv4Address multicastGroup(m_mcGroup.c_str());
    Ipv4StaticRoutingHelper multicast;
    Ptr<Node> sourceNode = GetNode(m_mcSource);
    Ipv4Address sourceAddr;

    for (const auto& route : m_mcRoutes)
    {
        if (route.node == m_mcSource)
        {
            NS_ASSERT_MSG(route.outers.size() == 1, "Source node route must have an 'out' link.");
            const std::string& sourceOutLink = route.outers[0];

            uint32_t index = FindInterfaceIndex(sourceNode, sourceOutLink);
            sourceAddr = sourceNode->GetObject<Ipv4>()->GetAddress(index, 0).GetLocal();
            break;
        }
    }

    for (auto& route : m_mcRoutes)
    {
        Ptr<Node> node = GetNode(route.node);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        Ptr<Ipv4StaticRouting> rt = multicast.GetStaticRouting(ipv4);

        if (route.inner.empty())
        {
            NS_ASSERT_MSG(route.outers.size() == 1,
                          "Source node must have exactly one outgoing link for default route");
            uint32_t index = FindInterfaceIndex(node, route.outers[0]);
            rt->SetDefaultMulticastRoute(index);
        }
        else
        {
            uint32_t index = FindInterfaceIndex(node, route.inner);
            std::vector<uint32_t> interfaces;
            interfaces.reserve(route.outers.size());

            for (auto& out : route.outers)
            {
                interfaces.push_back(FindInterfaceIndex(node, out));
            }
            rt->AddMulticastRoute(sourceAddr, multicastGroup, index, interfaces);
        }
    }
    std::cout << "application setting" << std::endl;

    for (auto& app : m_apps)
    {
        Ptr<Node> n = GetNode(app.node);
        ApplicationContainer container;

        if (app.type == "OnOff")
        {
            Ipv4Address group(app.target.c_str());
            OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(group, app.port)));

            onoff.SetConstantRate(DataRate(app.rate));
            onoff.SetAttribute("PacketSize", UintegerValue(app.packetSize));

            ApplicationContainer container = onoff.Install(n);
        }
        else if (app.type == "PacketSink")
        {
            PacketSinkHelper sink("ns3::UdpSocketFactory",
                                  Address(InetSocketAddress(Ipv4Address::GetAny(), app.port)));
            container = sink.Install(n);
        }
        else if (app.type == "Relay")
        {
            ObjectFactory factory;
            factory.SetTypeId("RelayApp");
            Ptr<RelayApp> relayApp = factory.Create<RelayApp>();

            Ptr<Node> gatewayNode = GetNode(app.gateway);
            Ipv4Address gatewayAddr = GetAddressOnLink(gatewayNode, app.link);
            relayApp->Setup(gatewayAddr, app.unicast, multicastGroup, app.port);
            n->AddApplication(relayApp);
            container.Add(relayApp);
        }
        else if (app.type == "Gateway")
        {
            ObjectFactory factory;
            factory.SetTypeId("GatewayApp");
            Ptr<GatewayApp> gatewayApp = factory.Create<GatewayApp>();
            gatewayApp->Setup(app.unicast);
            n->AddApplication(gatewayApp);
            container.Add(gatewayApp);
        }
        else
        {
            NS_FATAL_ERROR("Unknown application type: " + app.type);
        }

        container.Start(Seconds(app.start));
        container.Stop(Seconds(app.stop));
    }

    if (config["pcap"])
    {
        m_pcap = config["pcap"].as<std::string>();
        csma.EnablePcapAll(m_pcap);
    }
}

uint32_t
Topology::FindInterfaceIndex(Ptr<Node> node, const std::string& link)
{
    auto it = m_linkMap.find(link);
    if (it == m_linkMap.end())
    {
        NS_FATAL_ERROR("Link name not found: " << link);
    }

    auto devices = it->second.devices;
    auto ipv4 = node->GetObject<Ipv4>();

    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        if (devices.Get(i)->GetNode() == node)
        {
            return ipv4->GetInterfaceForDevice(devices.Get(i));
        }
    }

    NS_FATAL_ERROR("Node not found on link: " << link);

    return -1;
}

Ipv4Address
Topology::GetAddressOnLink(Ptr<Node> node, const std::string& link)
{
    auto it = m_linkMap.find(link);
    if (it == m_linkMap.end())
    {
        NS_FATAL_ERROR("Link name not found: " << link);
    }

    for (uint32_t i = 0; i < it->second.devices.GetN(); ++i)
    {
        if (it->second.devices.Get(i)->GetNode() == node)
        {
            return it->second.interfaces.GetAddress(i);
        }
    }

    NS_FATAL_ERROR("Node " << Names::FindName(node) << " not found on link: " << link);
    return Ipv4Address();
}
