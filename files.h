#pragma once
#include <cstdint>
#include <string>

namespace files {
std::string toString ( const std::string& file );
uint64_t toHash ( const std::string& file );
}
