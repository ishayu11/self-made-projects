/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <map>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("TcpAssignmentQ3");

static void CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldCwnd << "\t" << newCwnd << endl;
}

static void TraceCwnd(string context, Ptr<OutputStreamWrapper> stream)
{
  Config::ConnectWithoutContext(context, MakeBoundCallback(&CwndChange, stream));
}

int main(int argc, char* argv[])
{
  double simulation_time = 10.0;
  uint32_t payload_size = 1460;

  CommandLine cmd(__FILE__);
  cmd.Parse(argc, argv);

  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payload_size));
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
  p2pBottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("50p"));

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

  // 6. Setup Applications & Tracing
  p2pBottleneck.EnablePcapAll("scratch/combined/q3/bottleneck_q3");
  AsciiTraceHelper asciiTraceHelper;

  // USER 1 (Node 0 -> Node 2): 3 PARALLEL FLOWS
  uint16_t ports_n0[3] = {8080, 8081, 8082};
  for (int i = 0; i < 3; i++) {
      PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ports_n0[i]));
      ApplicationContainer sinkApp = sink.Install(n2n3.Get(0));
      sinkApp.Start(Seconds(0.0));
      sinkApp.Stop(Seconds(simulation_time));

      BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(i_r2n2.GetAddress(1), ports_n0[i]));
      source.SetAttribute("MaxBytes", UintegerValue(0));
      ApplicationContainer sourceApp = source.Install(n0n1.Get(0));
      sourceApp.Start(Seconds(1.0));
      sourceApp.Stop(Seconds(simulation_time));

      // Trace each of the 3 flows individually
      Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream("scratch/combined/q3/flow_n0_" + to_string(i) + ".cwnd");
      Simulator::Schedule(Seconds(1.1), &TraceCwnd, "/NodeList/0/$ns3::TcpL4Protocol/SocketList/" + to_string(i) + "/CongestionWindow", stream);
  }

  // USER 2 (Node 1 -> Node 3): 1 SINGLE FLOW
  uint16_t port_n1 = 8083;
  PacketSinkHelper sink2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port_n1));
  ApplicationContainer sinkApp2 = sink2.Install(n2n3.Get(1));
  sinkApp2.Start(Seconds(0.0));
  sinkApp2.Stop(Seconds(simulation_time));

  BulkSendHelper source2("ns3::TcpSocketFactory", InetSocketAddress(i_r2n3.GetAddress(1), port_n1));
  source2.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer sourceApp2 = source2.Install(n0n1.Get(1));
  sourceApp2.Start(Seconds(1.0));
  sourceApp2.Stop(Seconds(simulation_time));

  // Trace the single flow from Node 1
  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream("scratch/combined/q3/flow_n1.cwnd");
  Simulator::Schedule(Seconds(1.1), &TraceCwnd, "/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", stream2);

  // 7. FlowMonitor
  FlowMonitorHelper flowHelper;
  Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

  Simulator::Stop(Seconds(simulation_time));
  Simulator::Run();

  // 8. Results Output
  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
  map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  cout << "\n==============================================\n";
  cout << "=== Simulation Results (3 Flows vs 1 Flow) ===\n";
  cout << "==============================================\n";

  double total_n0_throughput = 0.0;
  double total_n1_throughput = 0.0;

  for (auto const& [id, stat] : stats) {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(id);
      double duration = stat.timeLastRxPacket.GetSeconds() - stat.timeFirstTxPacket.GetSeconds();
      double throughput = (duration > 0) ? (stat.rxBytes * 8.0) / (duration * 1e6) : 0;

      if(throughput > 0.5) { // Filter out empty ACK flows
          cout << "Flow " << id << " (" << t.sourceAddress << " -> " << t.destinationAddress << ") - " << throughput << " Mbps\n";

          if (t.sourceAddress == "10.1.1.1") total_n0_throughput += throughput;
          if (t.sourceAddress == "10.1.2.1") total_n1_throughput += throughput;
      }
  }

  cout << "\n--- Final Bandwidth Breakdown ---\n";
  cout << "User 1 (Node 0 - 3 Flows) Total: " << total_n0_throughput << " Mbps\n";
  cout << "User 2 (Node 1 - 1 Flow ) Total: " << total_n1_throughput << " Mbps\n";
  cout << "==============================================\n";

  Simulator::Destroy();
  return 0;
}