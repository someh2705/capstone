#ifndef CAPSTONE_BASIC_AMT_H
#define CAPSTONE_BASIC_AMT_H

#include <ns3/application.h>
#include <ns3/inet-socket-address.h>
#include <ns3/int64x64-128.h>
#include <ns3/ipv4-address.h>
#include <ns3/socket.h>
#include <ns3/type-id.h>

#include <cstdint>

class RelayApp : public ns3::Application
{
  public:
    static ns3::TypeId GetTypeId();

    RelayApp();
    ~RelayApp() override;

    void Setup(ns3::Ipv4Address gatewayAddress,
               uint16_t gatewayPort,
               ns3::Ipv4Address multicastGroup,
               uint16_t multicastPort);

  protected:
    void DoDispose() override;

  private:
    ns3::Ptr<ns3::Socket> m_recvSocket;
    ns3::Ptr<ns3::Socket> m_sendSocket;
    ns3::Ipv4Address m_gatewayAddress;
    uint16_t m_gatewayPort;
    ns3::Ipv4Address m_multicastGroup;
    uint16_t m_multicastPort;

    void StartApplication() override;
    void StopApplication() override;

    void HandleRead(ns3::Ptr<ns3::Socket> socket);
};

class GatewayApp : public ns3::Application
{
  public:
    static ns3::TypeId GetTypeId();

    GatewayApp();
    ~GatewayApp() override;

    void Setup(uint16_t unicastPort);
    void HandleRead(ns3::Ptr<ns3::Socket> socket);

  protected:
    void DoDispose() override;

  private:
    ns3::Ptr<ns3::Socket> m_recvSocket;
    ns3::Ptr<ns3::Socket> m_sendSocket;
    uint16_t m_unicastPort;

    void StartApplication() override;
    void StopApplication() override;
};

class BasicAmt
{
  public:
    BasicAmt(int, char*[]);
};

#endif
