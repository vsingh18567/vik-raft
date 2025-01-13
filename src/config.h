#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace vikraft {

struct NodeConfig {
  std::string host;
  int port;
};

class Config {
public:
  Config(const std::string &config_path) {
    std::ifstream config_file(config_path);
    std::string line;

    while (std::getline(config_file, line)) {
      std::istringstream iss(line);
      std::string host;
      int port;

      if (!(iss >> host >> port)) {
        throw std::runtime_error("Invalid config file format");
      }

      nodes.push_back(NodeConfig{host, port});
    }
  }

  size_t cluster_size() const { return nodes.size(); }

  std::vector<NodeConfig> nodes;
};

} // namespace vikraft
