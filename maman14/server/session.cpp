#include "session.h"
#include "protocols.h"

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <vector>

using namespace std;
using namespace boost::asio;
using namespace status_code;

static const string BACKUP_ROOT = "c:\\backupsvr\\";

Session::Session(ip::tcp::socket socket) : socket_(std::move(socket)) {}

void Session::start() { do_read_header(); }

/*void Session::do_read_header() {
  auto self(shared_from_this());
  buffer_.resize(9); // 1 byte for version, 2 bytes for command, 4 bytes for
                     // user id, 2 bytes for payload size
  async_read(socket_, buffer(buffer_),
             [this, self](boost::system::error_code error, std::size_t length) {
               if (!error && length == 9) {
                 handle_header();
               } else {
                 socket_.close();
               }
             });
}*/

void Session::do_read_header() {
  auto self(shared_from_this());
  buffer_.resize(1024); // Allocate a buffer for a simple text message

  async_read(socket_, buffer(buffer_),
             [this, self](boost::system::error_code error, std::size_t length) {
               if (!error) {
                 string message(buffer_.data(), length);
                 cout << "Received message from client: " << message << endl;

                 if (message == "Hello from client") {
                   string response = "Hello, Client. I'm ready to help you.";
                   do_write_response(response);
                 } else {
                   string response = "Unknown message received.";
                   do_write_response(response);
                 }
               } else {
                 cerr << "Error reading message: " << error.message() << endl;
               }
             });
}

void Session::do_write_response(const string &response) {
  auto self(shared_from_this());
  async_write(socket_, buffer(response),
              [this, self, response](boost::system::error_code error,
                                     std::size_t /*length*/) {
                if (error) {
                  cerr << "Error sending response: " << error.message() << endl;
                } else {
                  cout << "Response sent: " << response << endl;
                }
              });
}

void Session::handle_header() {
  if (buffer_.size() < 9) {
    send_error_response(ERROR_SERVER);
    return;
  }

  unsigned char version = static_cast<unsigned char>(buffer_[0]);
  uint16_t command = read_uint_16_le(&buffer_[1]);
  uint32_t userId = read_uint_32_le(&buffer_[3]);
  uint16_t filename_len = read_uint_16_le(&buffer_[7]);

  if (version == 0 || command == 0 || userId == 0 || filename_len == 0) {
    send_error_response(ERROR_SERVER);
    return;
  }

  auto self(shared_from_this());
  buffer_.resize(filename_len + 4);
  boost::asio::async_read(
      socket_, buffer(buffer_),
      [this, self, version, command, userId,
       filename_len](boost::system::error_code error, uint16_t length) {
        if (!error && length == (filename_len + 4)) {
          string filename(buffer_.data(), filename_len);
          uint32_t payload_size = read_uint_32_le(&buffer_[filename_len]);

          handle_command(version, command, userId, filename, payload_size);
        } else {
          send_error_response(ERROR_SERVER);
        }
      });
}

void Session::handle_command(unsigned char version, uint16_t command,
                             uint32_t userId, const std::string &filename,
                             uint32_t payload_size) {
  using namespace status_code;

  switch (command) {
  case static_cast<uint8_t>(Command::SAVE):
    do_handle_save(version, userId, filename, payload_size);
    break;

  case static_cast<uint8_t>(Command::DELETE):
    do_handle_delete(version, userId, filename);
    break;

  case static_cast<uint8_t>(Command::LIST):
    do_handle_list(version, userId);

  case static_cast<uint8_t>(Command::RESTORE):
    do_handle_restore(version, userId, filename);
    break;
  default:
    send_error_response(ERROR_SERVER);
    break;
  }
}

void Session::do_handle_save(unsigned char version, uint32_t userId,
                             const string &filename, uint32_t payload_size) {
  if (payload_size == 0) {
    send_error_response(ERROR_SERVER);
    return;
  }

  auto self(shared_from_this());
  buffer_.resize(payload_size);

  async_read(socket_, buffer(buffer_),
             [this, self, version, userId,
              filename](boost::system::error_code error, size_t length) {
               if (!error && length == buffer_.size()) {
                 try {
                   create_user_dir(userId);
                   string fullPath = get_user_file_path(userId, filename);

                   ofstream out(fullPath, ios::binary);
                   if (!out) {
                     send_error_response(ERROR_SERVER);
                     return;
                   }

                   out.write(buffer_.data(), buffer_.size());
                   out.close();

                   send_simple_response(version, SUCCESS_OP);
                 } catch (...) {
                   send_error_response(ERROR_SERVER);
                 }

               } else {
                 send_error_response(ERROR_SERVER);
               }
             });
}

void Session::do_handle_delete(unsigned char version, uint32_t userId,
                               const string &filename) {
  string fullpath = get_user_file_path(userId, filename);

  if (!boost::filesystem::exists(fullpath)) {
    send_error_response(ERROR_NO_FILE);
    return;
  }
  try {
    boost::filesystem::remove(fullpath);
    send_simple_response(version, SUCCESS_OP);
  } catch (...) {
    send_error_response(ERROR_SERVER);
  }
}

void Session::do_handle_list(unsigned char version, uint32_t userId) {
  cout << "List request for user: " << userId << endl;
  string user_dir = get_user_dir(userId);

  if (!boost::filesystem::exists(user_dir)) {
    send_error_response(ERROR_NO_FILES);
    return;
  }

  vector<string> user_files;

  for (auto &entry : boost::filesystem::directory_iterator(user_dir)) {
    if (boost::filesystem::is_regular_file(entry.path())) {
      user_files.push_back(entry.path().filename().string());
    }
  }

  if (user_files.empty()) {
    send_error_response(ERROR_NO_FILES);
    return;
  }

  string file_list;
  for (const auto &fn : user_files) {
    file_list += fn + "\n";
  }

  vector<char> response;
  response.push_back(version);
  write_uint_16_le(response.data() + 1, LIST_RETURNED);
  write_uint_16_le(response.data() + 3,
                   static_cast<uint16_t>(file_list.size()));
  response.insert(response.end(), file_list.begin(), file_list.end());

  cout << "Sending file list response: " << file_list << endl;

  auto self(shared_from_this());
  async_write(socket_, buffer(response),
              [self](boost::system::error_code ec, size_t bytes_transferred) {
                if (!ec) {
                  cout << "File list response sent successfully." << endl;
                } else {
                  cout << "Error sending file list response: " << ec.message()
                       << endl;
                }
              });

  /*string list_filename = generate_random_filename() + ".txt";
  string list_filepath = user_dir + "\\" + list_filename;

  try {
    ofstream out(list_filename, ios::binary);
    for (const auto &fn : user_files) {
      out << fn << "\n";
    }

    out.close();
  } catch (...) {
    send_error_response(ERROR_SERVER);
    return;
  }*/

  // send_file_response(version, LIST_RETURNED, list_filepath, list_filename);
}

void Session::do_handle_restore(unsigned char version, uint32_t userId,
                                const string &filename) {
  string full_path = get_user_file_path(userId, filename);
  if (!filesystem::exists(full_path)) {
    send_error_response(ERROR_NO_FILE);
    return;
  }

  send_file_response(version, FILE_FOUND, full_path, filename);
}

void Session::send_simple_response(unsigned char version, uint16_t status) {
  vector<char> resp;
  resp.resize(5);
  resp[0] = static_cast<char>(version);
  write_uint_16_le(&resp[1], status);
  write_uint_16_le(&resp[3], 0);

  auto self(shared_from_this());
  async_write(socket_, buffer(resp),
              [this, self](boost::system::error_code, size_t) {
                // done, socket can close
              });
}

void Session::send_file_response(unsigned char version, uint16_t status,
                                 const string &file_path,
                                 const string &filename) {
  ifstream in(file_path, ios::binary | ios::ate);
  {
    if (!in) {
      send_error_response(ERROR_SERVER);
      return;
    }

    streamsize filesize = in.tellg();
    in.seekg(0, ios::beg);

    uint16_t name_length = static_cast<uint16_t>(filename.size());
    vector<char> header;
    header.resize(name_length + 9);

    header[0] = static_cast<char>(version);
    write_uint_16_le(&header[1], status);
    write_uint_16_le(&header[3], name_length);
    memcpy(&header[5], filename.data(), name_length);
    write_uint_32_le(&header[5 + name_length], static_cast<uint32_t>(filesize));

    vector<char> filedata(filesize);
    if (!in.read(filedata.data(), filesize)) {
      send_error_response(ERROR_SERVER);
      return;
    }

    vector<char> response;
    response.reserve(header.size() + filedata.size());
    response.insert(response.end(), header.begin(), header.end());
    response.insert(response.end(), filedata.begin(), filedata.end());

    auto self(shared_from_this());
    async_write(socket_, buffer(response),
                [this, self](boost::system::error_code, size_t) {
                  // done
                });
  }
}

void Session::send_error_response(uint16_t status) {
  vector<char> resp(5);
  resp[0] = 1;
  write_uint_16_le(&resp[1], status);
  write_uint_16_le(&resp[3], 0);

  auto self(shared_from_this());
  async_write(socket_, buffer(resp),
              [this, self](boost::system::error_code, size_t) {
                // done
              });
}

string Session::get_user_file_path(uint32_t userId, const string &filename) {
  return BACKUP_ROOT + to_string(userId) + "\\" + filename;
}

string Session::get_user_dir(uint32_t userId) {
  return BACKUP_ROOT + to_string(userId);
}

void Session::create_user_dir(uint32_t userId) {
  string dir_path = get_user_dir(userId);
  filesystem::create_directories(dir_path);
}