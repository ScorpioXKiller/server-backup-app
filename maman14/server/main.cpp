#include "backup-server.h"

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  try {
    if (argc != 2) {
      cerr << "Usage: BackupServer <port>" << endl;
      return 1;
    }

    short port = static_cast<short>(atoi(argv[1]));

    boost::asio::io_context io_context;
    BackupServer server(io_context, port);

    io_context.run();
  } catch (const exception &e) {
    cerr << "Exception: " << e.what() << endl;
  }

  return 0;
}