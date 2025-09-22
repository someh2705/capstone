#include <ns3/ipv4-static-routing.h>
#include <ns3/ipv4.h>
#include <vector>

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CsmaMulticastExample");

int main(int argc, char *argv[]) {
  Config::SetDefault("ns3::CsmaNetDevice::EncapsulationMode",
                     StringValue("Dix"));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
  LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

  NS_LOG_INFO("Creating nodes.");
  NodeContainer c;
  c.Create(5);
  NodeContainer c0 = NodeContainer(c.Get(0), c.Get(1), c.Get(2));
  NodeContainer c1 = NodeContainer(c.Get(2), c.Get(3), c.Get(4));

  NS_LOG_INFO("Building topology.");
  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(5000000)));
  csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

  NetDeviceContainer nd0 = csma.Install(c0);
  NetDeviceContainer nd1 = csma.Install(c1);

  NS_LOG_INFO("Adding IP stack.");
  InternetStackHelper internet;
  internet.Install(c);

  NS_LOG_INFO("Assigning IP addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0 = ipv4.Assign(nd0);
  ipv4.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1 = ipv4.Assign(nd1);

  NS_LOG_INFO("Configuring multicast.");
  Ipv4Address multicastSource = i0.GetAddress(0);
  Ipv4Address multicastGroup = "225.1.2.4";

  Ipv4StaticRoutingHelper multicast;

  Ptr<Node> sourceNode = c.Get(0);
  Ptr<Ipv4> sourceIpv4 = sourceNode->GetObject<Ipv4>();
  Ptr<Ipv4StaticRouting> sourceStaticRouting =
      multicast.GetStaticRouting(sourceIpv4);
  sourceStaticRouting->SetDefaultMulticastRoute(1);

  Ptr<Node> multicastRouter = c.Get(2);
  Ptr<Ipv4> ipv4Router = multicastRouter->GetObject<Ipv4>();
  Ptr<Ipv4StaticRouting> staticRoutingRouter =
      multicast.GetStaticRouting(ipv4Router);

  std::vector<uint32_t> outputInterfaces{2};

  staticRoutingRouter->AddMulticastRoute(multicastSource, multicastGroup, 1,
                                         outputInterfaces);

  NS_LOG_INFO("Creating applications.");

  uint16_t multicastPort = 9999;
  OnOffHelper onoff("ns3::UdpSocketFactory",
                    Address(InetSocketAddress(multicastGroup, multicastPort)));
  onoff.SetConstantRate(DataRate("256b/s"));
  onoff.SetAttribute("PacketSize", UintegerValue(256));

  ApplicationContainer srcC = onoff.Install(c.Get(0));
  srcC.Start(Seconds(1.0));
  srcC.Stop(Seconds(60.0));

  PacketSinkHelper sink(
      "ns3::UdpSocketFactory",
      Address(InetSocketAddress(Ipv4Address::GetAny(), multicastPort)));

  ApplicationContainer sinkC = sink.Install(c.Get(4));
  sinkC.Start(Seconds(1.0));
  sinkC.Stop(Seconds(60.0));

  NS_LOG_INFO("Starting simulation.");

  csma.EnablePcapAll("csma-multicast", true);
  Simulator::Stop(Seconds(61.0));
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_INFO("Done.");
}
