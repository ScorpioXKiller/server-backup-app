#pragma once

#include <boost/asio.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace std;

class Session : public std::enable_shared_from_this<Session> {
public:
  explicit Session(boost::asio::ip::tcp::socket socket);
  void start();

private:
  void do_read_header();
  void do_write_response(const string &response);
  void handle_header();

  void handle_command(unsigned char version, uint16_t command, uint32_t userId,
                      const string &filename, uint32_t payload_size);
  void do_handle_save(unsigned char version, uint32_t userId,
                      const string &filename, uint32_t payload_size);
  void do_handle_delete(unsigned char version, uint32_t userId,
                        const string &filename);
  void do_handle_list(unsigned char version, uint32_t userId);
  void do_handle_restore(unsigned char version, uint32_t userId,
                         const string &filename);
  void send_simple_response(unsigned char version, uint16_t status_code);
  void send_file_response(unsigned char version, uint16_t status_code,
                          const string &filepath, const string &filename);
  void send_error_response(uint16_t status_code);

  string get_user_file_path(uint32_t userId, const string &filename);
  string get_user_dir(uint32_t userId);

  void create_user_dir(uint32_t userId);

private:
  boost::asio::ip::tcp::socket socket_;
  std::vector<char> buffer_;
};