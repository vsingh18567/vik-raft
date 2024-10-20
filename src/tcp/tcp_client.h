#pragma once
#include "messages.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

class TcpClient {
public:
  TcpClient(std::string address, int port) : address(address), port(port) {

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));

      if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
      }
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(port);

      if (inet_pton(AF_INET, address.c_str(), &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
      }
      if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
          0) {
        std::cout << "Connection failed, retrying in 1 second" << std::endl;
      } else {
        LOG("Connected to " + address + ":" + std::to_string(port));
        break;
      }
    }
  }

  // delete copy constructor/assignment
  TcpClient(const TcpClient &) = delete;
  TcpClient &operator=(const TcpClient &) = delete;

  void send_to(uint32_t sid, const std::string &msg, MessageType type) {
    auto msg_str = build_message(msg, sid, type);
    if (send(fd, msg_str.c_str(), msg_str.size(), 0) < 0) {
      perror("send failed");
      exit(EXIT_FAILURE);
    }
  }

  ~TcpClient() { close(fd); }

private:
  std::string address;
  int port;
  int fd;
  struct sockaddr_in server_addr;
};
