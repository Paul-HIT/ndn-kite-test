/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

#include "itt.h"

NFD_LOG_INIT("iTraceTable");

namespace nfd {
namespace itrace {

Itt::Itt()
{
}

shared_ptr<Entry>
Itt::match(const shared_ptr<pit::Entry>& pitEntry, uint32_t flag) const
{
  //const Interest& interest = pitEntry->getInterest();
  // check if trace entry already exists
  auto it = std::find_if(m_entries.begin(), m_entries.end(),
    [&pitEntry, flag] (const shared_ptr<Entry>& entry) {
      // initial part of name is guaranteed to be equal by NameTree
      // check implicit digest (or its absence) only
      return entry->matchesInterest(pitEntry, flag);
    });

  if (it != m_entries.end()) {
    NFD_LOG_INFO("ITT: Match found on TraceName: " << (*it)->getInterest().getTraceName());
    return *it;
  }
  else {
    return nullptr;
  }
}

shared_ptr<Entry>
Itt::find(const Interest& interest) const
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
    NFD_LOG_INFO("ITT: Found entry with TraceName: " << (*it)->getInterest().getTraceName());
    return *it;
  }
  else {
    return nullptr;
  }
}


std::pair<shared_ptr<Entry>, bool>
Itt::insert(Face& face, const Interest& interest, const shared_ptr<pit::Entry>& pitEntry)
{
  BOOST_ASSERT(interest.hasTraceName());

  shared_ptr<Entry> entry= find(interest);
  //if (entry != nullptr && (interest.getName()).compare((entry->getInterest()).getName())) {
  if (entry != nullptr) {
    if (entry->getFace().getId() == face.getId()) {
      return {entry, false};
    }
    else{
      entry->getPitEntry()->insertOrUpdateInRecord(face, interest);
      NFD_LOG_INFO("ITT: No match, adding entry for Interest Name: " << interest.getName() << ", table size: " << size());
      return {entry, true};
    }
  }
  else{
    NFD_LOG_INFO("ITT: No match, adding entry for TraceName: " << interest.getTraceName() << ", table size: " << size());

    entry = shared_ptr<Entry>(new Entry(face, interest, pitEntry));
    NFD_LOG_INFO("ITT: Entry created with TraceName: " << entry->getInterest().getTraceName());

    m_entries.push_back(entry);
    return {entry, true};
  }
}

void
Itt::erase(Entry* entry)
{
  auto it = std::find_if(m_entries.begin(), m_entries.end(),
    [entry] (const shared_ptr<Entry>& t_entry) {
      // initial part of name is guaranteed to be equal by NameTree
      // check implicit digest (or its absence) only
      return (t_entry.get() == entry);
    });
  if (it != m_entries.end()) {
    if (*it != nullptr) {
      NFD_LOG_INFO("ITT: Erasing entry with TraceName: " << (*it)->getTraceName());
      (*it).reset(); // right?
    }
    m_entries.erase(it);
  }
}

Itt::const_iterator
Itt::begin() const
{
  return m_entries.begin();
}

Itt::const_iterator
Itt::end() const
{
  return m_entries.end();
}

} // namespace Tt
} // namespace nfd
