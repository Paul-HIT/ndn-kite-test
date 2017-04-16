/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

// ndn-simple-kite.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ndn-kite-upload-server.h"
#include "ndn-kite-upload-mobile.h"

#include "trace-forwarding.h"

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

int
main(int argc, char* argv[])
{

  // LogComponentEnable("ndn.Producer", LOG_LEVEL_INFO);
  // LogComponentEnable("ndn.Consumer", LOG_LEVEL_INFO);

  // LogComponentEnable("ndn.kite.KiteUploadServer", LOG_LEVEL_INFO);
  // LogComponentEnable("ndn.kite.KiteUploadMobile", LOG_LEVEL_INFO);

  // LogComponentEnable("nfd.TraceTable", LOG_LEVEL_INFO);
  LogComponentEnable("nfd.TraceForwardingStrategy", LOG_LEVEL_INFO);

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(3);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  // ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll<nfd::fw::TraceForwardingStrategy>("/");

  // Installing applications

  // Stationary server
  ndn::AppHelper serverHelper("ns3::ndn::KiteUploadServer");
  serverHelper.SetPrefix("/mobile");
  serverHelper.SetAttribute("ServerPrefix", StringValue("/server"));
  serverHelper.Install(nodes.Get(0));                        // first node

  // Mobile node
  ndn::AppHelper mobileNodeHelper("ns3::ndn::KiteUploadMobile");
  mobileNodeHelper.SetPrefix("/mobile");
  mobileNodeHelper.SetAttribute("ServerPrefix", StringValue("/server"));
  mobileNodeHelper.SetAttribute("MobilePrefix", StringValue("/mobile"));
  mobileNodeHelper.SetAttribute("PayloadSize", StringValue("1024"));
  mobileNodeHelper.Install(nodes.Get(2)); // last node

  Simulator::Stop(Seconds(20.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
