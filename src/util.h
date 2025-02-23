#pragma once

#include <iostream>
#include <sstream>
#include <string>

// Log levels
#define LOG_TRACE_LEVEL 0
#define LOG_DEBUG_LEVEL 1
#define LOG_INFO_LEVEL 2
#define LOG_WARN_LEVEL 3
#define LOG_ERROR_LEVEL 4

// Current log level - can be changed at compile time
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO_LEVEL
#endif

// Internal logging macro
#define LOG_INTERNAL(level, level_str, msg)                                    \
  do {                                                                         \
    if (level >= LOG_LEVEL) {                                                  \
      std::stringstream ss;                                                    \
      ss << "[" << level_str << "] " << "["                                    \
         << __FILE__ + std::string(__FILE__).find_last_of("/\\") + 1 << ":"    \
         << __LINE__ << "] " << msg;                                           \
      std::cerr << ss.str() << std::endl;                                      \
    }                                                                          \
  } while (0)

// Public logging macros
#define LOG_TRACE(msg) LOG_INTERNAL(LOG_TRACE_LEVEL, "TRACE", msg)
#define LOG_DEBUG(msg) LOG_INTERNAL(LOG_DEBUG_LEVEL, "DEBUG", msg)
#define LOG_INFO(msg) LOG_INTERNAL(LOG_INFO_LEVEL, "INFO", msg)
#define LOG_WARN(msg) LOG_INTERNAL(LOG_WARN_LEVEL, "WARN", msg)
#define LOG_ERROR(msg) LOG_INTERNAL(LOG_ERROR_LEVEL, "ERROR", msg)
