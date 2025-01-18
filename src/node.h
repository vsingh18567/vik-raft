#include "networking/network_manager.h"
#include "state_manager.h"

namespace vikraft {
class Node : public MessageHandler {
public:
  Node(const NodeId &id, Config config, const std::string &log_file_path,
       const std::string &state_file_path)
      : id(id), log_file_path(log_file_path), state_file_path(state_file_path),
        election_timer(), state_manager(id, election_timer,
                                        config.cluster_size(), cluster_members),
        network_manager(config, id, this, cluster_members) {}

  ~Node() = default;

  void start() {
    election_timer.reset();
    timeout_checker_thread_ = std::thread(&Node::timeout_checker, this);
  }

  void handle_message(const Message &message, NodeId from) {
    switch (message.type()) {
    case MessageType::REQUEST_VOTE: {
      RequestVote request;
      request.ParseFromString(message.data());
      RequestVoteResponse response = state_manager.on_request_vote(request);
      send_message(response, MessageType::REQUEST_VOTE_RESPONSE,
                   message.from());
      break;
    }
    case MessageType::REQUEST_VOTE_RESPONSE: {
      RequestVoteResponse response;
      response.ParseFromString(message.data());
      state_manager.process_request_vote_response(message.from(), response);
      break;
    }
    case MessageType::APPEND_ENTRIES: {
      AppendEntries request;
      request.ParseFromString(message.data());
      AppendEntriesResponse response = state_manager.on_append_entries(request);
      send_message(response, MessageType::APPEND_ENTRIES_RESPONSE,
                   message.from());
      break;
    }
    case MessageType::APPEND_ENTRIES_RESPONSE: {
      AppendEntriesResponse response;
      response.ParseFromString(message.data());
      state_manager.process_append_entries_response(message.from(), response);
      break;
    }
    default:
      LOG(ERROR) << "Unknown message type: " << message.type();
      exit(1);
    }
  }

  void send_heartbeat() {
    if (!state_manager.is_leader()) {
      return;
    }

    AppendEntries request;
    request.set_term(state_manager.get_current_term());
    request.set_leader_id(id);
    request.set_prev_log_index(state_manager.get_last_log_index());
    request.set_prev_log_term(state_manager.get_last_log_term());
    request.set_leader_commit(state_manager.get_commit_index());

    send_message(request, MessageType::APPEND_ENTRIES);
  }

private:
  void start_election() {
    state_manager.start_election();
    RequestVote request;
    request.set_term(state_manager.get_current_term());
    request.set_candidate_id(id);
    request.set_last_log_index(state_manager.get_last_log_index());
    request.set_last_log_term(state_manager.get_last_log_term());
    send_message(request, MessageType::REQUEST_VOTE);
  }

  template <typename T>
  Message build_message(const T &message, MessageType type) {
    Message message_to_send;
    message_to_send.set_type(type);
    message_to_send.set_from(id);
    message_to_send.set_data(message.SerializeAsString());
    return message_to_send;
  }
  template <typename T> void send_message(const T &message, MessageType type) {
    Message message_to_send = build_message(message, type);
    network_manager.send_message(message_to_send.SerializeAsString());
  }

  template <typename T>
  void send_message(const T &message, MessageType type, int to) {
    Message message_to_send = build_message(message, type);
    network_manager.send_message(message_to_send.SerializeAsString(), to);
  }

  void timeout_checker() {
    while (!shutdown_) {
      if (state_manager.is_leader()) {
        send_heartbeat();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      } else if (election_timer.has_elapsed()) {
        start_election();

        if (!state_manager.is_leader()) {
          election_timer.reset();
        }
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  }
  NodeId id;
  ElectionTimer election_timer;
  ClusterMembers cluster_members;

  StateManager state_manager;
  NetworkManager network_manager;
  const std::string log_file_path;
  const std::string state_file_path;
  std::atomic<bool> shutdown_{false};
  std::thread timeout_checker_thread_;
};
} // namespace vikraft
