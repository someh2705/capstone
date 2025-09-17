// main.cc
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include <iostream>

using namespace ns3;

int main(int argc, char* argv[]) {
    // 로그 출력을 활성화하여 시뮬레이션 진행 상황을 볼 수 있게 합니다.
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // 1. 노드 생성
    NodeContainer nodes;
    nodes.Create(2);

    // 2. 점대점(Point-to-Point) 네트워크 설정
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // 3. 인터넷 스택 설치
    InternetStackHelper stack;
    stack.Install(nodes);

    // 4. IP 주소 할당
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // (선택) 간단한 통신 예제: 0번 노드에 서버, 1번 노드에 클라이언트 설치
    UdpEchoServerHelper echoServer(9); // Port 9
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = echoClient.Install(nodes.Get(1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));


    // 5. 시뮬레이션 실행
    std::cout << "Starting simulation..." << std::endl;
    Simulator::Run();
    Simulator::Destroy();
    std::cout << "Simulation finished successfully." << std::endl;

    return 0;
}
