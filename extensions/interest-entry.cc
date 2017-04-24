/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

#include "interest-entry.h"
#include <algorithm>

namespace nfd {
namespace itrace {

Entry::Entry(Face& face, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry)
  : m_face(face.shared_from_this())
  , m_interest(interest.shared_from_this()) // make sure that this interest has traceName
  , m_pitEntry(pitEntry)
{
}

bool
Entry::matchesInterest(const shared_ptr<pit::Entry>& pitEntry, uint32_t flag) const
{
  const Interest& interest = pitEntry->getInterest();

	if (flag == 0) {
    if ((m_interest->getName()).compare(0, Name::npos, interest.getTraceName(), 0) == 0){
      return true;
    }
    else{
      return false;
    }

	}
	else{
		return m_interest->getTraceName().compare(0, Name::npos, interest.getTraceName(), 0) == 0;
	}
}

bool
Entry::isEqual(const Interest& interest) const
{

  return (m_interest->getName().compare(0, Name::npos, interest.getName(), 0) == 0);
  
}

} // namespace trace
} // namespace nfd
