#include <cmath>
#include "root.h"
#if __linux__
#include <unistd.h>
#elif _WIN32
#include <sstream>
#include <cctype>
#endif

Root::Root () : Extender ( L"Root" ) {
	methods.AddFunction ( L"Shoot", L"Снять", 2, [ & ] ( tVariant* Params, tVariant* Result ) {
		return shoot ( Params, Result );
	} );
	methods.AddProcedure ( L"Maximize", L"Максимизировать", 1, [ & ] ( tVariant* Params ) {
		return maximize ( Params );
	} );
	methods.AddProcedure ( L"Minimize", L"Минимизировать", 1, [ & ] ( tVariant* Params ) {
		return minimize ( Params );
	} );
	methods.AddProcedure ( L"Pause", L"Пауза", 1, [ & ] ( tVariant* Params ) {
		pause ( Params );
		return true;
	} );
	methods.AddProcedure ( L"GotoConsole", L"ПерейтиВКонсоль", 0, [ & ] ( tVariant* Params ) {
		gotoConsole ( Params );
		return true;
	} );
	methods.AddFunction ( L"GetEnv", L"ПеременнаяСреды", 1, [ & ] ( tVariant* Params, tVariant* Result ) {
		getEnvironment ( Params, Result );
		return true;
	} );
}

bool Root::shoot ( tVariant* Params, tVariant* Result ) {
#if __linux__
	Shooter screenshot;
	std::optional<Shooter::RawBuffer> result;
	try {
		result = screenshot.Take ( Chars::WCHARToWide ( Params->pwstrVal ) );
	}
	catch ( std::regex_error& error ) {
		ShowError ( error.what () );
		return false;
	}
	catch ( ... ) {
		// There is no reason to notify client about internal stuff
		return true;
	}
	if ( result != std::nullopt ) {
		getPicture ( result.value (), Result );
	}
#elif _WIN32
	Shooter screenshot;
	std::optional<IStream*> result;
	try {
		result = screenshot.Window ( Params->pwstrVal, ( Params + 1 )->bVal );
	}
	catch ( std::regex_error& error ) {
		ShowError ( error.what () );
		return false;
	}
	catch ( ... ) {
		ShowError ( "Method screenshot.Window caused an internal error" );
		return false;
	}
	if ( result ) {
		getPicture ( result.value (), Result );
	}
#endif
	return true;
}

#if __linux__
void Root::getPicture ( Shooter::RawBuffer& Buffer, tVariant* Result ) const {
	auto size = Buffer.Size;
	auto source = Buffer.Buffer;
	if ( memoryManager->AllocMemory ( reinterpret_cast<void**>( &Result->pstrVal ), size ) ) {
		auto destination = Result->pstrVal;
		Result->strLen = size;
		Result->vt = VTYPE_BLOB;
		while ( size-- ) {
			*destination++ = *source++;
		}
	}
}
#elif _WIN32
void Root::getPicture ( IStream* Image, tVariant* Result ) const {
	STATSTG info;
	auto result = Image->Stat ( &info, STATFLAG_NONAME );
	if ( result != S_OK ) return;
	auto& size = info.cbSize.QuadPart;
	if ( size && memoryManager->AllocMemory ( reinterpret_cast<void**>( &Result->pstrVal ), size ) ) {
		ULONG copied;
		result = Image->Read ( Result->pstrVal, size, &copied );
		if ( result == S_OK ) {
			Result->strLen = copied;
			Result->vt = VTYPE_BLOB;
		}
	}
	Image->Release ();
}
#endif

bool Root::maximize ( tVariant* Params ) {
	auto title = Chars::WCHARToWide ( Params->pwstrVal );
#ifdef __linux__
	Shooter processor;
#elif _WIN32
	Expander processor;
#endif
	try {
		processor.Maximize ( title.data () );
	}
	catch ( std::regex_error& error ) {
		ShowError ( error.what () );
		return false;
	}
	catch ( ... ) {
		ShowError ( "Method maximize caused an internal error" );
		return false;
	}
	return true;
}

bool Root::minimize ( tVariant* Params ) {
	auto title = Chars::WCHARToWide ( Params->pwstrVal );
#ifdef __linux__
	Shooter processor;
#elif _WIN32
	Expander processor;
#endif
	try {
		processor.Minimize ( title.data () );
	}
	catch ( std::regex_error& error ) {
		ShowError ( error.what () );
		return false;
	}
	catch ( ... ) {
		ShowError ( "Method minimize caused an internal error" );
		return false;
	}
	return true;
}

void Root::pause ( tVariant* Params ) {
	if ( Params->vt == VTYPE_EMPTY ) return;
	auto seconds { getNumber ( Params ) };
#if __linux__
	sleep ( seconds );
#elif _WIN32
	Sleep ( 1000 * seconds );
#endif
}

void Root::getEnvironment ( tVariant* Params, tVariant* Result ) {
	std::wstring result;
	std::wstring variable { Chars::WCHARToWide ( Params->pwstrVal, Params->wstrLen ) };
#if __linux__
	auto value = std::getenv ( Chars::WideToString ( variable ).data () );
	if ( value ) {
		result = Chars::StringToWide ( value );
	}
#elif _WIN32
	auto variableName = variable.data ();
	size_t requiredSize;
	_wgetenv_s ( &requiredSize, nullptr, 0, variableName );
	if ( requiredSize > 0 ) {
		auto value = static_cast<wchar_t*>( malloc ( requiredSize * sizeof ( wchar_t ) ) );
		if ( value ) {
			_wgetenv_s ( &requiredSize, value, requiredSize, variableName );
			result = value;
			free ( value );
		}
	}
#endif
	returnString ( Result, result );
}

void Root::gotoConsole ( [[maybe_unused]] tVariant* Params ) {
#if _WIN32
	DWORD session;
	auto process = GetCurrentProcessId ();
	if ( !ProcessIdToSessionId ( process, &session ) ) {
		return;
	}
	std::ostringstream stream;
	stream << session;
	std::string cmd = "tscon " + stream.str () + " /dest:console";
#if _WIN64
	system ( cmd.data () );
#else
	system ( ( std::string ( "%windir%\\sysnative\\" ) + cmd ).data () );
#endif
#endif
}
