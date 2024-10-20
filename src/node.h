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

struct SelfElection {
  int num_responded;
  int num_votes;
};

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
      LOG_LEVEL("Unknown message type", LogLevel::ERROR);
      break;
    }
  }

  void main() {
    while (true) {
      if (state == ServerState::FOLLOWER) {
        consider_election();
      } else if (state == ServerState::CANDIDATE) {
        if (last_election + election_interval < time_ns()) {
          // timeout
          LOG("Election timeout");
          state = ServerState::FOLLOWER;
          last_election = time_ns();
          self_election.reset();
          voted_for.reset();
          current_term++;
          election_interval = get_election_interval();
        }
      } else if (state == ServerState::LEADER) {
        // send append entries
        consider_election();
        send_append_entries();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  uint32_t get_id() { return sid; }

private:
  void consider_election() {
    if (last_election + election_interval < time_ns()) {
      LOG("Starting election");
      current_term++;
      state = ServerState::CANDIDATE;
      self_election = SelfElection{1, 1};
      voted_for = sid;
      send_request_vote();
      last_election = time_ns();
    }
  }

  void send_message(const std::string &msg, MessageType type) {
    for (const auto &[id, client] : clients) {
      client->send_to(sid, msg, type);
    }
  }

  void send_message(const std::string &msg, MessageType type, uint32_t id) {
    clients.at(id)->send_to(sid, msg, type);
  }

  void send_request_vote() {
    RequestVote rv{current_term, sid, 0, 0};
    auto msg_str = build_message(rv, sid);
    send_message(msg_str, MessageType::REQUEST_VOTE);
  };

  void send_append_entries() {
    AppendEntries ae{current_term, sid, 0, 0, 0, {}};
    auto msg_str = build_message(ae, sid);
    send_message(msg_str, MessageType::APPEND_ENTRIES);
  };

  void on_request_vote(Message m) {
    RequestVote rv = parse_message_contents<RequestVote>(m.msg);
    LOG("Received request vote from " + std::to_string(rv.candidate_id) +
        " for term " + std::to_string(rv.term));
    RequestVoteResponse rvr{current_term, true};
    if (rv.term < current_term ||
        (voted_for.has_value() && voted_for != rv.candidate_id)) {
      LOG("Rejecting vote");
      rvr.vote_granted = false;
    } else {
      LOG("Granting vote to " + std::to_string(rv.candidate_id));
      voted_for = rv.candidate_id;
      current_term = rv.term;
      state = ServerState::FOLLOWER;
    }
    auto msg_str = build_message(rvr, sid);
    send_message(msg_str, MessageType::REQUEST_VOTE_RESPONSE, m.id);
  }

  void on_request_vote_response(Message m) {
    RequestVoteResponse rvr =
        parse_message_contents<RequestVoteResponse>(m.msg);
    LOG("Received request vote response from " + std::to_string(m.id) +
        " for term " + std::to_string(rvr.term) + " with vote " +
        std::to_string(rvr.vote_granted));
    if (rvr.vote_granted) {
      if (!self_election.has_value()) {
        LOG_LEVEL("Received vote without starting election", LogLevel::ERROR);
        exit(EXIT_FAILURE);
      }
      self_election->num_votes++;
      auto num_nodes = clients.size() + 1;
      if (self_election->num_votes > num_nodes / 2) {
        state = ServerState::LEADER;
        LOG("Becoming leader");
      } else if (self_election->num_responded == num_nodes) {
        LOG("Not enough votes");
        self_election.reset();
        state = ServerState::FOLLOWER;
        voted_for.reset();
      }
    }
  };

  void on_append_entries(Message m) {
    LOG("Received append entries");
    AppendEntries ae = parse_message_contents<AppendEntries>(m.msg);
    switch (state) {
    case ServerState::LEADER:
      LOG_LEVEL("Leader received append entries", LogLevel::ERROR);
      exit(EXIT_FAILURE);
    case ServerState::CANDIDATE: {
      AppendEntriesResponse aer;
      if (ae.term >= current_term) {
        current_term = ae.term;
        state = ServerState::FOLLOWER;
        last_election = time_ns();
        append_entries(ae);
        aer = {current_term, true};
      } else {
        LOG("Rejecting append entries");
        aer = {current_term, false};
      }
      auto msg_str = build_message(aer, sid);
      send_message(msg_str, MessageType::APPEND_ENTRIES_RESPONSE, m.id);
      break;
    }
    case ServerState::FOLLOWER: {
      if (ae.term >= current_term) {
        current_term = ae.term;
        last_election = time_ns();
      }
      AppendEntriesResponse aer{current_term, true};
      auto msg_str = build_message(aer, sid);
      send_message(msg_str, MessageType::APPEND_ENTRIES_RESPONSE, m.id);
      append_entries(ae);
    }
    };
  }

  void append_entries(const AppendEntries &ae) {
    LOG("Appending entries");
    log.insert(log.end(), ae.entries.begin(), ae.entries.end());
  }
  // persistent state
  uint32_t sid;
  std::unique_ptr<TcpServer> server;
  std::unordered_map<uint32_t, std::unique_ptr<TcpClient>> clients;
  uint64_t current_term = 0;
  std::optional<uint32_t> voted_for;
  std::vector<std::string> log;
  std::optional<SelfElection> self_election;
  ServerState state;
  int64_t election_interval;
  int64_t last_election;

  // volatile state
  uint64_t commit_index;
  uint64_t last_applied;
};