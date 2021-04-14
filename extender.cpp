#include "extender.h"
#include <chrono>
#include <mutex>
#ifdef __linux__
#include <cwchar>
#include <utility>
#elif _WIN32
#include <clocale>
#endif

Extender::Extender ( const std::wstring& Extension )
		: memoryManager ( nullptr ), baseConnector ( nullptr ) {
	ExtensionID = nullptr;
	Chars::ToWCHAR ( &ExtensionID, Extension.data () );
	methods.AddFunction ( L"Version", L"Версия", 0, [ & ] ( tVariant* Params, tVariant* Result ) {
		version ( Result );
		return true;
	} );
	methods.AddFunction ( L"Problem", L"Проблема", 0, [ & ] ( tVariant* Params, tVariant* Result ) {
		getLastError ( Result );
		return true;
	} );
	methods.AddFunction ( L"Error", L"Ошибка", 0, [ & ] ( tVariant* Params, tVariant* Result ) {
		isError ( Result );
		return true;
	} );
}

Extender::~Extender () {
	delete[] ExtensionID;
}

double Extender::getNumber ( tVariant* Params ) {
	switch ( Params->vt ) {
		case VTYPE_I2:
			return Params->shortVal;
		case VTYPE_I4:
			return Params->lVal;
		case VTYPE_R4:
			return Params->fltVal;
		case VTYPE_R8:
			return Params->dblVal;
		case VTYPE_ERROR:
			return Params->errCode;
		case VTYPE_BOOL:
			return Params->bVal;
		case VTYPE_I1:
			return Params->i8Val;
		case VTYPE_UI1:
			return Params->ui8Val;
		case VTYPE_UI2:
			return Params->ushortVal;
		case VTYPE_UI4:
			return Params->ulVal;
		case VTYPE_I8:
			return Params->llVal;
		case VTYPE_UI8:
			return Params->ullVal;
		case VTYPE_INT:
			return Params->intVal;
		case VTYPE_UINT:
			return Params->uintVal;
		default:
			return 0;
	}
}

bool Extender::Init ( void* Connection ) {
	baseConnector = static_cast<IAddInDefBase*>( Connection );
	baseConnector->SetEventBufferDepth ( YellowBuffer );
	return baseConnector != nullptr;
}

bool Extender::setMemManager ( void* Pointer ) {
	memoryManager = static_cast<imemorymanager*>( Pointer );
	return memoryManager != nullptr;
}

long Extender::GetInfo () {
	return 9045;
}

bool Extender::RegisterExtensionAs ( WCHAR_T** Entry ) {
	return allocate ( ExtensionID, Entry );
}

bool Extender::allocate ( const WCHAR_T* Source, WCHAR_T** Destination ) const {
	if ( !memoryManager ) {
		return false;
	}
	auto size = Chars::WCHARLength ( Source ) + 1;
	if ( !memoryManager->AllocMemory ( reinterpret_cast<void**>( Destination ), size * sizeof ( WCHAR_T ) ) ) {
		return false;
	}
	auto to = *Destination;
	while ( size-- ) *to++ = *Source++;
	return true;
}

long Extender::GetNProps () {
	return properties.Count;
}

long Extender::FindProp ( const WCHAR_T* Name ) {
	return properties.GetIndex ( Chars::FromWCHAR ( Name ).get () );
}

const WCHAR_T* Extender::GetPropName ( long Property, long Lang ) {
	return getName ( properties.PropertyKeys, properties.PropertyKeysRu, Property, Lang );
}

bool Extender::GetPropVal ( long Property, tVariant* Value ) {
	return false;
}

bool Extender::SetPropVal ( long Property, tVariant* Value ) {
	return false;
}

bool Extender::IsPropReadable ( long Property ) {
	return false;
}

bool Extender::IsPropWritable ( long Property ) {
	return false;
}

long Extender::GetNMethods () {
	return methods.Count;
}

long Extender::FindMethod ( const WCHAR_T* Name ) {
	return getIndex ( methods.Methods, methods.MethodsRu, Chars::FromWCHAR ( Name ).get () );
}

const WCHAR_T* Extender::GetMethodName ( long Method, long Lang ) {
	return getName ( methods.MethodKeys, methods.MethodKeysRu, Method, Lang );
}

long Extender::GetNParams ( long Method ) {
	return methods.Parameters[ Method ];
}

bool Extender::GetParamDefValue ( long Method, long Parameter, tVariant* Default ) {
	TV_VT ( Default ) = VTYPE_EMPTY;
	return false;
}

bool Extender::HasRetVal ( long Method ) {
	return methods.HasResult[ Method ];
}

bool Extender::CallAsProc ( long Method, tVariant* Params, long Count ) {
	if ( !checkParams ( Method, Count ) ) return false;
	return methods.Procedures[ Method ] ( Params );
}

bool Extender::checkParams ( long Method, long Count ) {
	return Count == methods.Parameters[ Method ];
}

bool Extender::CallAsFunc ( long Method, tVariant* Result, tVariant* Params, long Count ) {
	if ( !checkParams ( Method, Count ) ) return false;
	return methods.Functions[ Method ] ( Params, Result );
}

void Extender::SetLocale ( const WCHAR_T* Locale ) {
#if !defined( __linux__ ) && !defined(__APPLE__)
	_wsetlocale ( LC_ALL, Locale );
#else
	//We convert in char* char_locale
	//also we establish locale
	//setlocale(LC_ALL, char_locale);
#endif
}

void Extender::Done () {}

void Extender::ShowError ( const char* Message ) const {
	auto size = mbstowcs ( nullptr, Message, 0 ) + 1;
	assert ( size );
	auto message = new WCHAR[size];
	memset ( message, 0, size * sizeof ( wchar_t ) );
	mbstowcs ( message, Message, size );
	addError ( ADDIN_E_VERY_IMPORTANT, message, 0 );
	delete[] message;
}

[[maybe_unused]]
void Extender::propertiesList::Add ( const std::wstring& English, const std::wstring& Russian ) {
	++Count;
	Properties[ English ] = Count;
	PropertiesRu[ Russian ] = Count;
	PropertyKeys[ Count ] = English;
	PropertyKeysRu[ Count ] = Russian;
}

long Extender::propertiesList::GetIndex ( const wchar_t* Name ) const {
	auto value = Properties.find ( Name );
	if ( value == Properties.end () ) value = PropertiesRu.find ( Name );
	else return value->second;
	if ( value == PropertiesRu.end () ) return -1;
	return value->second;
}

void
Extender::methodsList::AddProcedure ( const wchar_t* English, const wchar_t* Russian, int Params, procedure Handler ) {
	addMethod ( English, Russian, Params, false );
	Procedures[ Count ] = std::move ( Handler );
}

void Extender::methodsList::addMethod ( const wchar_t* English, const wchar_t* Russian, int Params, bool Function ) {
	++Count;
	Methods[ English ] = Count;
	MethodsRu[ Russian ] = Count;
	MethodKeys[ Count ] = English;
	MethodKeysRu[ Count ] = Russian;
	HasResult[ Count ] = Function;
	Parameters[ Count ] = Params;
}

void Extender::methodsList::AddFunction ( const wchar_t* English, const wchar_t* Russian, int Params,
										  function Handler ) {
	addMethod ( English, Russian, Params, true );
	Functions[ Count ] = std::move ( Handler );
}

[[maybe_unused]]
long Extender::findName ( const wchar_t* Names[], const wchar_t* Name, uint32_t Size ) {
	long ret = -1;
	for ( uint32_t i = 0; i < Size; i++ ) {
		if ( !wcscmp ( Names[ i ], Name ) ) {
			ret = i;
			break;
		}
	}
	return ret;
}

void Extender::addError ( uint32_t Code, const wchar_t* Descriptor, long Id ) const {
	if ( baseConnector ) {
		baseConnector->AddError ( Code, ExtensionID, Chars::ToWCHAR ( Descriptor ).get (), Id );
	}
}

const WCHAR_T*
Extender::getName ( std::map<long, std::wstring>& SetEn, std::map<long, std::wstring>& SetRu, long Index,
					long Lang ) const {
	const wchar_t* name;
	switch ( Lang ) {
		case 0:
			name = SetEn[ Index ].c_str ();
			break;
		case 1:
			name = SetRu[ Index ].c_str ();
			break;
		default:
			return nullptr;
	}
	if ( name[ 0 ] == '\0' ) {
		return nullptr;
	}
	WCHAR_T* yellowName { nullptr };
	if ( allocate ( Chars::ToWCHAR ( name ).get (), &yellowName ) ) {
		return yellowName;
	} else {
		return nullptr;
	}
}

long
Extender::getIndex ( std::map<std::wstring, int>& SetEn, std::map<std::wstring, int>& SetRu, const wchar_t* Name ) {
	auto value = SetEn.find ( Name );
	if ( value == SetEn.end () ) value = SetRu.find ( Name );
	else return value->second;
	if ( value == SetRu.end () ) return -1;
	return value->second;
}

void Extender::returnString ( tVariant* Result, const std::wstring& String ) const {
	auto count = 1 + String.size ();
	auto size = count * sizeof std::wstring::value_type ();
	if ( !size || !memoryManager->AllocMemory ( reinterpret_cast<void**>( &Result->pwstrVal ), size ) ) return;
#ifdef __linux__
	Chars::ToWCHAR ( &Result->pwstrVal, String.data () );
#elif _WIN32
	if ( wcscpy_s ( Result->pwstrVal, count, String.c_str () ) ) return;
#endif
	Result->vt = VTYPE_PWSTR;
	Result->wstrLen = count - 1;
}

void Extender::returnBool ( tVariant* Result, bool Value ) {
	Result->vt = VTYPE_BOOL;
	Result->bVal = Value;
}

void Extender::version ( tVariant* Result ) {
	Result->llVal = GetInfo ();
	Result->vt = VTYPE_I4;
}

void Extender::getLastError ( tVariant* Result ) {
	if ( !LastError.empty () ) {
		returnString ( Result, LastError );
		LastError.clear ();
	}
}

void Extender::isError ( tVariant* Result ) {
	returnBool ( Result, !LastError.empty () );
}
