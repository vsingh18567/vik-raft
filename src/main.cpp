#include "absl/log/log.h"
#include "node.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  // Initialize the protobuf library
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  vikraft::Config config("config.txt");
  vikraft::Node node(std::stoi(argv[1]), config, "log.txt", "state.txt");
  node.start();
  while (true) {
  }
  return 0;
}
