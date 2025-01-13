#include "config.h"
#include "messages.pb.h"
#include "tcp_client.h"
#include "tcp_server.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
namespace vikraft {

class NetworkManager {
public:
  NetworkManager(Config config, int id,
                 MessageHandler *handler) { // Load config
    server_.start(config.nodes[id].port, handler);
    LOG(INFO) << "Server started on port " << config.nodes[id].port;

    // Initialize all clients (even for our own id, but leave it as nullptr)
    clients_.reserve(config.cluster_size());
    for (int i = 0; i < config.cluster_size(); i++) {
      if (i != id) {
        clients_.emplace_back(config.nodes[i].host, config.nodes[i].port);
        clients_.back().connect();
      } else {
        // For our own ID, we'll push a dummy client that won't be used
        clients_.emplace_back("", -1);
      }
    }
  }

  void send_message(const std::string &message) {
    for (int i = 0; i < clients_.size(); i++) {
      if (clients_[i].is_connected()) { // Only send to connected clients
        clients_[i].send(message);
      }
    }
  }

  void send_message(const std::string &message, int to) {
    if (to >= 0 && to < clients_.size() && clients_[to].is_connected()) {
      clients_[to].send(message);
    } else {
      throw std::runtime_error("Invalid destination or client not connected");
    }
  }

  ~NetworkManager() { server_.stop(); }

private:
  TcpServer server_;
  std::vector<TcpClient> clients_;
};
} // namespace vikraft