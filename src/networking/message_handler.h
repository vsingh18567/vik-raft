#pragma once

#include "messages.pb.h"
#include "node/node_id.h"

namespace vikraft {

class MessageHandler {
public:
  virtual void handle_message(const Message &message, NodeId from,
                              int client_fd) = 0;
};
} // namespace vikraft