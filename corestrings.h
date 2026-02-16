#pragma once
#include <cctype>
#include <cstdint>
#include <string>

namespace strings {
bool starts ( const std::wstring& source, std::wstring_view prefix );

bool ends ( const std::wstring& source, std::wstring_view suffix,
						bool dropSpaces = true );

std::wstring replace ( std::wstring string, const std::wstring& what,
											 const std::wstring& replace );

void replace ( std::wstring* string, const std::wstring& what,
							 const std::wstring& replace );

std::string clean ( const std::string& input );

std::wstring replaceAccents ( const std::wstring& input );

void replaceChars ( std::wstring& string, const std::wstring& list,
										const wchar_t replace );

inline bool isLetter ( const char symbol ) {
	return ( symbol >= 'A' && symbol <= 'Z' ) ||
				 ( symbol >= 'a' && symbol <= 'z' ) || symbol == '_';
}

inline bool isBlank ( const char symbol ) {
	return std::isspace ( symbol );
}
}

