#include "backup-server.h"
#include "session.h"

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <memory.h>
#include <memory>

using namespace std;
using boost::asio::ip::tcp;

BackupServer::BackupServer(boost::asio::io_context &io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
  cout << "Server listening on port: " << port << endl;
  do_accept();
}

void BackupServer::do_accept() {
  acceptor_.async_accept(
      [this](boost::system::error_code error, tcp::socket socket) {
        if (!error) {
          // Create a session for the connected client
          make_shared<Session>(std::move(socket))->start();
        }

        // Accept the next connection

        cout << "Waiting for next connection..." << endl;
        do_accept();
      });
}