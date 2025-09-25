#include "basic-multicast.h"

#include "setup.h"

#include <ns3/ipv4-address.h>
#include <ns3/log.h>
#include <ns3/simulator.h>

BasicMulticast::BasicMulticast(int argc, char* argv[])
{
    using namespace ns3;
    std::string filename{"../resources/basic-multicast.yaml"};

    std::cout << "topology setup: " << filename << std::endl;
    Topology topology(filename);

    Simulator::Stop(Seconds(21.0));
    Simulator::Run();
    Simulator::Destroy();
}
