#include "chars.h"
#include "1c/componentbase.h"
#include "root.h"
#include "watcher.h"
#include "regex.h"
#if _WIN32
[[maybe_unused]]
BOOL APIENTRY DllMain ( [[maybe_unused]] HMODULE hModule,
						[[maybe_unused]] DWORD ul_reason_for_call,
						[[maybe_unused]] LPVOID lpReserved ) {
	return TRUE;
}
#endif
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

static const wchar_t ClassNames[] ( L"Root|Watcher|Regex" );
static AppCapabilities ApplicationCapabilities { AppCapabilitiesInvalid };
static WCHAR_T* Names {};

long GetClassObject ( const WCHAR_T* Name, IComponentBase** Interface ) {
	if ( *Interface ) {
		return 0;
	}
	auto name { Chars::WCHARToWide ( Name ) };
	if ( name == L"Root" ) {
		*Interface = new Root ();
	} else if ( name == L"Watcher" ) {
		try {
			*Interface = new Watcher ();
		} catch ( ... ) {
			return 0;
		}
	} else if ( name == L"Regex" ) {
		*Interface = new Regex ();
	}
	return reinterpret_cast<long>(*Interface);
}

AppCapabilities SetPlatformCapabilities ( const AppCapabilities Capabilities ) {
	ApplicationCapabilities = Capabilities;
	return AppCapabilitiesLast;
}

long DestroyObject ( IComponentBase** Interface ) {
	if ( !*Interface ) {
		return -1;
	}
	delete *Interface;
	*Interface = nullptr;
	return 0;
}

const WCHAR_T* GetClassNames () {
	if ( !Names ) {
		Chars::ToWCHAR ( &Names, ClassNames );
	}
	return Names;
}
#pragma clang diagnostic pop
