#include "gateway/gateway.h"

int main(int argc, char **argv) {
  vikraft::Gateway gateway(argv[1], 0);
  gateway.start();
  while (true) {
  }
  return 0;
}
