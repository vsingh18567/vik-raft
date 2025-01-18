#include "node.h"
#include <csignal>
#include <iostream>
#include <string>
int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  vikraft::Config config("config.txt");
  vikraft::Node node(std::stoi(argv[1]), config, "log.txt", "state.txt");
  node.start();
  while (true) {
  }
  return 0;
}
