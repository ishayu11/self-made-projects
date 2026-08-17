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

NS_LOG_COMPONENT_DEFINE("TcpAssignmentQ1");

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
  string tcp_variant = "TcpNewReno";
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
  // FIX: Reduced queue to 50p to ensure a sharp, visible sawtooth pattern
  p2pBottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("50p"));

  // 3. Install NetDevices
  NetDeviceContainer d_n0r1 = p2pEdge.Install(n0n1.Get(0), r1r2.Get(0));
  NetDeviceContainer d_r1r2 = p2pBottleneck.Install(r1r2.Get(0), r1r2.Get(1));
  NetDeviceContainer d_r2n2 = p2pEdge.Install(r1r2.Get(1), n2n3.Get(0));

  // 4. Install Internet Stack
  InternetStackHelper stack;
  stack.InstallAll();

  // 5. Assign IP Addresses
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i_n0r1 = address.Assign(d_n0r1);

  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i_r1r2 = address.Assign(d_r1r2);

  address.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i_r2n2 = address.Assign(d_r2n2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // 6. Setup Application (SINGLE FLOW: n0 -> n2)
  uint16_t port = 8080;

  PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer sinkApp = sinkHelper.Install(n2n3.Get(0));
  sinkApp.Start(Seconds(0.0));
  sinkApp.Stop(Seconds(simulation_time));

  Address sinkAddress(InetSocketAddress(i_r2n2.GetAddress(1), port));
  BulkSendHelper sourceHelper("ns3::TcpSocketFactory", sinkAddress);
  sourceHelper.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer sourceApp = sourceHelper.Install(n0n1.Get(0));
  sourceApp.Start(Seconds(1.0));
  sourceApp.Stop(Seconds(simulation_time));

  // 7. Tracing PCAP and CWND
  p2pBottleneck.EnablePcapAll("scratch/combined/ns3-projects/bottleneck_q1");
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream("scratch/combined/ns3-projects/q1_single.cwnd");
  Simulator::Schedule(Seconds(1.1), &TraceCwnd, "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", stream);

  // 8. FlowMonitor
  FlowMonitorHelper flowHelper;
  Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

  Simulator::Stop(Seconds(simulation_time));
  Simulator::Run();

  // 9. Results Output
  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
  map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  cout << "\n=== Simulation Results (Q1: Single Flow) ===\n";
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