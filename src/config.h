#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
using json = nlohmann::json;

namespace vikraft {

struct NodeConfig {
  std::string host;
  int port;
};

struct GatewayConfig {
  std::string host;
  int port;
};

class Config {
public:
  Config(const std::string &config_path) {
    std::ifstream config_file(config_path);
    json config_json;
    config_file >> config_json;
    for (const auto &node : config_json["nodes"]) {
      nodes.push_back(
          NodeConfig{node["host"].get<std::string>(), node["port"].get<int>()});
    }
    for (const auto &gateway : config_json["gateways"]) {
      gateways.push_back(GatewayConfig{gateway["host"].get<std::string>(),
                                       gateway["port"].get<int>()});
    }
  }

  size_t cluster_size() const { return nodes.size(); }

  std::vector<NodeConfig> nodes;
  std::vector<GatewayConfig> gateways;
};

} // namespace vikraft
