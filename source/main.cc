#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h" // NetAnim 헤더 추가
#include "ns3/mobility-module.h" // Mobility 헤더 추가
#include "scenarios/multicast-basic/multicast-basic-scenario.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("StreamingSimMain");

int main(int argc, char* argv[]) {
    std::string scenarioType = "multicast-basic";
    uint32_t numReceivers = 3;

    CommandLine cmd;
    cmd.AddValue("scenario", "Select scenario (multicast-basic or amt-basic)", scenarioType);
    cmd.AddValue("numReceivers", "Number of multicast receivers", numReceivers);
    cmd.Parse(argc, argv);

    LogComponentEnable("StreamingReceiverApp", LOG_LEVEL_INFO);
    LogComponentEnable("MulticastBasicScenario", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);

    if (scenarioType == "multicast-basic") {
        NS_LOG_INFO("Setting up Multicast Basic Scenario...");
        MulticastBasicScenario scenario(numReceivers);
        scenario.Setup(); // 시나리오 설정만 수행

        // 모든 노드에 MobilityModel 설치 (NetAnim 호환성을 위해)
        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(scenario.GetSenderNode());
        mobility.Install(scenario.GetRouterNode());
        mobility.Install(scenario.GetReceiverNodes());

        // NetAnim 애니메이션 인터페이스 생성
        AnimationInterface anim("multicast.xml");

        // NetAnim 노드 위치 설정
        anim.SetConstantPosition(scenario.GetSenderNode().Get(0), 10.0, 50.0);
        anim.SetConstantPosition(scenario.GetRouterNode().Get(0), 50.0, 50.0);
        for (uint32_t i = 0; i < numReceivers; ++i) {
            anim.SetConstantPosition(scenario.GetReceiverNodes().Get(i), 90.0, 20.0 + i * 30.0);
        }

    } else if (scenarioType == "amt-basic") {
        NS_LOG_ERROR("AMT Basic Scenario is not yet implemented.");
        return 1;
    } else {
        NS_LOG_ERROR("Invalid scenario type specified: " << scenarioType);
        return 1;
    }

    // 모든 시나리오 설정이 완료된 후 시뮬레이션 실행
    Simulator::Stop(Seconds(11.0)); // 시뮬레이션 종료 시간 설정
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("Simulation finished.");
    return 0;
}