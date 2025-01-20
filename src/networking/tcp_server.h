#pragma once

#include "absl/log/log.h"
#include "networking/message_handler.h"
#include "node/cluster_members.h"
#include "node/node_id.h"
#include <arpa/inet.h>
#include <functional>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>
namespace vikraft {

int initialize_socket(int port) {
  int socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ < 0) {
    throw std::runtime_error("Failed to create socket");
  }

  // Allow port reuse
  int opt = 1;
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    close(socket_fd_);
    throw std::runtime_error("Failed to set socket options");
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(socket_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    close(socket_fd_);
    throw std::runtime_error("Failed to bind socket");
  }

  if (listen(socket_fd_, SOMAXCONN) < 0) {
    close(socket_fd_);
    throw std::runtime_error("Failed to listen on socket");
  }

  return socket_fd_;
}

class TcpServer {
public:
  TcpServer() : socket_fd_(-1), running_(false) {}

  ~TcpServer() { stop(); }

  void start(int port, MessageHandler *handler) {
    if (running_) {
      throw std::runtime_error("Server is already running");
    }
    socket_fd_ = initialize_socket(port);
    running_ = true;
    handler_ = handler;

    accept_thread_ = std::thread(&TcpServer::accept_connections, this);
    LOG(INFO) << "Server started on port " << port;
  }

  void stop() {
    if (running_) {
      running_ = false;

      // Close all client sockets
      for (auto &client_thread : client_threads_) {
        if (client_thread.joinable()) {
          client_thread.join();
        }
      }
      client_threads_.clear();

      // Close server socket
      if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
      }

      // Join accept thread
      if (accept_thread_.joinable()) {
        accept_thread_.join();
      }

      LOG(INFO) << "Server stopped";
    }
  }

  bool send_message(int client_fd, const std::string &data) {
    ssize_t total_sent = 0;
    ssize_t len = data.length();

    while (total_sent < len) {
      ssize_t sent =
          send(client_fd, data.c_str() + total_sent, len - total_sent, 0);
      if (sent < 0) {
        LOG(ERROR) << "Failed to send message to client";
        return false;
      }
      total_sent += sent;
    }
    return true;
  }

protected:
  void accept_connections() {
    while (running_) {
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(client_addr);

      int client_fd =
          accept(socket_fd_, (struct sockaddr *)&client_addr, &client_len);
      if (client_fd < 0) {
        if (running_) {
          LOG(ERROR) << "Failed to accept connection";
        }
        continue;
      }

      // Start a new thread for each client
      client_threads_.emplace_back(&TcpServer::handle_client, this, client_fd);
      LOG(INFO) << "New client connected";
    }
  }

  virtual void handle_client(int client_fd) = 0;

  int socket_fd_;
  bool running_;
  MessageHandler *handler_;
  std::thread accept_thread_;
  std::vector<std::thread> client_threads_;
};

class NodeTcpServer : public TcpServer {
public:
  NodeTcpServer(ClusterMembers &cluster_members,
                std::function<void(NodeId)> on_reconnect,
                std::function<void(NodeId)> on_disconnect)
      : cluster_members_(cluster_members), on_reconnect_(on_reconnect),
        on_disconnect_(on_disconnect) {}

private:
  void handle_client(int client_fd) {
    const size_t buffer_size = 1024;
    std::string buffer(buffer_size, 0);
    NodeId client_id = -1;
    while (running_) {
      ssize_t bytes_received = recv(client_fd, &buffer[0], buffer_size, 0);

      if (bytes_received <= 0) {
        if (client_id != -1) {
          LOG(INFO) << "Client disconnected or error";

          on_disconnect_(client_id);
        } else {
          LOG(INFO) << "Uninitialized client disconnected or error";
        }
        break; // Client disconnected or error
      }

      buffer.resize(bytes_received);

      vikraft::Message message;
      message.ParseFromString(buffer);
      if (message.type() == vikraft::MessageType::CONNECT) {
        vikraft::Connect connect;
        connect.ParseFromString(message.data());
        client_id = connect.id();
        on_reconnect_(client_id);
        LOG(INFO) << "Client connected with id " << client_id;
      } else {
        handler_->handle_message(message, client_id,
                                 client_fd); // I do not like that client_fd
                                             // and client_id needs to be
                                             // passed around, but it works
                                             // for now
      }
      buffer.resize(buffer_size);
    }

    close(client_fd);
    LOG(INFO) << "Client disconnected";
  }

  ClusterMembers &cluster_members_;
  std::function<void(NodeId)> on_reconnect_;
  std::function<void(NodeId)> on_disconnect_;
};

class GatewayTcpServer : public TcpServer {
private:
  void handle_client(int client_fd) {
    const size_t buffer_size = 1024;
    std::string buffer(buffer_size, 0);
    while (running_) {
      ssize_t bytes_received = recv(client_fd, &buffer[0], buffer_size, 0);

      if (bytes_received <= 0) {
        LOG(INFO) << "Client disconnected or error";
        break; // Client disconnected or error
      }

      buffer.resize(bytes_received);
      try {
        json j = json::parse(buffer);
        std::string data = j["data"];
        vikraft::Message message;
        message.set_type(vikraft::MessageType::CLIENT_COMMAND);
        ClientCommand command;
        command.set_command(data);
        message.set_data(command.SerializeAsString());
        handler_->handle_message(message, -1,
                                 client_fd); // client id not relevant for
                                             // gateway

      } catch (const std::exception &e) {
        LOG(ERROR) << "Failed to parse message: " << e.what();
        send_message(client_fd, "{\"success\": false}");
        continue;
      }
      buffer.resize(buffer_size);
    }
  }
};
} // namespace vikraft
