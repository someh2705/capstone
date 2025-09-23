cmake_minimum_required(VERSION 3.10)
project(capstone CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NS3 REQUIRED
  ns3-applications
  ns3-core
  ns3-tap-bridge
  ns3-traffic-control
  ns3-wifi
  ns3-visualizer
  ns3-mobility
  ns3-antenna
  ns3-network
  ns3-dsr
  ns3-virtual-net-device
  ns3-point-to-point
  ns3-dsdv
  ns3-internet
  ns3-propagation
  ns3-flow-monitor
  ns3-zigbee
  ns3-csma-layout
  ns3-mesh
  ns3-spectrum
  ns3-lte
  ns3-topology-read
  ns3-aodv
  ns3-energy
  ns3-lr-wpan
  ns3-wimax
  ns3-buildings
  ns3-nix-vector-routing
  ns3-config-store
  ns3-bridge
  ns3-netanim
  ns3-stats
  ns3-csma
  ns3-sixlowpan
  ns3-point-to-point-layout
  ns3-internet-apps
  ns3-olsr
  ns3-fd-net-device
  ns3-uan
)

file(GLOB_RECURSE ALL_SOURCES "source/main.cc")

add_executable(capstone ${ALL_SOURCES})

target_include_directories(capstone PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source ${NS3_INCLUDE_DIRS})
target_link_libraries(capstone PRIVATE ${NS3_LIBRARIES})
