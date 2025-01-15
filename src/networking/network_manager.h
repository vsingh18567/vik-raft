#include "cluster_members.h"
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
  NetworkManager(const Config &config, int id, MessageHandler *handler,
                 ClusterMembers &cluster_members)
      : server_(cluster_members),
        cluster_members_(cluster_members) { // Load config
    server_.start(config.nodes[id].port, handler);
    LOG(INFO) << "Server started on port " << config.nodes[id].port;

    // Initialize all clients (even for our own id, but leave it as nullptr)
    clients_.reserve(config.cluster_size());
    for (int i = 0; i < config.cluster_size(); i++) {
      if (i != id) {
        clients_.emplace_back(config.nodes[i].host, config.nodes[i].port);
        LOG(INFO) << "Connecting to client " << i << " at "
                  << config.nodes[i].host << ":" << config.nodes[i].port;
        clients_.back().connect();
      } else {
        clients_.emplace_back("", -1);
      }
    }
    LOG(INFO) << "Sleeping for 2 seconds to allow clients to connect";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    for (int i = 0; i < clients_.size(); i++) {
      if (i != id && clients_[i].is_connected()) {
        Connect connect_msg;
        connect_msg.set_id(id);
        Message message;
        message.set_type(MessageType::CONNECT);
        message.set_data(connect_msg.SerializeAsString());
        clients_[i].send(message.SerializeAsString());
      } else if (i != id) {
        LOG(WARNING) << "Client " << i << " not connected. Could not send "
                     << "connect message";
      }
    }
    
  }

  void send_message(const std::string &message) {
    for (int i = 0; i < clients_.size(); i++) {
      if (clients_[i].is_connected() &&
          cluster_members_.is_connected(i)) { // Only send to connected clients
        clients_[i].send(message);
      }
    }
  }

  void send_message(const std::string &message, int to) {
    if (to >= 0 && to < clients_.size() && clients_[to].is_connected() &&
        cluster_members_.is_connected(to)) {
      clients_[to].send(message);
    } else {
      LOG(WARNING) << "Invalid destination or client not connected";
    }
  }

  ~NetworkManager() { server_.stop(); }

private:
  TcpServer server_;
  std::vector<TcpClient> clients_;
  ClusterMembers &cluster_members_;
};
} // namespace vikraft