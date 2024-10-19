#pragma once
#include "logging.h"
#include "tcp/messages.h"
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

int64_t get_election_interval() {
  // [150, 300] ms
  return (150 + (rand() % 150)) * 1'000'000;
}

inline int64_t time_ns() {
  return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

class ServerNode : public TcpListener {
public:
  ServerNode(const std::string &config_file, int id)
      : sid(id), state(ServerState::FOLLOWER),
        election_interval(get_election_interval()), last_election(time_ns()) {
    // FOR SOME REASON, REFACTORING THIS BREAKS THE CONNNECTIONS
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

  void on_message(Message m) override {
    switch (m.type) {
    case MessageType::APPEND_ENTRIES:
      on_append_entries();
      break;
    case MessageType::REQUEST_VOTE:
      on_request_vote();
      break;
    }
  }

  void main() {
    while (true) {
      if (state == ServerState::FOLLOWER) {
        if (last_election + election_interval < time_ns()) {
          state = ServerState::CANDIDATE;
          send_request_vote();
        }
      } else if (state == ServerState::CANDIDATE) {
        // send request vote
      } else if (state == ServerState::LEADER) {
        // send append entries
      }
    }
  }
  void send_message(const std::string &msg, MessageType type) {
    for (const auto &[id, client] : clients) {
      LOG("Sending message to " + std::to_string(id));
      client->send_to(sid, msg, type);
    }
  }

private:
  void send_request_vote() {
    LOG("Sending request vote");
    send_message("vote", MessageType::REQUEST_VOTE);
  };

  void send_append_entries() { LOG("Sending append entries"); };

  void on_request_vote() { LOG("Received request vote"); };

  void on_append_entries() { LOG("Received append entries"); };

  // persistent state
  uint32_t sid;
  std::unique_ptr<TcpServer> server;
  std::unordered_map<int, std::unique_ptr<TcpClient>> clients;
  uint64_t current_term;
  uint64_t voted_for;
  std::vector<std::string> log;

  ServerState state;
  int64_t election_interval;
  int64_t last_election;

  // volatile state
  uint64_t commit_index;
  uint64_t last_applied;
};