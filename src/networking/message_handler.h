#pragma once

#include "messages.pb.h"
#include "node_id.h"

namespace vikraft {

class MessageHandler {
public:
  virtual void handle_message(const Message &message, NodeId from) = 0;
};
} // namespace vikraft