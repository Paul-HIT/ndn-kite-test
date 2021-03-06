/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

#include "trace-entry.h"
#include <algorithm>

namespace nfd {
namespace trace {

Entry::Entry(Face& face, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry)
  : m_face(face.shared_from_this())
  , m_interest(interest.shared_from_this()) // make sure that this interest has traceName
  , m_pitEntry(pitEntry)
{
}

bool
Entry::matchesInterest(const Interest& interest, uint32_t flag) const
{
	if (flag == 0) {
		return m_interest->getTraceName().compare(0, Name::npos, interest.getName(), 0) == 0;
	}
	else{
		return m_interest->getTraceName().compare(0, Name::npos, interest.getTraceName(), 0) == 0;
	}
}

bool
Entry::isEqual(const Interest& interest) const
{
  if (!interest.hasTraceName()) {
    return false;
  }

  return m_interest->getTraceName().compare(0, Name::npos,
                                            interest.getTraceName(), 0) == 0;
}

} // namespace trace
} // namespace nfd
