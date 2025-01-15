#pragma once

#include "absl/log/log.h"
#include <arpa/inet.h>
#include <chrono>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace vikraft {

class TcpClient {
public:
  TcpClient(const std::string host, int port)
      : host_(host), port_(port), socket_fd_(-1) {}

  ~TcpClient() { disconnect(); }

  void connect() {
    while (true) {
      socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
      if (socket_fd_ < 0) {
        LOG(WARNING) << "Failed to create socket, retrying...";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
      }

      struct sockaddr_in server_addr;
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(port_);

      if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
        close(socket_fd_);
        LOG(WARNING) << "Invalid address, retrying...";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
      }

      if (::connect(socket_fd_, (struct sockaddr *)&server_addr,
                    sizeof(server_addr)) < 0) {
        close(socket_fd_);
        LOG(WARNING) << "Connection failed, retrying...";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
      }

      LOG(INFO) << "Connected to " << host_ << ":" << port_;
      break;
    }
  }

  void send(const std::string &data) {
    if (socket_fd_ < 0) {
      throw std::runtime_error("Not connected");
    }

    ssize_t bytes_sent = ::send(socket_fd_, data.c_str(), data.length(), 0);
    if (bytes_sent < 0) {
      LOG(WARNING) << "Failed to send data to " << host_ << ":" << port_;
      disconnect();
    }
  }

  std::string receive(size_t max_length = 1024) {
    if (socket_fd_ < 0) {
      throw std::runtime_error("Not connected");
    }

    std::string buffer(max_length, 0);
    ssize_t bytes_received = recv(socket_fd_, &buffer[0], max_length, 0);

    if (bytes_received < 0) {
      throw std::runtime_error("Failed to receive data");
    }

    buffer.resize(bytes_received);
    return buffer;
  }

  void disconnect() {
    LOG(INFO) << "Disconnecting from " << host_ << ":" << port_;
    if (socket_fd_ >= 0) {
      close(socket_fd_);
      socket_fd_ = -1;
      LOG(INFO) << "Disconnected";
    }
  }

  bool is_connected() const { return socket_fd_ >= 0; }

private:
  const std::string host_;
  const int port_;
  int socket_fd_;
};

} // namespace vikraft
