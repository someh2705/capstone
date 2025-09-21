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
  ns3-netanim
)

file(GLOB_RECURSE ALL_SOURCES "source/*.cc")

add_executable(capstone ${ALL_SOURCES})

target_include_directories(capstone PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source ${NS3_INCLUDE_DIRS})
target_link_libraries(capstone PRIVATE ${NS3_LIBRARIES})
