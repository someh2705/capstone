#include "basic-multicast.h"

#include "setup.h"

#include <ns3/log.h>

BasicMulticast::BasicMulticast(int argc, char* argv[])
{
    using namespace ns3;
    std::string filename{"../resources/basic-multicast.yaml"};
    auto topo = SetupTopology(filename);
    NS_LOG_INFO("Topology Created!");
}
