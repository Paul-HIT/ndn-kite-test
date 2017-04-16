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
    removeTraceEntry(pitEntry);
  }
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
  NFD_LOG_TRACE("afterReceiveInterest");

  if (hasPendingOutRecords(*pitEntry)) {
    // not a new Interest, don't forward
    return;
  }

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  // Ensure there is at least 1 Face is available for forwarding
  if (!hasFaceForForwarding(inFace, nexthops, pitEntry)) {
    this->rejectPendingInterest(pitEntry);
    return;
  }

  std::pair<shared_ptr<trace::Entry>, bool> res;

  if (interest.hasTraceName()) {
    NFD_LOG_INFO("NFD: Receive Interest: " << interest.getName() << " from Face: " << inFace << ", with TraceName: " << interest.getTraceName());
    // In this version, IFI is treated as normal interest, and stored in PIT, when a traceable interest comes, its name will be matched against all IFIs still alive in PIT.
    // In the discussion, there's a new design, where IFI are processed and stored as forwarding info in FIB, the forwarding strategy remains untouched, other than processing IFI and updating FIB entries.
    // Issue with the latter method, FIB is no longer stable, and FIB is mostly updated by RIB daemon, operating FIB within forwarding strategy may be unconventional.
    // But as stated by lixia, the PIT size is used for other purposes as well, so we do have an issue.
    
    pit::InRecordCollection::iterator it = pitEntry->getInRecord(inFace);
    if (it != pitEntry->in_end()) {
      res = m_tt.insert(it->getFace(), interest); // the trace entry will be deleted when the interest expires, actually the IFI's lifetime is the trace entry's lifetime.
      // strategy may be responsible for updating traceEntries lifetime if necessary(interest no longer stays, eg. lifetime == 0, and new field called trace lifetime is added and set)
      NFD_LOG_INFO("NFD: Inserted trace entry with TraceName: " << res.first->getTraceName() << ", Trace Table size: " << m_tt.size());
    }
  }
  else{
    NFD_LOG_INFO("NFD: Receive Interest: " << interest.getName() << " from Face: " << inFace);
  }
  // else {
  //   NFD_LOG_INFO("NFD: Interest has no tracename: " << interest.getName());
  // }

  // testing only...
  // if (interest.getTraceFlag() == 0) { // traceFlag > 0, traceable, pull this interest to IFI, and if successfully pulled && traceFlag == 2, then skip normal forwarding process
  if (interest.getTraceFlag()) { // traceFlag > 0, traceable, pull this interest to IFI, and if successfully pulled && traceFlag == 2, then skip normal forwarding process
    if(Pull(inFace, interest, pitEntry) && interest.getTraceFlag() == 2) {
      return;
    }
  }

  //NFD_LOG_INFO("here");
  // flood it
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
TraceForwardingStrategy::Pull(const Face& inFace, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry)
{
  const shared_ptr<trace::Entry> traceEntry = matchTraceEntry(pitEntry); // the policy is one trace entry for one interest, trace does exact match

  if (traceEntry == nullptr) {
    return false;
  }

  Face& face = traceEntry->getFace();
  if (canForwardToFace(inFace, pitEntry, face)) {
    NFD_LOG_INFO("NFD: Pulling to TraceName: " << traceEntry->getTraceName());
    this->sendInterest(pitEntry, face, interest);
    return true;
  }

  NFD_LOG_INFO("NFD: Can't pull to TraceName: " << traceEntry->getTraceName());

  return false;
}

} // namespace fw
} // namespace nfd