#pragma once

#include "config.h"
#include "node_id.h"
#include <mutex>
#include <unordered_set>

namespace vikraft {
class ClusterMembers {
public:
  void add_node(NodeId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    connected_nodes_.insert(id);
  }

  void remove_node(NodeId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    connected_nodes_.erase(id);
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_nodes_.size();
  }

  bool is_connected(NodeId id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connected_nodes_.count(id) > 0;
  }

private:
  std::unordered_set<NodeId> connected_nodes_;
  mutable std::mutex mutex_;
};
} // namespace vikraft