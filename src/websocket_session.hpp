// Copyright (c) 2025 Rainer Leuschke
// University of Washington, CREST lab

#include <cstdlib>
#include <memory>
#include <string>
#include <iostream>
#include <functional>
#include <queue>
#include <stdbool.h>

#include <boost/asio.hpp>

namespace net = boost::asio;                    // namespace asio
using tcp = net::ip::tcp;                       // from <boost/asio/ip/tcp.hpp>
using error_code = boost::system::error_code;   // from <boost/system/error_code.hpp>

#include <boost/beast.hpp>

namespace beast = boost::beast;
namespace http = boost::beast::http;            // from <boost/beast/http.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

/**
 * @brief Websocket_Session Class is a websocket client handling a connection
 * to a websocket server
 */
class websocket_session : public std::enable_shared_from_this<websocket_session>
{
   tcp::resolver resolver_;
   websocket::stream<beast::tcp_stream> ws_;
   beast::flat_buffer buffer_;
   std::string host_;
   std::string target_;
   std::function<void(std::string)> readCallback;
   std::function<void(std::string)> handshakeCallback;
   std::queue<std::string> message_queue;
   bool write_scheduled = false;
   bool verbose_ = false;

   void fail(error_code ec, char const* what);
   void on_resolve(error_code ec, tcp::resolver::results_type results);
   void on_connect(error_code ec, tcp::resolver::results_type::endpoint_type ep);
   void on_handshake(error_code ec);
   void on_write(error_code ec, std::size_t bytes_transferred);
   void on_read(error_code ec, std::size_t bytes_transferred);
   void on_close(error_code ec);
   mutable std::mutex qmutex;

public:
   explicit websocket_session(net::io_context& ioc);
   ~websocket_session();

   void run(std::string host, std::string port, std::string target);
   void registerReadCallback(std::function<void(std::string)> cb);
   void registerHandshakeCallback(std::function<void(std::string)> cb);
   void do_write(std::string message);
   void do_close();
   void set_verbose(bool flag);
};
