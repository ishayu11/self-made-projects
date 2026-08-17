/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpExample");

class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

int
main (int argc, char *argv[])
{
  // Default parameters
  std::string tcp_variant = "TcpCubic";
  std::string bandwidth = "5Mbps";
  std::string delay = "5ms";
  std::string queuesize = "10p";
  double error_rate = 0.000001;
  int simulation_time = 10; //seconds

  // Allow passing parameters via command line
  CommandLine cmd;
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpReno, TcpNewReno, TcpCubic, TcpBic", tcp_variant);
  cmd.Parse (argc, argv);

  // Select TCP variant including BIC
  if (tcp_variant.compare("TcpCubic") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpCubic"));
  else if (tcp_variant.compare("TcpReno") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpReno"));
  else if (tcp_variant.compare("TcpNewReno") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  else if (tcp_variant.compare("TcpBic") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpBic"));
  else
    {
      fprintf (stderr, "Invalid TCP version\n");
      exit (1);
    }

  NodeContainer n0n1;
  n0n1.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));
  
  //set qsize for N0 and N1
  pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (queuesize));
  NetDeviceContainer devices;
  devices = pointToPoint.Install (n0n1);

  NodeContainer n1n2;
  n1n2.Add (n0n1.Get (1));
  n1n2.Create (1);
  
  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue (delay));
  
  //set qsize for N1 and N2
  pointToPoint2.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue (queuesize));
  NetDeviceContainer devices2;
  devices2= pointToPoint2.Install (n1n2);

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (error_rate));
  devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  InternetStackHelper stack;
  stack.InstallAll ();
  
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  Ipv4AddressHelper address2;
  address2.SetBase ("10.1.2.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces2 = address2.Assign (devices2);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  uint16_t sinkPort = 8080;
  Address sinkAddress (InetSocketAddress (interfaces2.GetAddress (1), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (n1n2.Get (1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (simulation_time));

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (n0n1.Get (0), TcpSocketFactory::GetTypeId ());

  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1460, 1000000, DataRate ("100Mbps"));
  n0n1.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (simulation_time));

  // Dynamic file naming based on the TCP variant chosen
  std::string cwndFileName = "scratch/my-projects/tcp-example-" + tcp_variant + ".cwnd";
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (cwndFileName);
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream));

  // Detailed trace of queue enq/deq packet tx/rx
  std::string trFileName = "scratch/my-projects/tcp-example-" + tcp_variant + ".tr";
  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream (trFileName));
  
  std::string pcapFileName = "scratch/my-projects/tcp-example-" + tcp_variant;
  pointToPoint.EnablePcapAll (pcapFileName);

  Simulator::Stop (Seconds (simulation_time));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}