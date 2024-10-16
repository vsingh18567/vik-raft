#include <string>
#include <vector>

struct AppendEntries {
  uint64_t term;
  uint64_t leader_id;
  uint64_t prev_log_index;
  uint64_t prev_log_term;
  std::vector<std::string> entries;
  uint64_t leader_commit;
};

struct AppendEntriesResponse {
  uint64_t term;
  bool success;
};

struct RequestVote {
  uint64_t term;
  uint64_t candidate_id;
  uint64_t last_log_index;
  uint64_t last_log_term;
};

struct RequestVoteResponse {
  uint64_t term;
  bool vote_granted;
};