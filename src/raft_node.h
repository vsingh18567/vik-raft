#pragma once

#include "proto/service.grpc.pb.h"
#include "raft_network.h"
#include "timer.h"
#include "util.h"
#include <atomic>
#include <grpcpp/grpcpp.h>
#include <thread>

class RaftNode : public vikraft::RaftService::Service {
public:
  enum class State {
    FOLLOWER,
    CANDIDATE,
    LEADER,
  };

  RaftNode(uint64_t id, const std::vector<std::string> &peer_addresses);

  grpc::Status RequestVote(grpc::ServerContext *context,
                           const vikraft::RequestVoteRequest *request,
                           vikraft::RequestVoteResponse *response) override;

  grpc::Status AppendEntries(grpc::ServerContext *context,
                             const vikraft::AppendEntriesRequest *request,
                             vikraft::AppendEntriesResponse *response) override;
  void Start();

  void Stop();

  ~RaftNode();

private:
  void Run();

  void RunFollower();

  void RunCandidate();

  void RunLeader();

private:
  uint64_t id_;
  State state_ = State::FOLLOWER;
  RaftNetwork network_;
  std::thread main_thread_;
  std::atomic<bool> running_ = false;
  ElectionTimer election_timer_;

  // persistent state
  uint64_t current_term_ = 0;
  int64_t voted_for_ = -1;
  std::vector<vikraft::LogEntry> log_;

  // volatile state
  uint64_t commit_index_ = 0;
  uint64_t last_applied_ = 0;

  // leader_state
  std::vector<uint64_t> next_index_;
  std::vector<uint64_t> match_index_;
};
