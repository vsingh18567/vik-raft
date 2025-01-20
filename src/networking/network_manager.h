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

class NodeNetworkManager {
public:
  NodeNetworkManager(const Config &config, int id, MessageHandler *handler,
                     ClusterMembers &cluster_members)
      : my_id(id),
        server_(
            cluster_members, [this](NodeId id_) { on_reconnect(id_); },
            [this](NodeId id_) { on_disconnect(id_); }),
        cluster_members_(cluster_members) {
    server_.start(config.nodes[id].port, handler);
    LOG(INFO) << "Server started on port " << config.nodes[id].port;

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
        send_connect_message(i);
      } else if (i != id) {
        LOG(WARNING) << "Client " << i << " not connected. Could not send "
                     << "connect message";
      }
    }
  }
  void send_server_response(const std::string &message, int client_fd) {
    server_.send_message(client_fd, message);
  }

  void send_message(const std::string &message) {
    for (int i = 0; i < clients_.size(); i++) {
      if (clients_[i].is_connected()) {
        clients_[i].send(message);
      }
    }
  }

  void send_message(const std::string &message, int to) {
    if (to >= 0 && to < clients_.size() && clients_[to].is_connected()) {
      clients_[to].send(message);
    } else {
      LOG(WARNING) << "Invalid destination or client not connected";
    }
  }

  ~NodeNetworkManager() { server_.stop(); }

private:
  void send_connect_message(NodeId to) {
    LOG(INFO) << "Sending connect message to client " << to;
    Connect connect_msg;
    connect_msg.set_id(my_id);
    Message message;
    message.set_type(MessageType::CONNECT);
    message.set_data(connect_msg.SerializeAsString());
    clients_[to].send(message.SerializeAsString());
  }

  void on_reconnect(NodeId id) {
    if (!clients_[id].is_connected()) {
      clients_[id].connect();
      send_connect_message(id);
    }
    cluster_members_.add_node(id); // deal with initial connection
  }

  void on_disconnect(NodeId id) {
    clients_[id].disconnect();
    cluster_members_.remove_node(id);
  }
  NodeId my_id;
  NodeTcpServer server_;
  std::vector<TcpClient> clients_;
  ClusterMembers &cluster_members_;
};
} // namespace vikraft