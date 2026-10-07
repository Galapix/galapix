// LogMich - A Trivial Logging Library
// Copyright (C) 2014 Ingo Ruhnke <grumbel@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "logmich/log.hpp"

#include <iostream>
#include <string>

#if defined(__ANDROID__)
#  include <android/log.h>
#  ifndef LOGMICH_ANDROID_TAG
#    define LOGMICH_ANDROID_TAG "logmich"
#  endif
#endif

namespace logmich {
namespace detail {

std::string_view log_pretty_print(std::string_view str)
{
  // FIXME: very basic, might not work with complex return types
  std::string_view::size_type function_start = 0;
  for(std::string_view::size_type i = 0; i < str.size(); ++i) {
    if (str[i] == ' ') {
      function_start = i+1;
    } else if (str[i] == '(') {
      return str.substr(function_start, i - function_start);
    }
  }
  return str.substr(function_start);
}

} // namespace detail

Logger::Logger() :
#if defined(__ANDROID__)
  // stderr is often invisible under adb logcat; default to DEBUG so early
  // boot messages are visible. Override with set_log_level() as needed.
  m_log_level(LogLevel::DEBUG)
#else
  m_log_level(LogLevel::WARNING)
#endif
{}

void
Logger::incr_log_level(LogLevel level)
{
  if (get_log_level() < level)
  {
    set_log_level(level);
  }
}

void
Logger::set_log_level(LogLevel level)
{
  m_log_level = level;
}

LogLevel
Logger::get_log_level() const
{
  return m_log_level;
}

void
Logger::append(LogLevel level,
               std::string_view file, int line,
               std::string_view msg)
{
  append(std::cerr, level, file, line, msg);
}

void
Logger::append(std::ostream& out,
               LogLevel level,
               std::string_view file, int line,
               std::string_view msg)
{
  switch (level)
  {
    case LogLevel::NONE:    out << "[NONE ] "; break;
    case LogLevel::FATAL:   out << "[FATAL] "; break;
    case LogLevel::ERROR:   out << "[ERROR] "; break;
    case LogLevel::WARNING: out << "[WARN ] "; break;
    case LogLevel::INFO:    out << "[INFO ] "; break;
    case LogLevel::DEBUG:   out << "[DEBUG] "; break;
    case LogLevel::TRACE:   out << "[TRACE] "; break;
  }

  if (msg.empty()) {
    out << file << ":" << line << ": -" << std::endl;
  } else {
    out << file << ":" << line << ": " << msg << std::endl;
  }
#if defined(__ANDROID__)
  {
    int prio = ANDROID_LOG_INFO;
    switch (level) {
      case LogLevel::FATAL:
      case LogLevel::ERROR:   prio = ANDROID_LOG_ERROR; break;
      case LogLevel::WARNING: prio = ANDROID_LOG_WARN;  break;
      case LogLevel::DEBUG:
      case LogLevel::TRACE:   prio = ANDROID_LOG_DEBUG; break;
      default: break;
    }
    std::string composed;
    if (msg.empty()) {
      composed = std::string(file) + ":" + std::to_string(line) + ": -";
    } else {
      composed = std::string(file) + ":" + std::to_string(line) + ": " + std::string(msg);
    }
    __android_log_write(prio, LOGMICH_ANDROID_TAG, composed.c_str());
  }
#endif
}

} // namespace logmich

/* EOF */
