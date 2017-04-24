/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

#ifndef NFD_DAEMON_TABLE_ITRACE_ENTRY_HPP
#define NFD_DAEMON_TABLE_ITRACE_ENTRY_HPP

#include "face/face.hpp"
#include "core/scheduler.hpp"
#include "table/pit.hpp"

namespace nfd {

namespace itrace {

/** \brief a trace table entry
 *
 *  An Interest table entry represents either a pending Interest or a recently satisfied Interest.
 *  Each entry contains a collection of in-records, a collection of out-records,
 *  and two timers used in forwarding pipelines.
 *  In addition, the entry, in-records, and out-records are subclasses of StrategyInfoHost,
 *  which allows forwarding strategy to store arbitrary information on them.
 *
 *  \todo Store all interests with the same traceName as this entry, set own timer, update timer when trace is renewed
 */
class Entry : noncopyable
{
public:
  Entry(Face& face, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry);

  /** \return the representative Interest of the trace entry
   */
  const Interest&
  getInterest() const
  {
    return *m_interest;
  }

  const shared_ptr<pit::Entry>& 
  getPitEntry() const
  {
    return m_pitEntry;
  }

  /** \return Interest Name
   */
  const Name&
  getTraceName() const
  {
    return m_interest->getTraceName();
  }

  /** \return whether interest matches this entry, i.e., the interest should be pulled by this entry(interest.name == this->traceName)
   *  \param interest the Interest
   */
  bool
  matchesInterest(const shared_ptr<pit::Entry>& pitEntry, uint32_t flag = 0) const;

  /** \return whether interest represents this entry, i.e., the interest has the same traceName(interest.traceName == this->traceName)
   *  \param interest the Interest
   */
  bool
  isEqual(const Interest& interest) const;

public: // face
  /** \return face towards which IFD should be forwarded to
   */
  Face&
  getFace() const
  {
    return *m_face;
  }

  /** \brief updates face
   */
  Face&
  updateFace(Face& face)
  {
      m_face = face.shared_from_this();
      return *m_face;
  }

public: // hmm...
  scheduler::EventId m_timeoutTimer;

private:
  shared_ptr<Face> m_face;
  shared_ptr<const Interest> m_interest;
  shared_ptr<pit::Entry> m_pitEntry;
};

} // namespace trace
} // namespace nfd

#endif // NFD_DAEMON_TABLE_ITRACE_ENTRY_HPP
