#include "gateway/gateway.h"

int main(int argc, char **argv) {
  vikraft::Config config(argv[1]);
  vikraft::Gateway gateway(config, std::stoi(argv[2]));
  gateway.start();
  while (true) {
  }
  return 0;
}
