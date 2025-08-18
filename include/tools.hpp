#pragma once

#include <cstdint>
#include <string>
#include <vector>

void int_to_bytes(int32_t value, unsigned char* bytes);
std::vector<unsigned char> string_to_bytes(std::string &str);
