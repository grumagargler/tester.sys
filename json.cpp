#include "json.h"
#include <utility>
#include <memory>

namespace JSON {
	std::wstring toHex ( wchar_t Value ) {
		std::wstring result;
		result += Hex[ ( Value & 0xF000 ) >> 12 ];
		result += Hex[ ( Value & 0xF00 ) >> 8 ];
		result += Hex[ ( Value & 0xF0 ) >> 4 ];
		result += Hex[ ( Value & 0x0F ) >> 0 ];
		return result;
	}

	void escape ( std::wstring* Result, const std::wstring& s ) {
		for ( wchar_t c : s ) {
			switch ( c ) {
				case '"':
					Result->append ( L"\\\"" );
					break;
				case '\\':
					Result->append ( L"\\\\" );
					break;
				case '\b':
					Result->append ( L"\\b" );
					break;
				case '\f':
					Result->append ( L"\\f" );
					break;
				case '\n':
					Result->append ( L"\\n" );
					break;
				case '\r':
					Result->append ( L"\\r" );
					break;
				case '\t':
					Result->append ( L"\\t" );
					break;
				default:
					if ( '\x00' <= c && c <= '\x1f' ) {
						Result->append ( L"\\u" + toHex ( c ) );
					} else {
						Result->push_back ( c );
					}
			}
		}
	}

	Value::Value ( std::wstring Name ) : Name ( std::move ( Name ) ) {}

	void Value::Presentation ( std::wstring* Result ) {
		if ( !Name.empty () ) {
			Result->push_back ( L'\"' );
			escape ( Result, Name );
			Result->append ( L"\":" );
		}
	}

	void String::Set ( const std::wstring& Value ) {
		Storage = Value;
	}

	void String::Presentation ( std::wstring* Result ) {
		Value::Presentation ( Result );
		Result->push_back ( L'\"' );
		escape ( Result, Storage );
		Result->push_back ( L'\"' );
	}

	[[maybe_unused]] void Number::Set ( int Value ) {
		Storage = std::to_wstring ( Value );
	}

	void Number::Presentation ( std::wstring* Result ) {
		Value::Presentation ( Result );
		escape ( Result, Storage
		);
	}

	void Null::Presentation ( std::wstring* Result ) {
		Value::Presentation ( Result );
		Result->append ( L"null" );
	}

	void Container::ItemsPresentation ( std::wstring* Result ) {
		bool next { false };
		for ( const auto& item : Items ) {
			if ( next ) {
				Result->push_back ( L',' );
			}
			item->Presentation ( Result );
			next = true;
		}
	}

	void Object::Presentation ( std::wstring* Result ) {
		Value::Presentation ( Result );
		Result->push_back ( L'{' );
		ItemsPresentation ( Result );
		Result->push_back ( L'}' );
	}

	void Array::Presentation ( std::wstring* Result ) {
		Value::Presentation ( Result );
		Result->push_back ( L'[' );
		ItemsPresentation ( Result );
		Result->push_back ( L']' );
	}
}