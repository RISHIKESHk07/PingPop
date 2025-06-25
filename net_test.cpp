#include "networks/networks.h"
#include <asio/io_context.hpp>
#include <map>
#include <string>
#include <sys/types.h>

enum class Game : uint32_t {
  FIRE,
  WATER,
  EARTH,
  PING_SERVER,
  ALL_MESSAGE,
  PONG_SERVER
};

class CustomServer : public networks::Server_interface::Server<Game> {
public:
  CustomServer(asio::io_context &context, uint32_t port)
      : networks::Server_interface::Server<Game>(context, port) {}
  void consume_InwardMessageQ(
      std::shared_ptr<networks::Connection::ConnectionObj<Game>> client,
      networks::message::message<Game> &msg) override {
    switch (msg.message_head.u_id) {
    case Game::PING_SERVER: {
      std::cout << "12345" << std::endl;
      int o = 2;
      msg >> o;
      std::cout << "SERVER [CONNECTION:" + std::to_string(client->getId()) +
                       "]:"
                << std::endl;
      networks::message::message<Game> response;
      response.message_head.u_id = Game::PONG_SERVER;
      response << int(client->getId());
      std::cout << "messageing" << std::endl;
      client->send_message_connection(response);
    } break;
    case Game::ALL_MESSAGE: {
      // will do smthg
    } break;
    }
  };
};

int main() {
  asio::io_context server_context;
  std::cout << "TEST:";
  CustomServer s1(server_context, 5000);
  s1.listen();
  while (1) {
    // will consume here
    s1.processing_messages_internalQ();
  }
  std::cout << "[SERVER]: Press Enter to quit..." << std::endl;
  std::cin.get();
  return 0;
}
