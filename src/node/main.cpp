#include "node.h"
#include <csignal>
#include <iostream>
#include <string>
int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  vikraft::Config config(argv[1]);
  vikraft::Node node(std::stoi(argv[2]), config, "log.txt", "state.txt");
  node.start();
  while (true) {
  }
  return 0;
}
