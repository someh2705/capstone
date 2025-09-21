#include "ns3/core-module.h"
#include <string>

int main(int argc, char* argv[]) {
    std::string scenarioType { "basic-multicast" };
    
    ns3::CommandLine cmd;
    cmd.AddValue("scenarioType", "set the scenario type", scenarioType);

    cmd.Parse(argc, argv);

    std::cout << "current scenario type is "
              << scenarioType
              << ". Hello NS3!"
              << std::endl;
    
    return 0;
}
