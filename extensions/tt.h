/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

#ifndef NFD_DAEMON_TABLE_TT_HPP
#define NFD_DAEMON_TABLE_TT_HPP

#include "trace-entry.h"

namespace nfd {
namespace trace {

typedef std::vector<shared_ptr<Entry>>::const_iterator Iterator;

/** \brief represents the trace Table
 */
class Tt : noncopyable
{
public:
  Tt();

  /** \return number of entries
   */
  size_t
  size() const
  {
    return m_entries.size();
  }

  /** \brief matches a trace entry for Interest
   *  \param interest the Interest
   *  \return an existing entry whose traceName equals the Interest's name
   */
  shared_ptr<Entry>
  match(const Interest& interest) const;

  /** \brief finds a trace entry for Interest
   *  \param interest the Interest
   *  \return an existing entry with same traceName
   */
  shared_ptr<Entry>
  find(const Interest& interest) const;

  /** \brief inserts a trace entry for Interest
   *  \param interest the Interest; must be created with make_shared
   *  \return a new or existing entry with same traceName,
   *          and true for new entry, false for existing entry
   */
  std::pair<shared_ptr<Entry>, bool>
  insert(Face& face, const Interest& interest);

  /** \brief deletes an entry
   */
  void
  erase(Entry* entry);  // why bare pointer here?

public: // enumeration
  typedef Iterator const_iterator;

  /** \return an iterator to the beginning
   *  \note Iteration order is implementation-defined.
   *  \warning Undefined behavior may occur if a FIB/PIT/Measurements/StrategyChoice entry
   *           is inserted or erased during enumeration.
   */
  const_iterator
  begin() const;

  /** \return an iterator to the end
   *  \sa begin()
   */
  const_iterator
  end() const;

private:
  std::vector<shared_ptr<Entry>> m_entries;
};

} // namespace trace

using trace::Tt;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_TT_HPP