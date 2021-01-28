#include "chars.h"
#include <string>
#include <locale>
#include <codecvt>

uint32_t Chars::WCHARLength ( const WCHAR_T* Source ) {
	uint32_t length = 0;
	auto tmpShort = const_cast<WCHAR_T*>( Source );
	while ( *tmpShort++ ) ++length;
	return length;
}

std::unique_ptr<wchar_t[]> Chars::FromWCHAR ( const WCHAR_T* Source, size_t Length ) {
	if ( !Length ) {
		Length = WCHARLength ( Source );
	}
	++Length;
	std::unique_ptr<wchar_t[]> result { new wchar_t[Length] };
	auto receiver = result.get ();
	memset ( receiver, 0, Length * sizeof ( wchar_t ) );
	while ( --Length && *Source ) {
		*receiver++ = *Source++;
	}
	return result;
}

std::wstring Chars::StringToWide ( const std::string& Source ) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes ( Source );
}

std::string Chars::WideToString ( const std::wstring& Source ) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	return converter.to_bytes ( Source );
}

std::wstring Chars::WCHARToWide ( const WCHAR_T* String, size_t Length ) {
	if ( String == nullptr ) {
		return {};
	}
#ifdef __linux__
	return std::wstring { FromWCHAR ( String, Length ).get () };
#elif _WIN32
	if ( Length ) {
		return { String, Length };
	} else {
		return String;
	}
#endif
}
