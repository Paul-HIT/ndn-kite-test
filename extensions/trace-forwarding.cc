/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

#include "trace-forwarding.h"

#include "core/logger.hpp"

NFD_LOG_INIT("TraceForwardingStrategy");

namespace nfd {
namespace fw {

const Name
  TraceForwardingStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/trace-forwarding");

TraceForwardingStrategy::TraceForwardingStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
{
}

TraceForwardingStrategy::~TraceForwardingStrategy()
{
}

void
TraceForwardingStrategy::beforeExpirePendingInterest(const shared_ptr<pit::Entry>& pitEntry)
{
  if (pitEntry->getInterest().hasTraceName()) { 
    // may have a trace entry in trace table, erase it.
    // Actually, IFI shouldn't be left in PIT as they don't solicit data, and trace entry should have there own lifetime.
    // Should be implemented in trace table, set timers.
    // If IFI no longer stays in PIT, this will be changed
    NFD_LOG_INFO("NFD: PIT entry expires: " << pitEntry->getInterest().getTraceName());

    if (pitEntry->hasInRecords()){
      /*const pit::InRecordCollection &inRecordCollection = pitEntry->getInRecords();
      for (pit::InRecordCollection::iterator it = inRecordCollection.begin(); it != inRecordCollection.end(); ++ it)
      {
        //cout << * it << ' ' ;
      }*/
      for (pit::InRecordCollection::iterator it = pitEntry->in_begin(); it != pitEntry->in_end(); it ++){
        NFD_LOG_INFO("Face of expire-PIT-entry: " << it->getFace());
      }
      //NFD_LOG_INFO("Face of expire-PIT-entry: " << pitEntry->getInRecords());
    }

    removeTraceEntry(pitEntry);
  }
    removeTFTEntry(pitEntry);
}

static bool
canForwardToFace(const Face& inFace, shared_ptr<pit::Entry> pitEntry, const Face& outFace)
{
  return !wouldViolateScope(inFace, pitEntry->getInterest(), outFace) &&
    canForwardToLegacy(*pitEntry, outFace);
}

static bool
canForwardToNextHop(const Face& inFace, shared_ptr<pit::Entry> pitEntry, const fib::NextHop& nexthop)
{
  return !wouldViolateScope(inFace, pitEntry->getInterest(), nexthop.getFace()) &&
    canForwardToLegacy(*pitEntry, nexthop.getFace());
}

static bool
hasFaceForForwarding(const Face& inFace, const fib::NextHopList& nexthops, const shared_ptr<pit::Entry>& pitEntry)
{
  return std::find_if(nexthops.begin(), nexthops.end(), bind(&canForwardToNextHop, cref(inFace), pitEntry, _1))
         != nexthops.end();
}

void
TraceForwardingStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                                 const shared_ptr<pit::Entry>& pitEntry)
{
  if (interest.hasTraceName()) {
    NFD_LOG_INFO("\nNFD: Receive Interest: " << interest.getName() << " from Face: " << inFace << ", with TraceName: " << interest.getTraceName());
  }
  else{
    NFD_LOG_INFO("\nNFD: Receive Interest: " << interest.getName() << " from Face: " << inFace);
  }

  std::pair<shared_ptr<itrace::Entry>, bool> ires;

  //Insert the interest into the TFT.
  pit::InRecordCollection::iterator TFT_it = pitEntry->getInRecord(inFace);
  if (TFT_it != pitEntry->in_end()) {
    ires = m_itt.insert(TFT_it->getFace(), interest, pitEntry); // the trace entry will be deleted when the interest expires, actually the IFI's lifetime is the trace entry's lifetime.
    // strategy may be responsible for updating traceEntries lifetime if necessary(interest no longer stays, eg. lifetime == 0, and new field called trace lifetime is added and set)
    NFD_LOG_INFO("NFD: Inserted TFT entry with TraceName: " << ires.first->getTraceName() << ", Trace Table size: " << m_itt.size());
  }

  std::pair<shared_ptr<trace::Entry>, bool> res;
  //if the interest has trace name, insert it into the trace table.
  if (interest.hasTraceName()){
    pit::InRecordCollection::iterator it = pitEntry->getInRecord(inFace);
    if (it != pitEntry->in_end()) {
      res = m_tt.insert(it->getFace(), interest, pitEntry); // the trace entry will be deleted when the interest expires, actually the IFI's lifetime is the trace entry's lifetime.
      // strategy may be responsible for updating traceEntries lifetime if necessary(interest no longer stays, eg. lifetime == 0, and new field called trace lifetime is added and set)
      NFD_LOG_INFO("NFD: Inserted trace entry with TraceName: " << res.first->getTraceName() << ", Trace Table size: " << m_tt.size());
    }
  }

  // if interest is traceable with flag=1, check if the interest can pull a tracing interest to its incoming face.
  if (interest.getTraceFlag() == 1) {
    if(!Pull(inFace, interest, pitEntry)) {
      NFD_LOG_INFO("\nNFD: Can't pull interest.");
    }
  }
  // if interest is a tracing interest, to find in pit if there is interests whose name is same with its trace name.
  else if (interest.getTraceFlag() == 2){
    if(forwardByTFT(inFace, interest, pitEntry)){
      return;
    }
  }

  //flood it according to the FIB
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  // Ensure there is at least 1 Face is available for forwarding
  if (!hasFaceForForwarding(inFace, nexthops, pitEntry)) {
    this->rejectPendingInterest(pitEntry);
    return;
  }
  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
    //NFD_LOG_INFO("Faces: " << it->getFace());
    if (canForwardToNextHop(inFace, pitEntry, *it)) {
      //NFD_LOG_INFO("Faces: " << it->getFace());
      this->sendInterest(pitEntry, it->getFace(), interest);
    }
  }
}

void 
TraceForwardingStrategy::beforeSatisfyInterest ( const shared_ptr< pit::Entry > &  pitEntry, const Face &  inFace,
                      const Data &  data) {
  //NFD_LOG_INFO("NOW_HERE");
}

bool
TraceForwardingStrategy::forwardByTFT(const Face& inFace, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry)
{
  const shared_ptr<itrace::Entry> traceEntry = matchTFTEntry(pitEntry);
  if (traceEntry == nullptr) {
    return false;
  }
  const shared_ptr<pit::Entry>& matchedPitEntry = traceEntry->getPitEntry();
  pit::InRecordCollection::iterator it = matchedPitEntry->in_begin();

  int counter = 0;
  for (; it != matchedPitEntry->in_end(); it ++){
    if (it->getFace().getId() != inFace.getId() && canForwardToFace(inFace, pitEntry, it->getFace())){
      this->sendInterest(pitEntry, it->getFace(), interest);
      counter ++;
      NFD_LOG_INFO("out face: " << it->getFace());
    }
  }

  if (counter > 0) {
    return true;
  }

  return false;
}

bool 
TraceForwardingStrategy::Pull(const Face& inFace, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry){
  const shared_ptr<trace::Entry> traceEntry = matchTraceEntry(pitEntry);
  if (traceEntry == nullptr) {
    return false;
  }
  const Interest& traceInterest = traceEntry->getInterest();

  const shared_ptr<pit::Entry>& tracePitEntry = traceEntry->getPitEntry();

  //Face& face = traceEntry->getFace();
  pit::InRecordCollection::iterator it = pitEntry->getInRecord(inFace);


  //if (it != pitEntry->in_end() && canForwardToFace(face, pitEntry, inFace)) {
  if (it != pitEntry->in_end()) {
    NFD_LOG_INFO("NFD: Pulling to TraceName: " << traceEntry->getTraceName() << ", Face: " << it->getFace() << ", Interest: " << traceInterest);
    this->sendInterest(tracePitEntry, it->getFace(), traceInterest);
    return true;
  }

  //NFD_LOG_INFO("NFD: Can't pull to TraceName: " << traceEntry->getTraceName());

  return false;

}

} // namespace fw
} // namespace nfd