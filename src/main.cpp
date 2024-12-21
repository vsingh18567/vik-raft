#include "absl/log/log.h"
#include "messages.pb.h"
#include "networking/network_manager.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  // Initialize the protobuf library
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  vikraft::NetworkManager network_manager("config.txt", std::stoi(argv[1]));
  while (true) {
  }
  return 0;
}
