#include "networks/networks.h"
enum class Game { PONG_SERVER };
int main() {
  std::cout << "12345" << std::endl;
  std::string message_con;
  std::cout << "SERVER [CONNECTION:"
               "]:"
            << std::endl;
  networks::message::message<Game> response;
  response.message_head.u_id = Game::PONG_SERVER;
  std::cout << "i";
  response << 1;
  std::cout << "j";
  int ij;
  response >> ij;
  std::cout << ij;

  return 0;
}
