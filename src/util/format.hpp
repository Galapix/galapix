#ifndef HEADER_GALAPIX_UTIL_FORMAT_HPP
#define HEADER_GALAPIX_UTIL_FORMAT_HPP

#include <boost/format.hpp>
#include <ostream>
#include <string>

inline void format(boost::format& fmt)
{
}

template<typename Head, typename ...Rest>
inline void format(boost::format& fmt, const Head& head, Rest... rest)
{
  format(fmt % head, rest...);
}

template<typename ...Arg>
inline std::string format(const std::string& str, Arg... arg)
{
  boost::format fmt(str);
  format(fmt, arg...);
  return fmt.str();
}

template<typename ...Arg>
inline void format(std::ostream& out, const std::string& str, Arg... arg)
{
  boost::format fmt(str);
  format(fmt, arg...);
  out << fmt;
}

#endif

/* EOF */
