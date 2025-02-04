#pragma once
#include "cluster_members.h"
#include "election_timer.h"
#include "messages.pb.h"
#include "node_id.h"
#include <chrono>
#include <optional>

namespace vikraft {
enum class NodeState { FOLLOWER, CANDIDATE, LEADER };

class StateManager {
public:
  StateManager(NodeId self_id, ElectionTimer &timer, int cluster_size,
               ClusterMembers &cluster_members)
      : self_id(self_id), current_term(0), voted_for(-1), commit_index(-1),
        last_applied(-1), state(NodeState::FOLLOWER), votes_received(0),
        timer_(timer), cluster_members_(cluster_members),
        known_leader(std::nullopt) {
    timer_.reset();
    next_index.resize(cluster_size - 1, 0);
    match_index.resize(cluster_size - 1, 0);
  }

  ~StateManager() = default;

  RequestVoteResponse on_request_vote(const RequestVote &args) {
    RequestVoteResponse response;
    response.set_term(current_term);
    response.set_vote_granted(false);

    LOG(INFO) << "Received RequestVote from " << args.candidate_id()
              << " for term " << args.term() << " (our term: " << current_term
              << ", our state: " << static_cast<int>(state) << ")";

    if (args.term() < current_term) {
      LOG(INFO) << "Rejecting vote: candidate term is lower";
      return response;
    }

    if (args.term() > current_term) {
      LOG(INFO) << "Becoming follower due to higher term in RequestVote";
      become_follower(args.term(), -1);
    }

    if ((voted_for == -1 || voted_for == args.candidate_id()) &&
        is_log_up_to_date(args.last_log_index(), args.last_log_term())) {
      voted_for = args.candidate_id();
      response.set_term(current_term);
      response.set_vote_granted(true);
      timer_.reset();
      LOG(INFO) << "Granted vote to " << args.candidate_id();
    } else {
      LOG(INFO) << "Rejected vote: already voted for " << voted_for
                << " or log not up to date";
    }

    return response;
  }
  AppendEntriesResponse on_append_entries(const AppendEntries &args) {
    AppendEntriesResponse response;
    response.set_term(current_term);
    response.set_success(false);

    if (args.term() < current_term) {
      LOG(INFO) << "Rejecting append entries: candidate term is lower";
      return response;
    }

    if (args.term() > current_term) {
      become_follower(args.term(), args.leader_id());
    }

    timer_.reset();
    if (args.prev_log_index() >= 0) {
      if (args.prev_log_index() >= log.size() ||
          log[args.prev_log_index()].term() != args.prev_log_term()) {
        LOG(INFO) << "Rejecting append entries: prev log index "
                  << args.prev_log_index() << " is out of bounds or term "
                  << args.prev_log_term() << " does not match";
        return response;
      }
    }

    size_t new_entry_index = args.prev_log_index() + 1;
    for (size_t i = 0; i < args.entries().size(); i++) {
      LOG(INFO) << "Appending entry: " << args.entries(i).command();
      if (new_entry_index < log.size()) {
        if (log[new_entry_index].term() != args.entries(i).term()) {
          log.resize(new_entry_index);
          log.push_back(args.entries(i));
        }
      } else {
        log.push_back(args.entries(i));
      }
      new_entry_index++;
    }

    if (args.leader_commit() > commit_index) {
      commit_index =
          std::min(args.leader_commit(), static_cast<int>(log.size() - 1));
    }

    response.set_success(true);
    return response;
  }

  void start_election() {
    LOG(INFO) << "Starting election";
    current_term++;
    state = NodeState::CANDIDATE;
    voted_for = self_id;
    votes_received = 1;
    timer_.reset();
  }

  void process_request_vote_response(const NodeId &from,
                                     const RequestVoteResponse &response) {
    if (state != NodeState::CANDIDATE || response.term() < current_term) {
      LOG(INFO) << "Ignoring request vote response from " << from
                << " with term " << response.term()
                << " because we are not a candidate or our term is higher";
      return;
    }

    if (response.term() > current_term) {
      LOG(INFO) << "Becoming follower from " << from << " with term "
                << response.term();
      become_follower(response.term(), -1);
      return;
    }

    if (response.vote_granted()) {
      votes_received++;
      LOG(INFO) << "Received vote from " << from << " with term "
                << response.term();
      if (votes_received > (cluster_members_.size() + 1) / 2) {
        LOG(INFO) << "Received majority votes: " << votes_received << " out of "
                  << cluster_members_.size() << " needed";
        become_leader();
      }
    }
  }

  ClientCommandResponse on_client_command(const ClientCommand &args) {
    ClientCommandResponse response;
    response.set_is_leader(is_leader());
    response.set_leader_id(known_leader.value_or(-1));
    response.set_success(append_entry(args.command()));
    return response;
  }

  std::vector<LogEntry> get_logs_to_send() const {
    std::vector<LogEntry> logs;
    for (int i = commit_index + 1; i < static_cast<int>(log.size()); i++) {
      logs.push_back(log[i]);
    }
    return logs;
  }

  bool append_entry(const std::string &command) {
    if (state != NodeState::LEADER) {
      return false;
    }

    LOG(INFO) << "Appending entry: " << command;

    LogEntry entry;
    entry.set_id(self_id);
    entry.set_term(current_term);
    entry.set_command(command);

    log.push_back(entry);
    return true;
  }

  void process_append_entries_response(const NodeId &from,
                                       const AppendEntriesResponse &response) {
    if (state != NodeState::LEADER) {
      return;
    }

    if (response.term() > current_term) {
      become_follower(response.term(), -1);
      return;
    }

    size_t node_idx = static_cast<size_t>(from);
    if (response.success()) {
      match_index[node_idx] = next_index[node_idx] - 1;
      next_index[node_idx]++;
      commit_entries();
    } else {
      next_index[node_idx] = std::max(0, next_index[node_idx] - 1);
    }
  }

  std::optional<LogEntry> get_log_entry(int index) const {
    if (index < 0 || index >= static_cast<int>(log.size())) {
      return std::nullopt;
    }
    return log[index];
  }

  bool is_leader() const { return state == NodeState::LEADER; }
  std::optional<NodeId> get_known_leader() const { return known_leader; }
  int get_current_term() const { return current_term; }
  NodeState get_state() const { return state; }
  int get_last_log_index() const { return static_cast<int>(log.size()) - 1; }

  int get_last_log_term() const { return log.empty() ? 0 : log.back().term(); }
  int get_commit_index() const { return commit_index; }

  int get_log_term(int index) const {
    if (index < 0 || index >= static_cast<int>(log.size())) {
      return 0;
    }
    return log[index].term();
  }

private:
  void become_follower(int new_term, NodeId new_leader) {
    LOG(INFO) << "Becoming follower";
    state = NodeState::FOLLOWER;
    current_term = new_term;
    voted_for = -1;
    timer_.reset();
    known_leader =
        new_leader == -1 ? std::nullopt : std::optional<NodeId>(new_leader);
  }

  void become_leader() {
    LOG(INFO) << "Becoming leader";
    if (state != NodeState::CANDIDATE) {
      return;
    }

    state = NodeState::LEADER;
    next_index.resize(next_index.size(), log.size());
    match_index.resize(match_index.size(), 0);
    known_leader = self_id;
    timer_.reset();
  }

  void update_term(int new_term) {
    if (new_term > current_term) {
      current_term = new_term;
      voted_for = -1;
      state = NodeState::FOLLOWER;
    }
  }

  bool is_log_up_to_date(int last_log_index, int last_log_term) const {
    int my_last_term = get_last_log_term();
    int my_last_index = get_last_log_index();

    return (last_log_term > my_last_term) ||
           (last_log_term == my_last_term && last_log_index >= my_last_index);
  }

  void commit_entries() {
    for (int n = commit_index + 1; n < static_cast<int>(log.size()); n++) {
      int replicas = 1; // Count self
      for (int match : match_index) {
        if (match >= n) {
          replicas++;
        }
      }

      if (replicas > (static_cast<int>(match_index.size()) + 1) / 2) {
        if (log[n].term() == current_term) {
          commit_index = n;
        }
      }
    }
  }

  const NodeId self_id;
  int current_term;
  NodeId voted_for;
  std::optional<NodeId> known_leader;
  std::vector<LogEntry> log;

  int commit_index;
  int last_applied;

  std::vector<int> next_index;
  std::vector<int> match_index;

  NodeState state;

  int votes_received;

  ElectionTimer &timer_;
  ClusterMembers &cluster_members_;
};

} // namespace vikraft