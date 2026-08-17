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

NS_LOG_COMPONENT_DEFINE("TcpAssignmentQ2");

// Function to log CWND changes
static void CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldCwnd << "\t" << newCwnd << endl;
}

// Function to connect the trace
static void TraceCwnd(string context, Ptr<OutputStreamWrapper> stream)
{
  Config::ConnectWithoutContext(context, MakeBoundCallback(&CwndChange, stream));
}

int main(int argc, char* argv[])
{
  string tcp_variant = "TcpCubic";
  double simulation_time = 10.0;
  uint32_t payload_size = 1460;

  CommandLine cmd(__FILE__);
  cmd.Parse(argc, argv);

  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payload_size));

  // FIX: Increase TCP buffer sizes so the sender can push hard enough to overflow the bottleneck queue
  Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1000000));
  Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1000000));

  // 1. Create Nodes
  NodeContainer n0n1, r1r2, n2n3;
  n0n1.Create(2);
  r1r2.Create(2);
  n2n3.Create(2);

  // 2. Configure Link Helpers
  PointToPointHelper p2pEdge;
  p2pEdge.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  p2pEdge.SetChannelAttribute("Delay", StringValue("2ms"));
  p2pEdge.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1000p"));

  PointToPointHelper p2pBottleneck;
  p2pBottleneck.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2pBottleneck.SetChannelAttribute("Delay", StringValue("20ms"));
  // FIX: Reduced queue to 50p to ensure a sharp, visible sawtooth pattern for competing flows
  p2pBottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("200p"));

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
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i_n0r1 = address.Assign(d_n0r1);

  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i_n1r1 = address.Assign(d_n1r1);

  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i_r1r2 = address.Assign(d_r1r2);

  address.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i_r2n2 = address.Assign(d_r2n2);

  address.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i_r2n3 = address.Assign(d_r2n3);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // 6. Setup Applications
  uint16_t port1 = 8080;
  uint16_t port2 = 8081;

  // FLOW 1: n0 -> n2
  PacketSinkHelper sinkHelper1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port1));
  ApplicationContainer sinkApp1 = sinkHelper1.Install(n2n3.Get(0));
  sinkApp1.Start(Seconds(0.0));
  sinkApp1.Stop(Seconds(simulation_time));

  Address sinkAddress1(InetSocketAddress(i_r2n2.GetAddress(1), port1));
  BulkSendHelper sourceHelper1("ns3::TcpSocketFactory", sinkAddress1);
  sourceHelper1.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer sourceApp1 = sourceHelper1.Install(n0n1.Get(0));
  sourceApp1.Start(Seconds(1.0));
  sourceApp1.Stop(Seconds(simulation_time));

  // FLOW 2: n1 -> n3 (The Competing Flow)
  PacketSinkHelper sinkHelper2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port2));
  ApplicationContainer sinkApp2 = sinkHelper2.Install(n2n3.Get(1));
  sinkApp2.Start(Seconds(0.0));
  sinkApp2.Stop(Seconds(simulation_time));

  Address sinkAddress2(InetSocketAddress(i_r2n3.GetAddress(1), port2));
  BulkSendHelper sourceHelper2("ns3::TcpSocketFactory", sinkAddress2);
  sourceHelper2.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer sourceApp2 = sourceHelper2.Install(n0n1.Get(1));
  sourceApp2.Start(Seconds(1.0));
  sourceApp2.Stop(Seconds(simulation_time));

  // 7. Tracing PCAP and CWND
  p2pBottleneck.EnablePcapAll("scratch/combined/q2/bottleneck_q2");
  AsciiTraceHelper asciiTraceHelper;

  // Trace Flow 1 (Node 0)
  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream("scratch/combined/q2/flow1.cwnd");
  Simulator::Schedule(Seconds(1.1), &TraceCwnd, "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", stream1);

  // Trace Flow 2 (Node 1)
  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream("scratch/combined/q2/flow2.cwnd");
  Simulator::Schedule(Seconds(1.1), &TraceCwnd, "/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", stream2);

  // 8. FlowMonitor
  FlowMonitorHelper flowHelper;
  Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

  Simulator::Stop(Seconds(simulation_time));
  Simulator::Run();

  // 9. Results Output
  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
  map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  cout << "\n=== Simulation Results (Q2: 2 Competing Flows) ===\n";
  for (auto const& [id, stat] : stats) {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(id);
      double duration = stat.timeLastRxPacket.GetSeconds() - stat.timeFirstTxPacket.GetSeconds();
      double throughput = (duration > 0) ? (stat.rxBytes * 8.0) / (duration * 1e6) : 0;

      if(throughput > 1.0) { // Filter out tiny ACK flows
          cout << "Flow " << id << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          cout << "  Throughput: " << throughput << " Mbps\n";
      }
  }

  Simulator::Destroy();
  return 0;
}