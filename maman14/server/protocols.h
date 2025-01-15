#pragma once

#include <cstdint>
#include <string>

enum class Command : uint8_t {
  SAVE = 100,
  DELETE = 201,
  LIST = 202,
  RESTORE = 200
};

namespace status_code {
constexpr uint16_t FILE_FOUND = 210;
constexpr uint16_t LIST_RETURNED = 211;
constexpr uint16_t SUCCESS_OP = 212;

constexpr uint16_t ERROR_NO_FILE = 1001;
constexpr uint16_t ERROR_NO_FILES = 1002;
constexpr uint16_t ERROR_SERVER = 1003;
} // namespace status_code

uint16_t read_uint_16_le(const char *data);
uint32_t read_uint_32_le(const char *data);

void write_uint_16_le(char *data, uint16_t value);
void write_uint_32_le(char *data, uint32_t value);

std::string generate_random_filename();