#include "files.h"
#include "transform.h"
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <xxhash.h>

namespace files {
std::string toString ( const std::string& file ) {
	std::ifstream stream ( file, std::ios::ate );
	if ( !stream ) {
		throw std::runtime_error ( "failed to open: " + file );
	}
	std::string content;
	content.resize ( stream.tellg () );
	stream.seekg ( 0 );
	stream.read ( content.data (), content.size () );
	return content;
}

uint64_t toHash ( const std::string& file ) {
	const auto string = strings::trim ( toString ( file ) );
	return XXH3_64bits ( string.data (), string.size () );
}
}
