#include "transform.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <xxhash.h>

namespace strings {
uint64_t toHash ( const std::string& string ) {
	return XXH3_64bits ( string.data (), string.size () );
}

void trim ( std::string& string ) {
	if ( string.size () >= 3 &&
			 static_cast<unsigned char> ( string [ 0 ] ) == 0xEF &&
			 static_cast<unsigned char> ( string [ 1 ] ) == 0xBB &&
			 static_cast<unsigned char> ( string [ 2 ] ) == 0xBF ) {
		string.erase ( 0, 3 );
	}
	const auto begin = string.begin ();
	string.erase (
			begin, std::find_if_not ( begin, string.end (), [] ( unsigned char c ) {
				return isspace ( c );
			} ) );
	string.erase (
			std::find_if_not ( string.rbegin (), string.rend (),
												 [] ( unsigned char c ) { return isspace ( c ); } )
					.base (),
			string.end () );
}

std::string trim ( std::string&& string ) {
	trim ( string );
	return std::move ( string );
}
}
