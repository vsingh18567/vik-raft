#include "raft_network.h"
#include "util.h"
#include <grpcpp/create_channel.h>
#include <thread>
RaftNetwork::RaftNetwork(uint64_t self_id) : self_id_(self_id) {}

void RaftNetwork::RegisterServer(vikraft::RaftService::Service *service,
                                 const std::string &address) {
  grpc::ServerBuilder builder;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials());
  builder.RegisterService(service);
  server_ = builder.BuildAndStart();
}

void RaftNetwork::ConnectToPeers(
    const std::vector<std::string> &peer_addresses) {
  for (uint64_t i = 0; i < peer_addresses.size(); i++) {
    if (i == self_id_)
      continue;

    peers_[i] = {CreateStub(peer_addresses[i]), peer_addresses[i]};
    LOG_INFO("Added peer " + std::to_string(i) + " at " + peer_addresses[i]);
  }

  LOG_INFO("RaftNetwork initialized with " + std::to_string(peers_.size()) +
           " peers:");
  for (const auto &[id, peer] : peers_) {
    LOG_INFO("Peer " + std::to_string(id) + " -> " + peer.address);
  }

  for (const auto &[id, peer] : peers_) {
    bool connected = false;
    while (!connected) {
      vikraft::AppendEntriesRequest request;
      request.set_term(0);
      request.set_leader_id(self_id_);

      grpc::ClientContext context;
      context.set_deadline(std::chrono::system_clock::now() +
                           std::chrono::milliseconds(100));

      vikraft::AppendEntriesResponse response;
      grpc::Status status =
          peer.stub->AppendEntries(&context, request, &response);

      if (status.ok()) {
        LOG_INFO("Successfully connected to peer " + std::to_string(id));
        connected = true;
      } else {
        LOG_INFO("Waiting to connect to peer " + std::to_string(id) +
                 "... retrying");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }

  LOG_INFO("Successfully connected to all peers");
}
vikraft::RequestVoteResponse
RaftNetwork::SendRequestVote(uint64_t target_id,
                             const vikraft::RequestVoteRequest &request) {
  vikraft::RequestVoteResponse response;
  grpc::ClientContext context;

  context.set_deadline(std::chrono::system_clock::now() +
                       std::chrono::milliseconds(500)); // 500ms timeout

  auto it = peers_.find(target_id);
  if (it == peers_.end()) {
    LOG_ERROR("RequestVote RPC failed: peer " + std::to_string(target_id) +
              " not found");
    return response;
  }

  grpc::Status status =
      it->second.stub->RequestVote(&context, request, &response);
  if (!status.ok()) {
    LOG_ERROR("RequestVote RPC failed to " + it->second.address + ": " +
              status.error_message() +
              " (code=" + std::to_string(status.error_code()) + ")");
  }

  return response;
}

vikraft::AppendEntriesResponse
RaftNetwork::SendAppendEntries(uint64_t target_id,
                               const vikraft::AppendEntriesRequest &request) {
  vikraft::AppendEntriesResponse response;
  grpc::ClientContext context;

  context.set_deadline(std::chrono::system_clock::now() +
                       std::chrono::milliseconds(200)); // 200ms timeout

  auto it = peers_.find(target_id);
  if (it == peers_.end()) {
    LOG_ERROR("AppendEntries RPC failed: peer " + std::to_string(target_id) +
              " not found");
    return response;
  }

  grpc::Status status =
      it->second.stub->AppendEntries(&context, request, &response);
  if (!status.ok()) {
    LOG_ERROR("AppendEntries RPC failed to " + it->second.address + ": " +
              status.error_message() +
              " (code=" + std::to_string(status.error_code()) + ")");
  } else {
    LOG_INFO("AppendEntries to " + std::to_string(target_id) +
             " (term=" + std::to_string(request.term()) +
             "): success=" + std::to_string(response.success()) +
             " term=" + std::to_string(response.term()));
  }

  return response;
}

size_t RaftNetwork::GetPeerCount() const { return peers_.size(); }

std::unique_ptr<vikraft::RaftService::Stub>
RaftNetwork::CreateStub(const std::string &address) {
  auto channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  return vikraft::RaftService::NewStub(channel);
}