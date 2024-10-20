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
  uint64_t leader_commit;
  std::vector<std::string> entries;

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

template <> AppendEntries parse_message_contents(const std::string &msg) {
  AppendEntries ae;
  ae.term = *reinterpret_cast<const uint64_t *>(msg.data());
  ae.leader_id = *reinterpret_cast<const uint32_t *>(msg.data() + 8);
  ae.prev_log_index = *reinterpret_cast<const uint64_t *>(msg.data() + 12);
  ae.prev_log_term = *reinterpret_cast<const uint64_t *>(msg.data() + 20);
  ae.leader_commit = *reinterpret_cast<const uint64_t *>(msg.data() + 28);
  uint32_t entries_size = *reinterpret_cast<const uint32_t *>(msg.data() + 36);
  uint32_t offset = 40;
  for (uint32_t i = 0; i < entries_size; i++) {
    uint32_t entry_size =
        *reinterpret_cast<const uint32_t *>(msg.data() + offset);
    std::string entry(msg.data() + offset + 4, entry_size);
    ae.entries.push_back(entry);
    offset += entry_size + 4;
  }
  return ae;
}

template <typename T> std::string build_message(const T &t, uint32_t id) {
  std::string t_str(reinterpret_cast<const char *>(&t), sizeof(T));
  return t_str;
}

template <> std::string build_message(const AppendEntries &ae, uint32_t id) {
  std::string msg;
  msg.append(reinterpret_cast<const char *>(&ae.term), sizeof(uint64_t));
  msg.append(reinterpret_cast<const char *>(&ae.leader_id), sizeof(uint32_t));
  msg.append(reinterpret_cast<const char *>(&ae.prev_log_index),
             sizeof(uint64_t));
  msg.append(reinterpret_cast<const char *>(&ae.prev_log_term),
             sizeof(uint64_t));
  msg.append(reinterpret_cast<const char *>(&ae.leader_commit),
             sizeof(uint64_t));
  uint32_t entries_size = ae.entries.size();
  msg.append(reinterpret_cast<const char *>(&entries_size), sizeof(uint32_t));
  for (const auto &entry : ae.entries) {
    uint32_t entry_size = entry.size();
    msg.append(reinterpret_cast<const char *>(&entry_size), sizeof(uint32_t));
    msg.append(entry);
  }
  return build_message(msg, id, AppendEntries::get_type());
}