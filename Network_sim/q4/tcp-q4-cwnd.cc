/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("TcpComparisonQ4");

// Function to log CWND changes to a file
static void CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldCwnd << "\t" << newCwnd << endl;
}

// Function to connect the congestion window trace source
static void TraceCwnd(string context, Ptr<OutputStreamWrapper> stream)
{
  Config::ConnectWithoutContext(context, MakeBoundCallback(&CwndChange, stream));
}

int main(int argc, char* argv[])
{
  // Default variant is TcpCubic, but we can change it via command line
  string tcp_variant = "TcpCubic";
  double simulation_time = 30.0; // Increased time to let the Cubic curve develop

  CommandLine cmd(__FILE__);
  cmd.AddValue("transport_prot", "Transport protocol to use (TcpNewReno or TcpCubic)", tcp_variant);
  cmd.Parse(argc, argv);

  // Set the global TCP variant based on selection
  string socket_factory = "ns3::" + tcp_variant;
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(socket_factory));

  // Increase buffer sizes to allow windows to grow large enough to show the curve
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(2000000));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(2000000));

  // 1. Create Nodes
  NodeContainer n0n1, r1r2, n2n3;
  n0n1.Create(2); r1r2.Create(2); n2n3.Create(2);

  // 2. Configure Link Helpers
  PointToPointHelper p2pEdge;
  p2pEdge.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  p2pEdge.SetChannelAttribute("Delay", StringValue("2ms"));

  PointToPointHelper p2pBottleneck;
  p2pBottleneck.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  // HIGH LATENCY: 500ms is crucial to differentiate Cubic from Reno
  p2pBottleneck.SetChannelAttribute("Delay", StringValue("500ms"));
  p2pBottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("100p"));

  // 3. Install NetDevices
  NetDeviceContainer d_n0r1 = p2pEdge.Install(n0n1.Get(0), r1r2.Get(0));
  NetDeviceContainer d_n1r1 = p2pEdge.Install(n0n1.Get(1), r1r2.Get(0));
  NetDeviceContainer d_r1r2 = p2pBottleneck.Install(r1r2.Get(0), r1r2.Get(1));
  NetDeviceContainer d_r2n2 = p2pEdge.Install(r1r2.Get(1), n2n3.Get(0));
  NetDeviceContainer d_r2n3 = p2pEdge.Install(r1r2.Get(1), n2n3.Get(1));

  // 4. Install Internet Stack
  InternetStackHelper stack;
  stack.InstallAll();

  // 5. Assign IP Addresses
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0"); Ipv4InterfaceContainer i_n0r1 = address.Assign(d_n0r1);
  address.SetBase("10.1.2.0", "255.255.255.0"); Ipv4InterfaceContainer i_n1r1 = address.Assign(d_n1r1);
  address.SetBase("10.1.3.0", "255.255.255.0"); Ipv4InterfaceContainer i_r1r2 = address.Assign(d_r1r2);
  address.SetBase("10.1.4.0", "255.255.255.0"); Ipv4InterfaceContainer i_r2n2 = address.Assign(d_r2n2);
  address.SetBase("10.1.5.0", "255.255.255.0"); Ipv4InterfaceContainer i_r2n3 = address.Assign(d_r2n3);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // 6. Setup Applications
  uint16_t port1 = 8080;
  BulkSendHelper source1("ns3::TcpSocketFactory", InetSocketAddress(i_r2n2.GetAddress(1), port1));
  source1.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer sourceApp1 = source1.Install(n0n1.Get(0));
  sourceApp1.Start(Seconds(1.0)); sourceApp1.Stop(Seconds(simulation_time));

  PacketSinkHelper sink1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port1));
  sink1.Install(n2n3.Get(0)).Start(Seconds(0.0));

  // 7. Setup Tracing
  AsciiTraceHelper ascii;
  // Files will be saved in scratch/combined/q4/ (make sure this folder exists)
  Ptr<OutputStreamWrapper> stream1 = ascii.CreateFileStream("scratch/combined/q4/flow_data.cwnd");
  Simulator::Schedule(Seconds(1.1), &TraceCwnd, "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", stream1);

  // 8. Run Simulation
  Simulator::Stop(Seconds(simulation_time));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}