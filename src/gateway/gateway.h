#include "config.h"
#include "messages.pb.h"
#include "networking/tcp_client.h"
#include "networking/tcp_server.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace vikraft {

class Gateway : public MessageHandler {
public:
  Gateway(const std::string &config_path, int id)
      : config(config_path), id(id) {}

  void start() {
    tcp_server.start(config.gateways[id].port, this);
    nodes.reserve(config.cluster_size());
    for (const auto &node : config.nodes) {
      nodes.emplace_back(node.host, node.port);
      nodes.back().connect();
    }
  }

  void handle_message(const Message &message, int client_id,
                      int client_fd) override {
    int tries = 0;
    const int max_tries = 3;
    while (tries < max_tries) {
      if (!leader_id.has_value()) {
        leader_id = rand() % nodes.size();
      }
      LOG(INFO) << "Sending message to leader: " << leader_id.value();
      nodes[leader_id.value()].send(message.SerializeAsString());
      auto response_string = nodes[leader_id.value()].receive();
      Message response_message;
      response_message.ParseFromString(response_string);
      ClientCommandResponse response;
      response.ParseFromString(response_message.data());
      if (response.success()) {
        json j;
        j["success"] = true;
        tcp_server.send_message(client_fd, j.dump());
        return;
      } else if (!response.is_leader()) {
        LOG(INFO) << "Not leader, trying again";
        LOG(INFO) << "Leader id: " << response.leader_id();
        if (response.leader_id() < 0) {
          leader_id = (leader_id.value() + 1) % nodes.size();
        } else {
          leader_id = response.leader_id();
        }
      } else {
        LOG(INFO) << "Leader id: " << leader_id.value();
        break;
      }
      tries++;
      std::this_thread::sleep_for(std::chrono::milliseconds(50 * tries));
    }
    json j;
    j["success"] = false;
    tcp_server.send_message(client_fd, j.dump());
  }

private:
  Config config;
  int id;
  std::vector<TcpClient> nodes;
  GatewayTcpServer tcp_server;
  std::optional<int> leader_id;
};

} // namespace vikraft