#include "messages.pb.h"
#include "tcp_client.h"
#include "tcp_server.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
namespace vikraft {

class NetworkManager {
public:
  NetworkManager(std::string config_path, int id) {
    // Load config
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
      throw std::runtime_error("Failed to open config file");
    }
    std::string line;
    while (std::getline(config_file, line)) {
      std::stringstream ss(line);
      std::string line_id;
      std::string ip;
      std::string port;
      ss >> line_id >> ip >> port;
      if (line_id == std::to_string(id)) {
        server_.start(std::stoi(port),
                      [](const vikraft::Message &message, int client_id) {
                        LOG(INFO) << "Received message from client "
                                  << client_id << ": " << message.type();
                      });
        LOG(INFO) << "Server started on port " << port;
      } else {
        clients_.push_back(TcpClient(ip, std::stoi(port)));
      }
    }
    config_file.close();
    for (auto &client : clients_) {
      client.connect();
    }
  };

  void send_message(const std::string &message) {
    for (auto &client : clients_) {
      client.send(message);
    }
  }

  ~NetworkManager() { server_.stop(); }

private:
  TcpServer server_;
  std::vector<TcpClient> clients_;
};
} // namespace vikraft