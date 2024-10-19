#pragma once
#include "tcp_client.h"
#include "tcp_server.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Connections {
  std::unique_ptr<TcpServer> server;
  std::unordered_map<int, std::unique_ptr<TcpClient>> clients;
};

Connections build_connections(const std::string &config_file, uint32_t sid) {
  std::ifstream file(config_file);
  std::string line;
  std::unordered_map<int, std::pair<std::string, int>> client_addresses;
  Connections connections;
  while (std::getline(file, line)) {
    int comma = line.find(",");
    int this_id = std::stoi(line.substr(0, comma));
    std::string address_port = line.substr(comma + 1);
    int colon = address_port.find(":");
    std::string address = address_port.substr(0, colon);
    int port = std::stoi(address_port.substr(colon + 1));
    if (this_id == sid) {
      connections.server = std::make_unique<TcpServer>(port);
      std::thread(TcpServer::open, std::ref(*connections.server)).detach();

    } else {
      client_addresses.emplace(
          std::make_pair(this_id, std::make_pair(address, port)));
    }
  }
  for (const auto &[id, data] : client_addresses) {
    connections.clients.emplace(std::make_pair(
        id, std::make_unique<TcpClient>(data.first, data.second)));
  }
  return connections;
}
