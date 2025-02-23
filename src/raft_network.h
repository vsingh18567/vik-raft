#pragma once

#include "proto/service.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <unordered_map>

class RaftNetwork {
public:
  RaftNetwork(uint64_t self_id);

  void RegisterServer(vikraft::RaftService::Service *service,
                      const std::string &address);

  void ConnectToPeers(const std::vector<std::string> &peer_addresses);

  // Send RequestVote RPC to a specific node
  vikraft::RequestVoteResponse
  SendRequestVote(uint64_t target_id,
                  const vikraft::RequestVoteRequest &request);

  // Send AppendEntries RPC to a specific node
  vikraft::AppendEntriesResponse
  SendAppendEntries(uint64_t target_id,
                    const vikraft::AppendEntriesRequest &request);

  // Get the number of peers (excluding self)
  size_t GetPeerCount() const;

private:
  struct Peer {
    std::unique_ptr<vikraft::RaftService::Stub> stub;
    std::string address;
  };

  // Map of peer ID to their connection info
  std::unordered_map<uint64_t, Peer> peers_;
  uint64_t self_id_;
  std::unique_ptr<grpc::Server> server_;

  // Helper to create a new gRPC channel and stub
  std::unique_ptr<vikraft::RaftService::Stub>
  CreateStub(const std::string &address);
};