#pragma once
#include "messages.h"
#include <string>
class TcpListener {
public:
  virtual void on_message(Message m) = 0;
};