#include "networks/networks.h"
#include <asio/detail/type_traits.hpp>
#include <asio/io_context.hpp>
#include <sys/types.h>
enum class Game { FIRE, WATER, EARTH, PING_SERVER, PONG_SERVER, ALL_MESSAGE };

class CustomClient : public networks::Client_interface::Client<Game> {
public:
  CustomClient(asio::io_context &m_io_context, const std::string &host,
               uint16_t port)
      : networks::Client_interface::Client<Game>(m_io_context, host, port) {}

  void PingServer() {
    networks::message::message<Game> message1;
    int m1 = 3;
    message1.message_head.u_id = Game::PING_SERVER;
    message1 << m1;
    std::cout << "done";
    send(message1);
  }
  void MessageAll() {
    networks::message::message<Game> message2;
    std::string str2 = "Hello server ,use this data";
    message2.message_head.u_id = Game::ALL_MESSAGE;
    message2 << str2;
    send(message2);
  }
  void consume_InwardMessageQ(
      std::shared_ptr<networks::Connection::ConnectionObj<Game>> client,
      networks::message::message<Game> msg) override {
    std::cout << 2 << std::endl;
    switch (msg.message_head.u_id) {
    case Game::PONG_SERVER: {
      int rint;
      msg >> rint;
      std::cout << "[CLIENT]:" + std::to_string(rint) << std::endl;
    } break;
    case Game::ALL_MESSAGE: {
      // will do smthg
    } break;
    }
  };
};

bool key[3] = {false};
bool old_key[3] = {false};
bool bQuit = false;

void enableNonBlockingInput() {
  struct termios ttystate;
  tcgetattr(STDIN_FILENO, &ttystate);
  ttystate.c_lflag &= ~ICANON;
  ttystate.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

char getKeyPress() {
  char ch = 0;
  read(STDIN_FILENO, &ch, 1);
  return ch;
}

int main() {
  enableNonBlockingInput();
  asio::io_context io1;
  std::string host = "127.0.0.1";
  u_int32_t port = 5000;
  int bflag = 0;

  CustomClient c1(io1, host, port);
  c1.connect();
  while (!bflag) {
    char ch = getKeyPress();
    if (ch - '0' >= 0)
      std::cout << ch << std::endl;
    key[0] = ch == '1';
    key[1] = ch == '2';
    key[2] = ch == '3';

    if (key[0] && !old_key[0]) {
      c1.PingServer();
      std::cout << "ping started" << std::endl;
    }
    if (key[1] && !old_key[1])
      c1.MessageAll();
    if (key[2] && !old_key[2])
      bQuit = true;

    for (int i = 0; i < 3; i++)
      old_key[i] = key[i];

    c1.view_internalQ();

    usleep(100000);
  }
  return 0;
}
