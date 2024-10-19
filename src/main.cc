#include "node.h"
#include <thread>

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "Usage: ./main <config_file> <id>" << std::endl;
    return 1;
  }
  int id = std::stoi(argv[2]);
  ServerNode node(argv[1], id);
  for (int i = 0; i < 20; i++) {
    if (id == 0) {
      node.send_message(
          "hellotheremyanmeifdfdsfrwjfrjfnrejnvefjnvfjenvekrnvkenvkrenvrinvirnv"
          "iefnvneviernvirenviernvirenverinverinverin");
    } else if (id == 1) {
      node.send_message("world");
    } else {
      node.send_message("foo");
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}