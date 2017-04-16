/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"

#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "helper/ndn-face-container.hpp"

#include "ns3/ndnSIM-module.h"

#include "ndn-kite-upload-server.h"
#include "ndn-kite-upload-mobile.h"

#include "trace-forwarding.h"

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("NdnPull");

//
// DISCLAIMER:  Note that this is an extremely simple example, containing just 2 wifi nodes
// communicating directly over AdHoc channel.
//

// Ptr<ndn::NetDeviceFace>
// MyNetDeviceFaceCallback (Ptr<Node> node, Ptr<ndn::L3Protocol> ndn, Ptr<NetDevice> device)
// {
//   // NS_LOG_DEBUG ("Create custom network device " << node->GetId ());
//   Ptr<ndn::NetDeviceFace> face = CreateObject<ndn::MyNetDeviceFace> (node, device);
//   ndn->AddFace (face);
//   return face;
// }

int
main(int argc, char* argv[])
{
  NS_LOG_INFO ("Start Sim...");

  LogComponentEnable("ndn.kite.KiteUploadServer", LOG_LEVEL_INFO);
  LogComponentEnable("ndn.kite.KiteUploadMobile", LOG_LEVEL_INFO);

  LogComponentEnable("nfd.TraceForwardingStrategy", LOG_LEVEL_INFO);

  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("20ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("10"));

  // disable fragmentation
  // Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
  // Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
  // Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
  //                    StringValue("OfdmRate24Mbps"));


  int isKite = 1;
  int gridSize = 3;
  int mobileSize = 1;
  int speed = 20;
  int stopTime = 100;
  int joinTime = 1;

  CommandLine cmd;
  cmd.AddValue("kite", "enable Kite", isKite);
  cmd.AddValue("speed", "mobile speed m/s", speed);
  cmd.AddValue("size", "# mobile", mobileSize);
  cmd.AddValue("grid", "grid size", gridSize);  
  cmd.AddValue("stop", "stop time", stopTime);  
  cmd.AddValue("join", "join period", joinTime);  
  cmd.Parse (argc, argv);

  //////////////////////
  //////////////////////
  //////////////////////
  // WifiHelper wifi = WifiHelper::Default();
  // wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  // wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
  // wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
  //                              StringValue("OfdmRate24Mbps"));

  // YansWifiChannelHelper wifiChannel; // = YansWifiChannelHelper::Default ();
  // wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  // wifiChannel.AddPropagationLoss("ns3::ThreeLogDistancePropagationLossModel");
  // wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel");

  // YansWifiPhy wifiPhy = YansWifiPhy::Default();
  // YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default();
  // wifiPhyHelper.SetChannel(wifiChannel.Create());
  // wifiPhyHelper.Set("TxPowerStart", DoubleValue(5));
  // wifiPhyHelper.Set("TxPowerEnd", DoubleValue(5));

  // NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default();
  // wifiMacHelper.SetType("ns3::AdhocWifiMac");

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set ("TxPowerStart",DoubleValue(5));
  wifiPhy.Set ("TxPowerEnd",DoubleValue(5));
  wifiPhy.Set ("TxPowerLevels",UintegerValue (1));
  wifiPhy.Set ("TxGain",DoubleValue (1));
  wifiPhy.Set ("RxGain",DoubleValue (1));

  WifiHelper wifi = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");
  wifiMac.SetType("ns3::AdhocWifiMac");

  // wifi.Install( wifiPhy, wifiMac, wifiNodes.GetGlobal());
  
  // make it move
  Ptr<RandomRectanglePositionAllocator> positionAlloc = CreateObject<RandomRectanglePositionAllocator> ();
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (10));
  x->SetAttribute ("Max", DoubleValue (30));
  positionAlloc->SetX (x);
  positionAlloc->SetY (x);

  MobilityHelper mobility;
  mobility.SetPositionAllocator(positionAlloc);
  std::stringstream ss;
  ss << "ns3::UniformRandomVariable[Min=" << speed << "|Max=" << speed << "]";

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    "Bounds", RectangleValue (Rectangle (0, 40, -30, 30)),
    "Distance", DoubleValue (60),
    "Speed", StringValue (ss.str ()));

  // prefixes
  std::string serverPrefix = "/server/upload";
  std::string mobilePrefix = "/mobile/file";

  // gen nodes, a p2p grid, maybe faulty
  /*PointToPointHelper p2p;
  PointToPointGridHelper grid (gridSize, gridSize, p2p);
  grid.BoundingBox(0,0,400,400);*/// Creating nodes
  NodeContainer nodes;
  nodes.Create(4);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(1), nodes.Get(3));


  NodeContainer wifiNodes;
  NodeContainer mobileNodes;
  mobileNodes.Create (mobileSize);

  // make mobile nodes mobile
  mobility.Install (mobileNodes);

  // install wifi
  //NS_LOG_INFO(wifiNodes.GetGlobal());

  //wifi.Install (wifiPhy, wifiMac, nodes.GetGlobal());
  wifi.Install (wifiPhy, wifiMac, nodes.Get(2));
  wifi.Install (wifiPhy, wifiMac, nodes.Get(3));
  wifi.Install (wifiPhy, wifiMac, mobileNodes.Get(0));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();
  // install separately
  // ndnHelper.InstallAll();

  // Set BestRoute strategy
  // ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  // multicat strategy
  // ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");
  // pit forwarding strategy

  //Ptr<Node> serverNode = nodes.Get(0);

  //ndn::StrategyChoiceHelper::InstallAll<nfd::fw::TraceForwardingStrategy>("/");
  ndn::StrategyChoiceHelper::Install(nodes.Get(2), "/", "nfd::fw::TraceForwardingStrategy");
  ndn::StrategyChoiceHelper::Install(nodes.Get(3), "/", "nfd::fw::TraceForwardingStrategy");
  ndn::StrategyChoiceHelper::Install(nodes.Get(1), "/", "nfd::fw::TraceForwardingStrategy");
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
  mobileNodeHelper.Install(mobileNodes); // last node


  ndn::FibHelper::AddRoute (nodes.Get(3), "/", nodes.Get(1), 1);
  ndn::FibHelper::AddRoute (nodes.Get(2), "/", nodes.Get(1), 1);
  ndn::FibHelper::AddRoute (nodes.Get(1), "/", nodes.Get(0), 1);

  //Ptr<Node> userNode = grid.GetNode(gridSize - 1, gridSize - 1);
  
  /*Ptr<ndn::FaceContainer> faces = ndnHelper.Install (mobileNodes);
  for (uint32_t i = 0;
       i != faces->GetN();
       i++)
  {
    std::shared_ptr<ndn::Face> face = faces->Get(i);
    ndn::FibHelper::AddRoute (//face->GetNode ()
    mobileNodes.Get(0), serverPrefix, face, 1);
  }*/


  /*for (int i = 0; i<gridSize; i++)
  {
    for (int j = 0; j<gridSize; j++)
    {
        ndnHelper.Install (grid.GetNode (i, j));
        if (i ==0 && j==0) continue;
        int m = i>j ? (i - 1) : i;
        int n = i>j ? j : (j - 1);
        ndn::FibHelper::AddRoute (grid.GetNode (i, j), serverPrefix, grid.GetNode (m, n), 1);
    }
  }

  ndn::StrategyChoiceHelper::InstallAll<ndn::nfd::fw::TraceForwardingStrategy>("/");

  // 4. Set up applications
  NS_LOG_INFO("Installing Applications");

  ndn::AppHelper consumerHelper("ServerApp");
  // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // consumerHelper.SetPrefix(mobilePrefix);
  // consumerHelper.SetAttribute("Frequency", StringValue("1"));
  consumerHelper.Install(serverNode);
  // ApplicationContainer consumerApp = consumerHelper.Install(rpNode);
  // consumerApp.Start(Seconds(0));

  ndn::AppHelper producerHelper("MobileApp");
  // ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // producerHelper.SetPrefix(mobilePrefix);
  // producerHelper.SetAttribute("PayloadSize", StringValue("1200"));
  // producerHelper.Install(userNode);
  producerHelper.Install(mobileNodes);
  // ApplicationContainer producerApp = producerHelper.Install(mobileNode);
  // producerApp.Start(Seconds(1));

  // Installing global routing interface on all nodes
  // ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  // ndnGlobalRoutingHelper.InstallAll();


  // Add /prefix origins to ndn::GlobalRouter
  // ndnGlobalRoutingHelper.AddOrigins(serverPrefix, serverNode);
  // ndnGlobalRoutingHelper.AddOrigins(mobilePrefix, mobileNodes);
  // ndnGlobalRoutingHelper.AddOrigins(mobilePrefix, userNode);

  // Calculate and install FIBs
  // ndn::GlobalRoutingHelper::CalculateRoutes();

  ////////////////
*/
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
