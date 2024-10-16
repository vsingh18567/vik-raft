#pragma once
#include "tcp/tcp_client.h"
#include "tcp/tcp_listener.h"
#include "tcp/tcp_server.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class ServerState { LEADER, FOLLOWER, CANDIDATE };

class ServerNode : public TcpListener {
public:
  ServerNode(const std::string &config_file, int id) : sid(id) {
    // read config file
    // set up connections
    // start election timer
    std::ifstream file(config_file);
    std::string line;
    while (std::getline(file, line)) {
      int comma = line.find(",");
      int this_id = std::stoi(line.substr(0, comma));
      std::string address_port = line.substr(comma + 1);
      int colon = address_port.find(":");
      std::string address = address_port.substr(0, colon);
      int port = std::stoi(address_port.substr(colon + 1));
      if (this_id == sid) {
        server = std::make_unique<TcpServer>(port);
        server->add_listener(this);
        std::thread(TcpServer::open, std::ref(*server)).detach();
      } else {
        auto client = std::make_unique<TcpClient>(address, port);
        clients.emplace(this_id, std::move(client));
      }
    }
  }

  void on_message(const std::string &msg) override {
    std::cout << "id: " << sid << " Received message: " << msg << std::endl;
  }

  void send_message(int id, const std::string &msg) {
    std::cout << "id: " << sid << " Sending message: " << msg << std::endl;
    clients.at(id)->send_to(msg);
  }

private:
  // persistent state
  int sid;
  std::unique_ptr<TcpServer> server;
  std::unordered_map<int, std::unique_ptr<TcpClient>> clients;
  uint64_t current_term;
  uint64_t voted_for;
  std::vector<std::string> log;

  // volatile state
  uint64_t commit_index;
  uint64_t last_applied;
};