#include "node.h"
#include <thread>

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "Usage: ./main <config_file> <id>" << std::endl;
    return 1;
  }
  uint32_t id = std::stoi(argv[2]);
  ServerNode node(argv[1], id);
  set_log_file(std::to_string(id));
  node.main();
  // for (int i = 0; i < 20; i++) {
  //   if (id == 0) {
  //     node.send_message("hello");
  //   } else {
  //     node.send_message("world");
  //   }
  //   std::this_thread::sleep_for(std::chrono::seconds(1));
  // }
}