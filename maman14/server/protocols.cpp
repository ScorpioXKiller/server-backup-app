#include "protocols.h"

#include <cstdint>
#include <random>
#include <string>

using namespace std;

uint16_t read_uint_16_le(const char *data) {
  return static_cast<uint16_t>((static_cast<unsigned char>(data[1]) << 8) |
                               (static_cast<unsigned char>(data[0])));
}

uint32_t read_uint_32_le(const char *data) {
  return (static_cast<uint32_t>(static_cast<unsigned char>(data[0]))) |
         ((static_cast<uint32_t>(static_cast<unsigned char>(data[1])) << 8)) |
         ((static_cast<uint32_t>(static_cast<unsigned char>(data[2])) << 16)) |
         ((static_cast<uint32_t>(static_cast<unsigned char>(data[3])) << 24));
}

void write_uint_16_le(char *data, uint16_t value) {
  data[0] = static_cast<char>(value & 0xFF);
  data[1] = static_cast<char>((value >> 8) & 0xFF);
}

void write_uint_32_le(char *data, uint32_t value) {
  data[0] = static_cast<char>(value & 0xFF);
  data[1] = static_cast<char>((value >> 8) & 0xFF);
  data[2] = static_cast<char>((value >> 16) & 0xFF);
  data[3] = static_cast<char>((value >> 24) & 0xFF);
}

string generate_random_filename() {
  static const char chars[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  static mt19937 rng(random_device{}());
  static uniform_int_distribution<std::size_t> dist(0, sizeof(chars) - 2);

  string result;
  result.reserve(32);

  for (int i = 0; i < 32; ++i) {
    result.push_back(chars[dist(rng)]);
  }

  return result;
}