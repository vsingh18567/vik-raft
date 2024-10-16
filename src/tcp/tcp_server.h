#pragma once
#include "tcp_listener.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class TcpServer {

public:
  TcpServer(int port) : port(port) {
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      perror("socket failed");
      exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      perror("bind failed");
      exit(EXIT_FAILURE);
    }
  }

  void add_listener(TcpListener *listener) { listeners.push_back(listener); }

  static void open(TcpServer &server) {
    if (listen(server.fd, 3) < 0) {
      perror("listen");
      exit(EXIT_FAILURE);
    }
    while (true) {
      int new_socket;
      int addrlen = sizeof(server_addr);
      if ((new_socket =
               accept(server.fd, (struct sockaddr *)&server.server_addr,
                      (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
      }
      std::thread t(handle_connection, new_socket, server.listeners);
      t.detach();
    }
  }

  static void handle_connection(int new_socket,
                                std::vector<TcpListener *> listeners) {
    while (true) {
      char buffer[1024] = {0};
      int valread = read(new_socket, buffer, 1024);
      if (valread == 0) {
        std::cout << "Connection closed" << std::endl;
        break;
      }
      for (auto listener : listeners) {
        listener->on_message(std::string(buffer));
      }
    }
  }

  ~TcpServer() {
    std::cout << "Destructing server\n";
    close(fd);
  }

private:
  int port;
  int fd;
  struct sockaddr_in server_addr;
  std::vector<TcpListener *> listeners;
};
