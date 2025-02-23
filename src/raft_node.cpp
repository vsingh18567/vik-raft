#include "raft_node.h"

RaftNode::RaftNode(uint64_t id, const std::vector<std::string> &peer_addresses)
    : id_(id), network_(id) {
  network_.RegisterServer(this, peer_addresses[id]);
  network_.ConnectToPeers(peer_addresses);
}

grpc::Status RaftNode::RequestVote(grpc::ServerContext *context,
                                   const vikraft::RequestVoteRequest *request,
                                   vikraft::RequestVoteResponse *response) {
  response->set_vote_granted(false);
  response->set_term(current_term_);

  if (request->term() < current_term_) {
    return grpc::Status::OK;
  }

  if (request->term() > current_term_) {
    current_term_ = request->term();
    state_ = State::FOLLOWER;
    voted_for_ = -1;
  }

  uint64_t our_last_log_index = log_.empty() ? 0 : log_.size() - 1;
  uint64_t our_last_log_term = log_.empty() ? 0 : log_.back().term();

  // Only grant vote if we haven't voted yet in this term
  // and the candidate's log is at least as up to date as ours
  if ((voted_for_ == -1 || voted_for_ == request->candidate_id()) &&
      request->last_log_term() >= our_last_log_term &&
      request->last_log_index() >= our_last_log_index) {

    response->set_vote_granted(true);
    voted_for_ = request->candidate_id();

    election_timer_.Reset();
  }

  return grpc::Status::OK;
}

grpc::Status
RaftNode::AppendEntries(grpc::ServerContext *context,
                        const vikraft::AppendEntriesRequest *request,
                        vikraft::AppendEntriesResponse *response) {
  response->set_success(false);
  response->set_term(current_term_);

  if (request->term() < current_term_) {
    return grpc::Status::OK;
  }

  if (request->term() >= current_term_) {
    if (request->term() > current_term_) {
      current_term_ = request->term();
      voted_for_ = -1;
    }

    if (state_ != State::FOLLOWER) {
      state_ = State::FOLLOWER;
    }

    election_timer_.Reset();

    response->set_success(true);
  }

  return grpc::Status::OK;
}

void RaftNode::Start() {
  running_ = true;
  main_thread_ = std::thread(&RaftNode::Run, this);
}

void RaftNode::Stop() {
  running_ = false;
  if (main_thread_.joinable()) {
    main_thread_.join();
  }
}

void RaftNode::Run() {
  LOG_INFO("Raft node " + std::to_string(id_) + " started");
  while (running_) {
    switch (state_) {
    case State::FOLLOWER:
      RunFollower();
      break;
    case State::CANDIDATE:
      RunCandidate();
      break;
    case State::LEADER:
      RunLeader();
      break;
    }
  }
}

RaftNode::~RaftNode() {
  if (main_thread_.joinable()) {
    main_thread_.join();
  }
}

void RaftNode::RunFollower() {
  LOG_INFO("Raft node " + std::to_string(id_) + " is a follower");
  election_timer_.Reset();
  while (running_ && state_ == State::FOLLOWER) {
    if (election_timer_.HasExpired()) {
      state_ = State::CANDIDATE;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void RaftNode::RunCandidate() {
  LOG_INFO("Raft node " + std::to_string(id_) + " is a candidate");
  current_term_++;
  voted_for_ = id_;
  uint64_t votes = 1;

  election_timer_.Reset();

  vikraft::RequestVoteRequest request;
  request.set_term(current_term_);
  request.set_candidate_id(id_);
  request.set_last_log_index(log_.empty() ? 0 : log_.size() - 1);
  request.set_last_log_term(log_.empty() ? 0 : log_.back().term());

  for (uint64_t peer = 0; peer < network_.GetPeerCount() + 1; peer++) {
    if (peer == id_)
      continue;

    auto response = network_.SendRequestVote(peer, request);

    if (response.vote_granted()) {
      votes++;
    } else if (response.term() > current_term_) {
      current_term_ = response.term();
      state_ = State::FOLLOWER;
      return;
    }

    if (votes > (network_.GetPeerCount() + 1) / 2) {
      state_ = State::LEADER;

      next_index_.resize(network_.GetPeerCount() + 1, log_.size());
      match_index_.resize(network_.GetPeerCount() + 1, 0);
      return;
    }
  }

  // If election timeout elapses, start new election
  while (running_ && state_ == State::CANDIDATE) {
    if (election_timer_.HasExpired()) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void RaftNode::RunLeader() {
  LOG_INFO("Raft node " + std::to_string(id_) + " is a leader");
  while (running_ && state_ == State::LEADER) {
    vikraft::AppendEntriesRequest request;
    request.set_term(current_term_);
    request.set_leader_id(id_);

    for (uint64_t peer = 0; peer < network_.GetPeerCount() + 1; peer++) {
      if (peer == id_)
        continue;

      auto response = network_.SendAppendEntries(peer, request);

      if (response.term() > current_term_) {
        current_term_ = response.term();
        state_ = State::FOLLOWER;
        return;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}
