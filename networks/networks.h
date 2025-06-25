#pragma once
#include "asio.hpp"
#include <asio/impl/read.hpp>
#include <asio/impl/write.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sys/types.h>
#include <system_error>
#include <thread>

namespace networks {
namespace Connection {
template <typename T> class ConnectionObj;
}

//------------------------------------------Thread
// queue-------------------------
namespace TSQueue {

template <typename T> class TsQueue {
public:
  TsQueue() {};
  TsQueue(const TsQueue<T> &) =
      delete; // do not allow the copying of the q object
  virtual ~TsQueue() { clear(); }
  T &front() {
    std::scoped_lock lock(DQ_mutex);
    return DQ.front();
  }
  T &back() {
    std::scoped_lock lock(DQ_mutex);
    return DQ.back();
  }
  T pop_front() {
    std::scoped_lock lock(DQ_mutex);
    auto t = std::move(DQ.front());
    DQ.pop_front();
    return t;
  };
  T &pop_back() {
    std::scoped_lock lock(DQ_mutex);
    auto t1 = std::move(DQ.back());
    DQ.pop_back();
    return t1;
  };
  void clear() {
    std::scoped_lock lock(DQ_mutex);
    DQ.clear();
  }
  bool empty() {
    std::scoped_lock lock(DQ_mutex);
    return DQ.empty();
  }
  void push_back(const T &item) {
    std::scoped_lock lock(DQ_mutex);
    DQ.emplace_back(std::move(item));
  }
  size_t count() { return DQ.size(); }

private:
  std::deque<T> DQ;
  std::mutex DQ_mutex;
};

} // namespace TSQueue

//------------------------------------- Message structs
//-----------------------------
// namespace networks

namespace message {
template <typename T> struct message_header {
  T u_id;
  size_t size_of_the_message = 0;
};

template <typename T> struct message {
  networks::message::message_header<T> message_head;
  std::vector<u_int8_t> message_body;

  size_t size() { return message_body.size(); }

  friend std::ostream &operator<<(std::ostream &os, const message<T> &msg) {
    os << "ID:" << int(msg.message_head.u_id)
       << " Size:" << msg.message_head.size_of_the_message;
    return os;
  }

  // Pushes any POD-like data into the message buffer
  template <typename DataType,
            typename std::enable_if<std::is_trivially_copyable<DataType>::value,
                                    int>::type = 0>
  friend message<T> &operator<<(message<T> &msg, const DataType &data) {
    // Check that the type of the data being pushed is trivially copyable
    static_assert(std::is_standard_layout<DataType>::value,
                  "Data is too complex to be pushed into vector");

    // Cache current size of vector, as this will be the point we insert the
    // data
    size_t i = msg.message_body.size();

    // Resize the vector by the size of the data being pushed
    msg.message_body.resize(msg.message_body.size() + sizeof(DataType));

    // Physically copy the data into the newly allocated vector space
    std::memcpy(msg.message_body.data() + i, &data, sizeof(DataType));

    // Recalculate the message size
    msg.message_head.size_of_the_message = msg.size();

    // Return the target message so it can be "chained"
    return msg;
  }

  // Pulls any POD-like data form the message buffer
  template <typename DataType,
            typename std::enable_if<std::is_trivially_copyable<DataType>::value,
                                    int>::type = 0>
  friend message<T> &operator>>(message<T> &msg, DataType &data) {
    // Check that the type of the data being pushed is trivially copyable
    static_assert(std::is_standard_layout<DataType>::value,
                  "Data is too complex to be pulled from vector");

    // Cache the location towards the end of the vector where the pulled data
    // starts
    size_t i = msg.message_body.size() - sizeof(DataType);

    // Physically copy the data from the vector into the user variable
    std::memcpy(&data, msg.message_body.data() + i, sizeof(DataType));

    // Shrink the vector to remove read bytes, and reset end position
    msg.message_body.resize(i);

    // Recalculate the message size
    msg.message_head.size_of_the_message = msg.size();

    // Return the target message so it can be "chained"
    return msg;
  }
};
// Serialize string
template <typename T>
message<T> &operator<<(message<T> &msg, const std::string &str) {
  size_t len = str.size();
  std::cout << "[DEBUG] Serializing string of length: " << len << std::endl;
  msg << len; // use existing overload for size_t
  msg.message_body.insert(msg.message_body.end(), str.begin(), str.end());
  msg.message_head.size_of_the_message = msg.size();
  return msg;
}

// Deserialize string
template <typename T>
message<T> &operator>>(message<T> &msg, std::string &str) {
  size_t len;
  msg >> len; // use existing overload
  std::cout << "[DEBUG] Serializing string of length: " << len << std::endl;
  if (len > msg.message_body.size()) {
    std::cerr << "[ERROR] Invalid string length during deserialization\n";
    throw std::runtime_error("Invalid message length");
  }

  size_t start = msg.message_body.size() - len;
  str.assign(reinterpret_cast<const char *>(&msg.message_body[start]), len);
  msg.message_body.resize(start);
  msg.message_head.size_of_the_message = msg.size();
  return msg;
}
template <typename T> struct ownned_message {
  std::shared_ptr<networks::Connection::ConnectionObj<T>>
      client_owner_respective_connection;
  networks::message::message<T> message;
};

} // namespace message

//--------------------------------ConnectionObj----------------

namespace Connection {

template <typename T>
class ConnectionObj : public std::enable_shared_from_this<ConnectionObj<T>> {
public:
  enum class owner { client, server };
  ConnectionObj(owner owner_type, asio::ip::tcp::socket &m_socket,
                asio::io_context &m_iocontext,
                networks::TSQueue::TsQueue<networks::message::ownned_message<T>>
                    &remote_inQ)
      : connection_io(m_iocontext), connection_socket(std::move(m_socket)),
        pointer_internalQ_of_remote_end(remote_inQ), owner_type(owner_type) {}
  u_int32_t getId() { return connection_id; }

  void read_header_from_remote_end() {
    std::cout << "reading.." << std::endl;
    asio::async_read(
        connection_socket,
        asio::buffer(&tempo_read_message_buffer.message_head,
                     sizeof(networks::message::message_header<T>)),
        [this](std::error_code ec, std::size_t length) {
          if (!ec) {
            if (tempo_read_message_buffer.message_head.size_of_the_message >
                0) {
              tempo_read_message_buffer.message_body.resize(
                  tempo_read_message_buffer.message_head
                      .size_of_the_message); // resize the buff for body/whole
                                             // message
              read_body_from_remote_end();
            } else {
              if (owner_type ==
                  networks::Connection::ConnectionObj<T>::owner::server) {
                pointer_internalQ_of_remote_end.push_back(
                    {this->shared_from_this(), tempo_read_message_buffer});
              } else {
                pointer_internalQ_of_remote_end.push_back(
                    {nullptr, tempo_read_message_buffer});
              }
              read_header_from_remote_end();
              // message header directly  into internalQ
            }
          } else {
            std::cout << "[SERVER]:Reading Header error" << std::endl;
            std::cout << "[CLIENT]:" + std::to_string(this->getId()) +
                             "Happens to have a error..." + ec.message()
                      << std::endl;
            connection_socket.close();
          }
        }); // read header from the line here;
  };

  void start_reading_from_connection() {
    if (isConnected()) {
      read_header_from_remote_end();
    }
  }

  void connectToClient(u_int32_t uid) {
    if (isConnected() && owner_type == owner::server) {
      std::cout << connection_socket.is_open() << "---" << int(isConnected())
                << std::endl;
      read_header_from_remote_end();
      connection_id = uid;
    }
  }
  void connectToServer(asio::ip::tcp::resolver::results_type &endpoints) {
    if (owner_type == owner::client) {
      asio::async_connect(
          connection_socket, endpoints,
          [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
            if (!ec) {
              std::cout << "CLIENT:Connection succesful on client side"
                        << std::endl;
              read_header_from_remote_end();
            } else {
              std::cout << "[CLIENT]:Errorconnecting to server" + ec.message()
                        << std::endl;
            }
          });
    }
  }

  void send_message_connection(
      networks::message::message<T> message_from_the_server) {
    // push into outward Q then check if the a message exists & call
    if (isConnected()) {
      try {
        std::cout << "post";
        asio::post(connection_io, [this, message_from_the_server]() {
          outward_q_from_the_current_entity.push_back(message_from_the_server);
          if (!outward_q_from_the_current_entity.empty())
            write_messageh_to_stream_from_OutwardMessageQ();
        });
      } catch (std::exception err) {
        std::cout << "EEEEEEEEEEEE";
      }
    } else {
      std::cout << "[SERVER]:" + std::to_string(this->getId()) + "... is closed"
                << std::endl;
    }
  }

  bool isConnected() {
    if (connection_socket.is_open())
      return true;
    else
      return false;
  }

  bool disconnect() {
    if (isConnected())
      asio::post(connection_io, [this]() { connection_socket.close(); });
  }

  size_t size_of_outwardQ() { return outward_q_from_the_current_entity.size(); }

  void read_body_from_remote_end() {
    std::cout << "body reading ..." << std::endl;
    std::cout << int(owner_type == owner::server) << std::endl;

    // read body of the message next and then push to the Q
    asio::async_read(
        connection_socket,
        asio::buffer(tempo_read_message_buffer.message_body.data(),
                     tempo_read_message_buffer.message_body.size()),
        [this](std::error_code ec, std::size_t length) {
          if (!ec) {
            std::cout << int(owner_type == owner::server) << std::endl;
            if (owner_type == owner::server) {
              std::cout << "here.....exe..." << tempo_read_message_buffer
                        << std::endl;
              pointer_internalQ_of_remote_end.push_back(
                  {this->shared_from_this(), tempo_read_message_buffer});
            } else {
              pointer_internalQ_of_remote_end.push_back(
                  {nullptr, tempo_read_message_buffer});
            }
            read_header_from_remote_end();
          } else {
            std::cout << "[SERVER]:Error Reading message_body" << ec.message()
                      << std::endl;
          };
        });
  }

  void write_messageh_to_stream_from_OutwardMessageQ() {
    asio::async_write(
        connection_socket,
        asio::buffer(&outward_q_from_the_current_entity.front().message_head,
                     sizeof(networks::message::message_header<T>)),
        [this](std::error_code ec, std::size_t lenght) {
          if (!ec) {
            if (outward_q_from_the_current_entity.front()
                    .message_head.size_of_the_message > 0) {
              write_body_from_outwardMessageQ_to_stream();
            } else {
              outward_q_from_the_current_entity.pop_front();
              if (!outward_q_from_the_current_entity.empty()) {
                write_messageh_to_stream_from_OutwardMessageQ();
              }
            }
          } else {
            std::cout << "[Server]: Error write header" << std::endl;
            connection_socket.close();
          }
        });
  }
  void write_body_from_outwardMessageQ_to_stream() {
    asio::async_write(
        connection_socket,
        asio::buffer(
            outward_q_from_the_current_entity.front().message_body.data(),
            outward_q_from_the_current_entity.front().message_body.size()),
        [this](std::error_code ec, std::size_t length) {
          if (!ec) {
            outward_q_from_the_current_entity.pop_front();

            if (!outward_q_from_the_current_entity.empty())
              write_messageh_to_stream_from_OutwardMessageQ();

          } else {
            std::cout << "[SERVER]:Error reading body ..." + ec.message()
                      << std::endl;
            connection_socket.close();
          }
        });
  }

protected:
private:
  asio::io_context &connection_io;
  asio::ip::tcp::socket connection_socket;
  networks::TSQueue::TsQueue<networks::message::message<T>>
      outward_q_from_the_current_entity;
  networks::TSQueue::TsQueue<networks::message::ownned_message<T>>
      &pointer_internalQ_of_remote_end;
  int connection_id;
  networks::message::message<T> tempo_read_message_buffer;
  owner owner_type;
};
}; // namespace Connection

//-------------------------------Client----------------------
namespace Client_interface {
template <typename T> class Client {
public:
  Client(asio::io_context &m_iocontext, std::string host, u_int32_t port)
      : client_io_context(m_iocontext), client_socket(client_io_context),
        host(host), port(port) {};
  virtual ~Client() { disconnect(); };
  void connect() {
    asio::ip::tcp::resolver resolver(client_io_context);
    auto end = resolver.resolve(host, std::to_string(port));
    auto new_client = std::make_shared<networks::Connection::ConnectionObj<T>>(
        networks::Connection::ConnectionObj<T>::owner::client, client_socket,
        client_io_context, CInwardMessageQ);
    client_connection = new_client;
    client_connection->connectToServer(end);
    client_thread = std::thread([this]() { client_io_context.run(); });
  };
  void disconnect() {
    client_io_context.stop();
    if (client_thread.joinable())
      client_thread.join();
    std::cout << "[CLIENT]: Disconnected" << std::endl;
  };
  bool isConnected() {
    if (client_connection)
      return client_connection
          ->isConnected(); // here due to moving of the socket from the org obj
                           // in this client to connectionobj object it would
                           // show the socket is closed here .... mostly likely
                           // reason
    else
      return false;
  };
  void send(const message::message<T> &mess) {
    std::cout << "before" << " --- " << int(isConnected()) << std::endl;
    if (isConnected()) {
      std::cout << "here" << std::endl;
      client_connection->send_message_connection(mess);
    }
  }
  networks::TSQueue::TsQueue<T> Incoming_q_server() { return CInwardMessageQ; }

  void view_internalQ() {
    if (!CInwardMessageQ.empty() && client_connection->isConnected()) {
      std::cout << "processing_started_client" << std::endl;
      networks::message::ownned_message<T> processing_msg =
          CInwardMessageQ.pop_front();
      consume_InwardMessageQ(processing_msg.client_owner_respective_connection,
                             processing_msg.message);
    }
  }

protected:
  virtual void consume_InwardMessageQ(
      std::shared_ptr<networks::Connection::ConnectionObj<T>> connection,
      networks::message::message<T> message_recieved) = 0;

private:
  std::thread client_thread;
  asio::io_context &client_io_context;
  asio::ip::tcp::socket client_socket;
  std::shared_ptr<networks::Connection::ConnectionObj<T>> client_connection;
  networks::TSQueue::TsQueue<networks::message::ownned_message<T>>
      CInwardMessageQ; // client inward q , messaged from the server
  int client_id;
  u_int32_t port;
  std::string host; // assigned from the server
};
} // namespace Client_interface
// --------------------------Server-------------------------
namespace Server_interface {
template <typename T> class Server {
public:
  Server(asio::io_context &m_iocontext, u_int32_t port)
      : server_io_context(m_iocontext), server_socket(server_io_context),
        acceptorObj(m_iocontext,
                    asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
    this->port = port;
    std::cout << "[SERVER]:HERE ..." << std::endl;
  };
  virtual ~Server() { stop(); };
  void listen() {
    WaitForConnections();
    server_thread = std::thread([this]() { server_io_context.run(); });
  }; // listen for connections using async connect here & create a q of
     // conections to process
  void WaitForConnections() {
    acceptorObj.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket in_socket) {
          if (!ec) {

            std::shared_ptr<networks::Connection::ConnectionObj<T>>
                new_client_conn =
                    std::make_shared<networks::Connection::ConnectionObj<T>>(
                        networks::Connection::ConnectionObj<T>::owner::server,
                        in_socket, server_io_context, SInwardMessageQ);
            if (Client_approve_connection()) {
              ClientQ.push_back(std::move(new_client_conn));
              ClientQ.back()->connectToClient(IntialIds++);
              std::cout << "[Server]:Connection Accepted to ID->" +
                               std::to_string(ClientQ.back()->getId())
                        << std::endl;

              WaitForConnections();

            } else {
              std::cout << "[SERVER]:denied connection";
            };
            WaitForConnections(); // if client rejects us

          } else {
            std::cout << "[Server]:" + ec.message() << std::endl;
          }
        });
  }
  void
  MessageClient(std::shared_ptr<networks::Connection::ConnectionObj<T>> CLIENT,
                networks::message::message<T> &mess) {
    if (CLIENT->isConnected()) {
      CLIENT->send_message_connection(mess);
    } else {
      CLIENT.reset();
      ClientQ.erase(std::remove(ClientQ.begin(), ClientQ.end(), CLIENT),
                    ClientQ.end());
    };
  }
  void MessageAllClients(
      std::shared_ptr<networks::Connection::ConnectionObj<T>> IGNORED_CLIENT,
      networks::message::message<T> &message) {
    bool flag_indicates_loss_conns = false;
    for (auto client : ClientQ) {
      if (client != IGNORED_CLIENT) {
        if (client.isConnected()) {
          client->send(message);
        } else {
          std::cout << client->getId() << ": Disconnected"
                    << std::cout; // Disconnected client patch here
          client.reset();
          flag_indicates_loss_conns = true;
        }
      };
    }
    if (flag_indicates_loss_conns)
      ClientQ.erase(std::remove(ClientQ.begin(), ClientQ.end(), nullptr),
                    ClientQ.end());
  }

  void stop() {
    server_io_context.stop();
    if (server_thread.joinable()) {
      server_thread.join();
    }
    std::cout << "[SERVER]: Stopped" << std::endl;
  };

  void processing_messages_internalQ() {
    int count_limit = 0;
    int max_limit = 5;
    if (SInwardMessageQ.count() > 0)
      printer_Qs();
    while (count_limit < max_limit && !SInwardMessageQ.empty()) {
      std::cout << SInwardMessageQ.count() << std::endl;
      networks::message::ownned_message<T> messagetemp =
          SInwardMessageQ.pop_front();
      // consume the message over here
      std::cout << "[SERVER]:" << messagetemp.message << std::endl;
      consume_InwardMessageQ(messagetemp.client_owner_respective_connection,
                             messagetemp.message);

      count_limit++;
    }
  };

  void printer_Qs() {
    std::cout << "SERVER internalQ:" + int(SInwardMessageQ.count())
              << std::endl;
  }

protected:
  virtual void consume_InwardMessageQ(
      std::shared_ptr<networks::Connection::ConnectionObj<T>> client,
      networks::message::message<T> &msg) = 0;
  // hadlers for user overriding here to perform various stuff like process
  // messages and then send more messages;
  void OnClientConnect();
  void OnClientDisconnect();
  bool Client_approve_connection() { return true; };

private:
  asio::io_context &server_io_context;
  asio::ip::tcp::socket server_socket;
  std::thread server_thread;
  int IntialIds = 456;
  u_int32_t port = 5000;
  networks::TSQueue::TsQueue<networks::message::ownned_message<T>>
      SInwardMessageQ;
  std::deque<std::shared_ptr<networks::Connection::ConnectionObj<T>>> ClientQ;
  asio::ip::tcp::acceptor acceptorObj;
};

} // namespace Server_interface
} // namespace networks
