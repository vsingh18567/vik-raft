#pragma once
#include <string>

class TcpListener {
public:
  virtual void on_message(const std::string &msg) = 0;
};