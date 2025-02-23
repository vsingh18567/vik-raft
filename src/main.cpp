#include "raft_node.h"

int main(int argc, char **argv) {
  std::vector<std::string> addresses = {"localhost:50051", "localhost:50052",
                                        "localhost:50053"};
  RaftNode node(atoi(argv[1]), addresses);
  node.Start();
  while (true) {
  }
}
