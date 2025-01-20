#include "config.h"
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

    tcp_server = std::make_unique<TcpServer>({}, config.gateways[id].host,
                                             config.gateways[id].port);
    tcp_server->start(this);
  }

  void handle_message(const std::string &message) override {
    void handle_message(const std::string &message) override {
      std::cout << "Received message: " << message << std::endl;
    }

  private:
    Config config;
    int id;
    std::unique_ptr<TcpClient> tcp_client;
    std::unique_ptr<TcpServer> tcp_server;
  };

} // namespace vikraft