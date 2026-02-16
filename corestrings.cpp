#include "corestrings.h"
#include <algorithm>
#include <cstdint>
#include <utf8.h>
#include <xxhash.h>

namespace strings {
bool starts ( const std::wstring& source, std::wstring_view prefix ) {
	auto sourceSize = source.size ();
	auto lastPosition = prefix.length ();
	if ( sourceSize < lastPosition ) {
		return false;
	}
	while ( lastPosition-- ) {
		if ( source [ lastPosition ] != prefix [ lastPosition ] ) {
			return false;
		}
	}
	return true;
}

std::wstring::size_type startPoint ( const std::wstring& source ) {
	auto index = source.size ();
	while ( index-- ) {
		auto symbol = source [ index ];
		if ( symbol == ' ' || symbol == '\t' || symbol == '\n' ) {
			continue;
		}
		return ++index;
	}
	return 0;
}

bool ends ( const std::wstring& source, std::wstring_view suffix,
						bool dropSpaces ) {
	auto sourceIndex = dropSpaces ? startPoint ( source ) : source.size ();
	if ( !sourceIndex ) {
		return false;
	}
	auto suffixIndex = suffix.length ();
	if ( sourceIndex < suffixIndex ) {
		return false;
	}
	while ( suffixIndex-- && sourceIndex-- ) {
		if ( source [ sourceIndex ] != suffix [ suffixIndex ] )
			return false;
	}
	return true;
}

std::wstring replace ( std::wstring string, const std::wstring& what,
											 const std::wstring& replace ) {
	if ( string.empty () || what.empty () ) {
		return string;
	}
	size_t pos = 0, replaceLength = replace.length (),
				 whatLength = what.length ();
	while ( ( pos = string.find ( what, pos ) ) != std::wstring::npos ) {
		string.replace ( pos, whatLength, replace );
		pos += replaceLength;
	}
	return string;
}

void replace ( std::wstring* string, const std::wstring& what,
							 const std::wstring& replace ) {
	if ( string->empty () || what.empty () ) {
		return;
	}
	size_t pos = 0, replaceLength = replace.length (),
				 whatLength = what.length ();
	while ( ( pos = string->find ( what, pos ) ) != std::wstring::npos ) {
		string->replace ( pos, whatLength, replace );
		pos += replaceLength;
	}
}

namespace {
bool isValidXML ( char c ) {
	const uint32_t code = static_cast<unsigned char> ( c );
	return (
			code == 0x09 ||													// Horizontal Tab
			code == 0x0A ||													// Line Feed
			code == 0x0D ||													// Carriage Return
			( code >= 0x20 && code <= 0xD7FF ) ||		// Unicode code points in range
			( code >= 0xE000 && code <= 0xFFFD ) || // Unicode code points in range
			( code >= 0x10000 && code <= 0x10FFFF ) // Unicode code points in range
	);
}
}

std::string fix ( const std::string& str ) {
	std::string result;
	result.reserve ( str.size () );
	utf8::replace_invalid ( str.begin (), str.end (), back_inserter ( result ) );
	return result;
}

std::string clean ( const std::string& input ) {
	std::string output;
	output.reserve ( input.size () );
	std::copy_if ( input.begin (), input.end (), std::back_inserter ( output ),
								 isValidXML );
	return fix ( output );
}

void replaceChars ( std::wstring& string, const std::wstring& list,
										const wchar_t replace = L' ' ) {
	std::replace_if (
			string.begin (), string.end (),
			[ &list ] ( wchar_t c ) { return list.find ( c ) != std::string::npos; },
			replace );
}
}
