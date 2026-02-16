#pragma once
#include <cstdint>
#include <string>

namespace strings {
uint64_t toHash ( const std::string& string );
void trim ( std::string& string );
[[nodiscard]] std::string trim ( std::string&& string );
}
