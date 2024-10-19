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

void LOG(const std::string &msg, LogLevel level) {
  std::cout << getCurrentTime() << " [" << levelToString(level) << "] " << msg
            << std::endl;
}
void LOG(const std::string &msg) { LOG(msg, LogLevel::INFO); }
