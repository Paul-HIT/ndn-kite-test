/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

#ifndef NDN_KITE_TRACE_FORWARDING_STRATEGY_HPP
#define NDN_KITE_TRACE_FORWARDING_STRATEGY_HPP

#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/algorithm.hpp"

#include "tt.h" // wanted to name it Trace Information Table, but...

namespace nfd {
namespace fw {

class TraceForwardingStrategy : public Strategy {
public:
  TraceForwardingStrategy(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  virtual ~TraceForwardingStrategy() override;

  virtual void
  beforeExpirePendingInterest(const shared_ptr<pit::Entry>& pitEntry);

  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;
  virtual void 
  beforeSatisfyInterest ( const shared_ptr< pit::Entry > &  pitEntry, const Face &  inFace,
                      const Data &  data) override;
  
  bool forwardByTFT(const Face& inFace, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry);

  bool Pull(const Face& inFace, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry);

protected:

  const shared_ptr<trace::Entry>
  matchTraceEntry(const shared_ptr<pit::Entry>& pitEntry, uint32_t flag = 0)
  {
    return m_tt.match(pitEntry->getInterest(), flag);
  }

  void
  removeTraceEntry(const shared_ptr<pit::Entry>& pitEntry)
  {
    shared_ptr<trace::Entry> traceEntry = findTraceEntry(pitEntry);
    if (traceEntry == nullptr) {
      return;
    }
    m_tt.erase(traceEntry.get());
  }

  const shared_ptr<trace::Entry>
  findTraceEntry(const shared_ptr<pit::Entry>& pitEntry)
  {
    return m_tt.find(pitEntry->getInterest());
  }

public:
  static const Name STRATEGY_NAME;

private:
  trace::Tt m_tt;
};

} // namespace fw
} // namespace nfd

#endif // NDN_KITE_TRACE_FORWARDING_STRATEGY_HPP