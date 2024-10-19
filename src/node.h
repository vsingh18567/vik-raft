#pragma once

#include "config_parser.h"

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
  ServerNode(const std::string &config_file, uint32_t id)
      : sid(id), state(ServerState::FOLLOWER),
        election_interval(get_election_interval()), last_election(0) {

    ConfigParser parser(config_file, id);
    server = parser.get_server();
    clients = parser.get_clients();
    server->add_listener(this);
    std::thread(TcpServer::open, std::ref(*server)).detach();
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
  void send_message(const std::string &msg) {
    for (const auto &[id, client] : clients) {
      std::cout << "id: " << sid << " Sending message: " << msg << std::endl;
      client->send_to(sid, msg);
    }
  }

private:
  void send_request_vote() {

  };

  void send_append_entries() {

  };

  void on_request_vote() {

  };

  void on_append_entries() {

  };

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