#pragma once
#include <fstream>
#include <iostream>
#include <string>

std::string getCurrentTime() {
  std::time_t result = std::time(nullptr);
  std::tm *tm_info = std::localtime(&result);
  char buffer[80];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
  return buffer;
}
enum class LogLevel { INFO, WARN, ERROR };

inline std::string levelToString(LogLevel level) {
  switch (level) {
  case LogLevel::INFO:
    return "INFO";
  case LogLevel::WARN:
    return "WARN";
  case LogLevel::ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

std::string LOG_SID = "0";
std::string LOG_FILEPATH = "log_0.txt";
static std::ofstream log_file;

void set_log_file(std::string sid) {
  if (log_file.is_open())
    log_file.close();
  LOG_SID = sid;
  LOG_FILEPATH = "log/log_" + sid + ".txt";
  log_file.open(LOG_FILEPATH);
}

inline void log_to_file(std::string &&msg) {
  log_file << msg << std::endl;
  std::cout << msg << std::endl;
}

#define LOG_LEVEL(msg, lvl)                                                    \
  (log_to_file(getCurrentTime() + " [" + levelToString(lvl) + "] [" +          \
               __FILE__ + ":" + std::to_string(__LINE__) + "] " + msg))
#define LOG(msg) (LOG_LEVEL(msg, LogLevel::INFO))