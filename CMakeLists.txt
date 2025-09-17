cmake_minimum_required(VERSION 3.10)
project(capstone CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NS3 REQUIRED
  ns3-core
  ns3-network
  ns3-internet
  ns3-point-to-point
  ns3-applications
)

add_executable(capstone src/main.cpp)

target_include_directories(capstone PRIVATE ${NS3_INCLUDE_DIRS})
target_link_libraries(capstone PRIVATE ${NS3_LIBRARIES})
