#ifndef __json_h__
#define __json_h__
#include <utility>
#include <vector>
#include <iomanip>
#include <memory>

namespace JSON {
	const char Hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	std::wstring toHex ( const wchar_t Value ) {
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

	class Value {
	public:
		Value () = default;
		explicit Value ( std::wstring Name ) : Name ( std::move ( Name ) ) {};

		virtual void Presentation ( std::wstring* Result ) {
			if ( !Name.empty () ) {
				Result->push_back ( L'\"' );
				escape ( Result, Name );
				Result->append ( L"\":" );
			}
		}
	protected:
		std::wstring Name;
	};

	class [[maybe_unused]] String : public Value {
	public:
		using Value::Value;

		void Set ( const std::wstring& Value ) {
			Storage = Value;
		}

		void Presentation ( std::wstring* Result ) override {
			Value::Presentation ( Result );
			Result->push_back ( L'\"' );
			escape ( Result, Storage );
			Result->push_back ( L'\"' );
		}
	private:
		std::wstring Storage;
	};

	class [[maybe_unused]] Null : public Value {
	public:
		using Value::Value;

		void Presentation ( std::wstring* Result ) override {
			Value::Presentation ( Result );
			Result->append ( L"null" );
		}
	};

	class Container {
	public:
		template <class ValueType>
		ValueType* Add () {
			auto item = new ValueType ();
			Items.push_back ( std::shared_ptr<ValueType> ( item ) );
			return static_cast<ValueType*>(item);
		}

		template <class ValueType>
		[[maybe_unused]]
		ValueType* Add ( const std::wstring& Name ) {
			auto item = new ValueType ( Name );
			Items.push_back ( std::shared_ptr<ValueType> ( item ) );
			return static_cast<ValueType*>(item);
		}

		void ItemsPresentation ( std::wstring* Result ) {
			bool next { false };
			for ( const auto& item : Items ) {
				if ( next ) {
					Result->push_back ( L',' );
				}
				item->Presentation ( Result );
				next = true;
			}
		}

	private:
		std::vector<std::shared_ptr<Value>> Items;
	};

	class [[maybe_unused]] Object : public Value, public Container {
	public:
		using Value::Value;
		using Container::Container;

		void Presentation ( std::wstring* Result ) override {
			Value::Presentation ( Result );
			Result->push_back ( L'{' );
			ItemsPresentation ( Result );
			Result->push_back ( L'}' );
		}
	};

	class [[maybe_unused]] Array : public Value, public Container {
	public:
		using Value::Value;
		using Container::Container;

		void Presentation ( std::wstring* Result ) override {
			Value::Presentation ( Result );
			Result->push_back ( L'[' );
			ItemsPresentation ( Result );
			Result->push_back ( L']' );
		}
	};
}
#endif