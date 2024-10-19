#pragma once
#include "tcp/tcp_client.h"
#include "tcp/tcp_server.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ConfigParser {

public:
  ConfigParser(const std::string &config_file, uint32_t sid) {
    std::ifstream file(config_file);
    std::string line;
    std::unordered_map<int, std::pair<std::string, int>> client_addresses;
    while (std::getline(file, line)) {
      int comma = line.find(",");
      int this_id = std::stoi(line.substr(0, comma));
      std::string address_port = line.substr(comma + 1);
      int colon = address_port.find(":");
      std::string address = address_port.substr(0, colon);
      int port = std::stoi(address_port.substr(colon + 1));
      if (this_id == sid) {
        server = std::make_unique<TcpServer>(port);
      } else {
        client_addresses.emplace(
            std::make_pair(this_id, std::make_pair(address, port)));
      }
    }
    for (const auto &[id, data] : client_addresses) {
      clients.emplace(std::make_pair(
          id, std::make_unique<TcpClient>(data.first, data.second)));
    }
    server_valid = true;
    clients_valid = true;
  }

  std::unique_ptr<TcpServer> get_server() {
    if (!server_valid) {
      throw std::runtime_error("Server not valid");
    }
    server_valid = false;
    return std::move(server);
  }

  std::unordered_map<int, std::unique_ptr<TcpClient>> get_clients() {
    if (!clients_valid) {
      throw std::runtime_error("Clients not valid");
    }
    clients_valid = false;
    return std::move(clients);
  }

private:
  std::unique_ptr<TcpServer> server;
  std::unordered_map<int, std::unique_ptr<TcpClient>> clients;
  bool server_valid = false;
  bool clients_valid = false;
};