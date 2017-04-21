/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

#include "tt.h"

NFD_LOG_INIT("TraceTable");

namespace nfd {
namespace trace {

Tt::Tt()
{
}

shared_ptr<Entry>
Tt::match(const Interest& interest, uint32_t flag) const
{
  // check if trace entry already exists
  auto it = std::find_if(m_entries.begin(), m_entries.end(),
    [&interest, flag] (const shared_ptr<Entry>& entry) {
      // initial part of name is guaranteed to be equal by NameTree
      // check implicit digest (or its absence) only
      return entry->matchesInterest(interest, flag);
    });

  if (it != m_entries.end()) {
    NFD_LOG_INFO("TT: Match found on TraceName: " << (*it)->getInterest().getTraceName());
    return *it;
  }
  else {
    return nullptr;
  }
}

shared_ptr<Entry>
Tt::find(const Interest& interest) const
{
  BOOST_ASSERT(interest.hasTraceName());
  // check if trace entry already exists
  auto it = std::find_if(m_entries.begin(), m_entries.end(),
    [&interest] (const shared_ptr<Entry>& entry) {
      // initial part of name is guaranteed to be equal by NameTree
      // check implicit digest (or its absence) only
      return entry->isEqual(interest);
    });

  if (it != m_entries.end()) {
    NFD_LOG_INFO("TT: Found entry with TraceName: " << (*it)->getInterest().getTraceName());
    return *it;
  }
  else {
    return nullptr;
  }
}


std::pair<shared_ptr<Entry>, bool>
Tt::insert(Face& face, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry)
{
  BOOST_ASSERT(interest.hasTraceName());

  shared_ptr<Entry> entry= find(interest);
  if (entry != nullptr) {
    return {entry, false};
  }

  NFD_LOG_INFO("TT: No match, adding entry for TraceName: " << interest.getTraceName() << ", table size: " << size());

  entry = shared_ptr<Entry>(new Entry(face, interest, pitEntry));
  NFD_LOG_INFO("TT: Entry created with TraceName: " << entry->getInterest().getTraceName());

  m_entries.push_back(entry);
  return {entry, true};
}

void
Tt::erase(Entry* entry)
{
  auto it = std::find_if(m_entries.begin(), m_entries.end(),
    [entry] (const shared_ptr<Entry>& t_entry) {
      // initial part of name is guaranteed to be equal by NameTree
      // check implicit digest (or its absence) only
      return (t_entry.get() == entry);
    });
  if (it != m_entries.end()) {
    if (*it != nullptr) {
      NFD_LOG_INFO("TT: Erasing entry with TraceName: " << (*it)->getTraceName());
      (*it).reset(); // right?
    }
    m_entries.erase(it);
  }
}

Tt::const_iterator
Tt::begin() const
{
  return m_entries.begin();
}

Tt::const_iterator
Tt::end() const
{
  return m_entries.end();
}

} // namespace Tt
} // namespace nfd
