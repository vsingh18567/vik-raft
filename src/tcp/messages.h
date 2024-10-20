#pragma once
#include <iostream>
#include <string>
#include <vector>

enum class MessageType : uint32_t {
  APPEND_ENTRIES = 0,
  REQUEST_VOTE = 1,
  APPEND_ENTRIES_RESPONSE = 2,
  REQUEST_VOTE_RESPONSE = 3
};
struct AppendEntries {
  uint64_t term;
  uint32_t leader_id;
  uint64_t prev_log_index;
  uint64_t prev_log_term;
  std::vector<std::string> entries;
  uint64_t leader_commit;

  constexpr static MessageType get_type() {
    return MessageType::APPEND_ENTRIES;
  }
};

struct AppendEntriesResponse {
  uint64_t term;
  bool success;

  constexpr static MessageType get_type() {
    return MessageType::APPEND_ENTRIES_RESPONSE;
  }
};

struct RequestVote {
  uint64_t term;
  uint32_t candidate_id;
  uint64_t last_log_index;
  uint64_t last_log_term;

  constexpr static MessageType get_type() { return MessageType::REQUEST_VOTE; }
};

struct RequestVoteResponse {
  uint64_t term;
  bool vote_granted;

  constexpr static MessageType get_type() {
    return MessageType::REQUEST_VOTE_RESPONSE;
  }
};
struct Message {
  uint32_t size;
  uint32_t id;
  MessageType type;
  std::string msg;
} __attribute__((packed));

std::string build_message(const std::string &msg, uint32_t id,
                          MessageType type) {

  uint32_t size = sizeof(uint32_t) * 3 + msg.size();
  std::string size_str =
      std::string(reinterpret_cast<char *>(&size), sizeof(uint32_t));
  std::string id_str =
      std::string(reinterpret_cast<char *>(&id), sizeof(uint32_t));
  std::string type_str =
      std::string(reinterpret_cast<char *>(&type), sizeof(MessageType));
  std::string msg_str = size_str + id_str + type_str + msg;
  return msg_str;
}

Message parse_message(char *msg) {
  uint32_t size = *reinterpret_cast<uint32_t *>(msg);
  uint32_t id = *reinterpret_cast<uint32_t *>(msg + 4);
  MessageType type = *reinterpret_cast<MessageType *>(msg + 8);
  std::string full_string(msg, size);
  auto msg_str = full_string.substr(12);
  return {size, id, type, msg_str};
}

template <typename T> T parse_message_contents(const std::string &msg) {
  T t = *reinterpret_cast<const T *>(msg.data());
  return t;
}

template <typename T> std::string build_message(const T &t, uint32_t id) {
  std::string t_str(reinterpret_cast<const char *>(&t), sizeof(T));
  return t_str;
}