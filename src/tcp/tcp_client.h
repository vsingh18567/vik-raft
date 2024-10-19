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
        std::this_thread::sleep_for(std::chrono::seconds(1));
      } else {
        break;
      }
    }
  }

  // delete copy constructor/assignment
  TcpClient(const TcpClient &) = delete;
  TcpClient &operator=(const TcpClient &) = delete;

  void send_to(uint32_t sender, const std::string &msg) {
    auto built_msg = build_message(msg, sender, MessageType::APPEND_ENTRIES);
    send(fd, built_msg.c_str(), built_msg.size(), 0);
  }

  ~TcpClient() { close(fd); }

private:
  std::string address;
  int port;
  int fd;
  struct sockaddr_in server_addr;
};
