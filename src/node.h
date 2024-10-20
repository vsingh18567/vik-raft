#pragma once
#include "logging.h"
#include "tcp/connection_store.h"
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
  return (500 + (rand() % 500)) * 1'000'000;
}

inline int64_t time_ns() {
  return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

class ServerNode : public TcpListener {
public:
  ServerNode(const std::string &config_file, uint32_t id)
      : sid(id), state(ServerState::FOLLOWER),

        election_interval(get_election_interval()), last_election(time_ns()) {

    auto connections = build_connections(config_file, sid);
    server = std::make_unique<TcpServer>(connections.server_port);
    server->add_listener(this);
    std::thread(TcpServer::open, std::ref(*server)).detach();
    for (const auto &[id, address_port] : connections.client_addresses) {
      clients.insert({id, std::make_unique<TcpClient>(address_port.first,
                                                      address_port.second)});
    }
    LOG(std::to_string(clients.size()));
    for (const auto &[id, client] : clients) {
      LOG("Sending to " + std::to_string(id));
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  void on_message(Message m) override {
    switch (m.type) {
    case MessageType::APPEND_ENTRIES:
      on_append_entries(m);
      break;
    case MessageType::REQUEST_VOTE:
      on_request_vote(m);
      break;
    case MessageType::REQUEST_VOTE_RESPONSE:
      on_request_vote_response(m);
      break;
    default:
      LOG("Unknown message type", LogLevel::ERROR);
      break;
    }
  }

  void main() {
    while (true) {
      if (state == ServerState::FOLLOWER) {
        if (last_election + election_interval < time_ns()) {
          LOG("Starting election");
          current_term++;
          state = ServerState::CANDIDATE;
          num_votes = 1;
          voted_for = sid;
          send_request_vote();
        }
      } else if (state == ServerState::CANDIDATE) {
        // send request vote
      } else if (state == ServerState::LEADER) {
        // send append entries
      }
    }
  }

private:
  void send_message(const std::string &msg, MessageType type) {
    for (const auto &[id, client] : clients) {
      client->send_to(sid, msg, type);
    }
  }

  void send_request_vote() {
    LOG("Sending request vote");
    RequestVote rv{current_term, sid, 0, 0};
    auto msg_str = build_message(rv, sid);
    num_votes = 1;
    send_message(msg_str, MessageType::REQUEST_VOTE);
  };

  void send_append_entries() { LOG("Sending append entries"); };

  void on_request_vote(Message m) {
    RequestVote rv = parse_message_contents<RequestVote>(m.msg);
    LOG("Received request vote from " + std::to_string(rv.candidate_id) +
        " for term " + std::to_string(rv.term));
    if (rv.term < current_term ||
        (voted_for.has_value() && voted_for != rv.candidate_id)) {
      LOG("Rejecting vote");
      RequestVoteResponse rvr{current_term, false};
      auto msg_str = build_message(rvr, sid);
      clients.at(rv.candidate_id)
          ->send_to(sid, msg_str, MessageType::REQUEST_VOTE_RESPONSE);
    } else {
      LOG("Granting vote to " + std::to_string(rv.candidate_id));
      voted_for = rv.candidate_id;
      RequestVoteResponse rvr{current_term, true};
      auto msg_str = build_message(rvr, sid);
      clients.at(m.id)->send_to(sid, msg_str,
                                MessageType::REQUEST_VOTE_RESPONSE);
    }
  };

  void on_request_vote_response(Message m) {
    RequestVoteResponse rvr =
        parse_message_contents<RequestVoteResponse>(m.msg);
    LOG("Received request vote response from " + std::to_string(m.id) +
        " for term " + std::to_string(rvr.term) + " with vote " +
        std::to_string(rvr.vote_granted));
    if (rvr.vote_granted) {
      num_votes++;
      if (num_votes > clients.size() / 2) {
        state = ServerState::LEADER;
        LOG("Becoming leader");
      }
    }
  };

  void on_append_entries(Message m) { LOG("Received append entries"); };

  // persistent state
  uint32_t sid;
  std::unique_ptr<TcpServer> server;
  std::unordered_map<uint32_t, std::unique_ptr<TcpClient>> clients;
  uint64_t current_term = 0;
  std::optional<uint32_t> voted_for;
  std::vector<std::string> log;
  uint32_t num_votes;

  ServerState state;
  int64_t election_interval;
  int64_t last_election;

  // volatile state
  uint64_t commit_index;
  uint64_t last_applied;
};