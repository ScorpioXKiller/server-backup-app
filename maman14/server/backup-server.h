#pragma once

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>

class BackupServer {
public:
  BackupServer(boost::asio::io_context &io_context, short port);

private:
  void do_accept();
  boost::asio::ip::tcp::acceptor acceptor_;
};