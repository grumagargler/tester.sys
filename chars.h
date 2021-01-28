#ifndef __chars_h__
#define __chars_h__
#include <cstdint>
#include "1c/types.h"
#include <string>
#include <memory>

class Chars {
public:
	template <typename SourceType>
	static uint32_t ToWCHAR ( WCHAR_T** Destination, const SourceType* Source ) {
		size_t size;
		if constexpr ( std::is_same_v<SourceType, wchar_t> ) {
			size = wcslen ( Source ) + 1;
		} else {
			size = strlen ( Source ) + 1;
		}
		if ( !*Destination ) {
			*Destination = new WCHAR_T[size];
		}
		uint32_t converted { 0 };
		memset ( *Destination, 0, size * sizeof ( WCHAR_T ) );
		auto receiver = *Destination;
		while ( --size && *Source ) {
			*receiver++ = *Source++;
		}
		return converted;
	}

	template <typename SourceType>
	static std::unique_ptr<WCHAR_T[]> ToWCHAR ( const SourceType* Source ) {
		size_t size;
		if constexpr ( std::is_same_v<SourceType, wchar_t> ) {
			size = wcslen ( Source ) + 1;
		} else {
			size = strlen ( Source ) + 1;
		}
		std::unique_ptr<WCHAR_T[]> result { new WCHAR_T[size] };
		auto receiver = result.get ();
		memset ( receiver, 0, size * sizeof ( WCHAR_T ) );
		while ( --size && *Source ) {
			*receiver++ = *Source++;
		}
		return result;
	}

	static std::unique_ptr<wchar_t[]> FromWCHAR ( const WCHAR_T* Source, size_t Length = 0 );
	static std::wstring StringToWide ( const std::string& Source );
	static std::string WideToString ( const std::wstring& Source );
	static std::wstring WCHARToWide ( const WCHAR_T* String, size_t Length = 0 );
	static uint32_t WCHARLength ( const WCHAR_T* Source );
};
#endif