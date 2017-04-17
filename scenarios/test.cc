/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

// ndn-simple-kite.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"

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
	//NS_LOG_INFO ("Simulation test");

	LogComponentEnable("nfd.TraceForwardingStrategy", LOG_LEVEL_INFO);

	// Setting default parameters for PointToPoint links and channels
	Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
	Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("20ms"));
	Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("10"));

	int isKite = 1;
	int gridSize = 3;
	int mobileSize = 1;
	int speed = 100;
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

	//set stationary nodes
	NodeContainer nodes;
	nodes.Create(4);

	MobilityHelper mobility;
	Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator> ();
	posAlloc->Add (Vector (0.0, 0.0, 0.0));
	posAlloc->Add (Vector (100.0, 0.0, 0.0));
	posAlloc->Add (Vector (200.0, 100.0, 0.0));
	posAlloc->Add (Vector (200.0, -100.0, 0.0));
	mobility.SetPositionAllocator (posAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (nodes);

	PointToPointHelper p2p;
	p2p.Install(nodes.Get(0), nodes.Get(1));
	p2p.Install(nodes.Get(1), nodes.Get(2));
	p2p.Install(nodes.Get(1), nodes.Get(3));

	/*
	//set mobile-nodes
	Ptr<RandomRectanglePositionAllocator> positionAlloc = CreateObject<RandomRectanglePositionAllocator> ();
	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
	x->SetAttribute ("Min", DoubleValue (150));
	x->SetAttribute ("Max", DoubleValue (250));
	positionAlloc->SetX (x);
	x->SetAttribute ("Min", DoubleValue (-100));
	x->SetAttribute ("Max", DoubleValue (100));
	positionAlloc->SetY (x);

	MobilityHelper a_mobility;
	// a_mobility.SetPositionAllocator(positionAlloc);
	// std::stringstream ss;
	// ss << "ns3::UniformRandomVariable[Min=" << speed << "|Max=" << speed << "]";

	// a_mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
	// 	"Bounds", RectangleValue (Rectangle (150, 250, -100, 100)),
	// 	"Distance", DoubleValue (200),
	// 	"Speed", StringValue (ss.str ()));

	

	NodeContainer mobileNodes;
	mobileNodes.Create (mobileSize);
	// make mobile nodes mobile
	a_mobility.Install (mobileNodes);


	posAlloc = CreateObject<ListPositionAllocator> ();
	posAlloc->Add (Vector (200.0, 80.0, 0.0));
	mobility.SetPositionAllocator (posAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (mobileNodes);
	*/

	//set mobile-nodes
	Ptr<RandomRectanglePositionAllocator> positionAlloc = CreateObject<RandomRectanglePositionAllocator> ();
	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
	x->SetAttribute ("Min", DoubleValue (150));
	x->SetAttribute ("Max", DoubleValue (250));
	positionAlloc->SetX (x);
	x->SetAttribute ("Min", DoubleValue (0));
	x->SetAttribute ("Max", DoubleValue (100));
	positionAlloc->SetY (x);

	MobilityHelper a_mobility;
	a_mobility.SetPositionAllocator(positionAlloc);
	std::stringstream ss;
	ss << "ns3::UniformRandomVariable[Min=" << speed << "|Max=" << speed << "]";

	a_mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
		"Bounds", RectangleValue (Rectangle (249, 250, -100, 100)),
		"Distance", DoubleValue (200),
		"Speed", StringValue (ss.str ()));

	NodeContainer mobileNodes;
	mobileNodes.Create (mobileSize);
	// make mobility move
	a_mobility.Install (mobileNodes);


	//apply wifi component on mobile-nodes and constant-nodes
	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
	// Set to a non-QoS upper mac
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	// Set it to adhoc mode
	wifiMac.SetType ("ns3::AdhocWifiMac");
	// Set Wi-Fi rate manager
	std::string phyMode ("OfdmRate54Mbps");
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue (phyMode), "ControlMode",StringValue (phyMode));

	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
	// ns-3 supports RadioTap and Prism tracing extensions for 802.11
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue (3.0));
	wifiPhy.SetChannel (wifiChannel.Create ());
	wifi.Install (wifiPhy, wifiMac, mobileNodes);
	wifi.Install (wifiPhy, wifiMac, nodes.Get(2));
	wifi.Install (wifiPhy, wifiMac, nodes.Get(3));

	//a previous way to install wifi nodes
	/*YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
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

	wifi.Install (wifiPhy, wifiMac, mobileNodes);
	wifi.Install (wifiPhy, wifiMac, nodes.Get(2));
	wifi.Install (wifiPhy, wifiMac, nodes.Get(3));

	ndn::StackHelper ndnHelper;
	ndnHelper.SetDefaultRoutes(true);
	ndnHelper.InstallAll();*/

	ndn::StackHelper ndnHelper;
	ndnHelper.SetDefaultRoutes(true);
	ndnHelper.InstallAll();

	//ndn::StrategyChoiceHelper::InstallAll<nfd::fw::TraceForwardingStrategy>("/");
	ndn::StrategyChoiceHelper::Install<nfd::fw::TraceForwardingStrategy>(nodes.Get(1), "/");
	ndn::StrategyChoiceHelper::Install<nfd::fw::TraceForwardingStrategy>(nodes.Get(2), "/");
	ndn::StrategyChoiceHelper::Install<nfd::fw::TraceForwardingStrategy>(nodes.Get(3), "/");


	// ndn::FibHelper::AddRoute (nodes.Get(1), serverPrefix, nodes.Get(0), 1);
	// ndn::FibHelper::AddRoute (nodes.Get(1), MobilePrefix, nodes.Get(2), 1);
	// ndn::FibHelper::AddRoute (nodes.Get(1), MobilePrefix, nodes.Get(3), 1);

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
	mobileNodeHelper.Install(mobileNodes.Get(0)); // last node

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
