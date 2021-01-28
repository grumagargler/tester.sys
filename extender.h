#ifndef __addin_h__
#define __addin_h__
#include <functional>
#include <map>
#include "1c/componentbase.h"
#include "1c/addindefbase.h"
#include "1c/imemorymanager.h"
#include "chars.h"
#define BASE_ERRNO 7

class Extender : public IComponentBase {
public:
	typedef std::function<void ( tVariant* Params )> procedure;
	typedef std::function<void ()> method;
	typedef std::function<void ( tVariant* Params, tVariant* Result )> function;

	explicit Extender ( const std::wstring& Extension );
	~Extender () override;
	bool ADDIN_API Init ( void* Connection ) override;
	bool ADDIN_API setMemManager ( void* Pointer ) override;
	long ADDIN_API GetInfo () override;
	bool ADDIN_API RegisterExtensionAs ( WCHAR_T** Entry ) override;
	bool allocate ( const WCHAR_T* Source, WCHAR_T** Destination ) const;
	long ADDIN_API GetNProps () override;
	long ADDIN_API FindProp ( const WCHAR_T* Name ) override;
	const WCHAR_T* ADDIN_API GetPropName ( long Property, long Lang ) override;
	bool ADDIN_API GetPropVal ( long Property, tVariant* Value ) override;
	bool ADDIN_API SetPropVal ( long Property, tVariant* Value ) override;
	bool ADDIN_API IsPropReadable ( long Property ) override;
	bool ADDIN_API IsPropWritable ( long Property ) override;
	long ADDIN_API GetNMethods () override;
	long ADDIN_API FindMethod ( const WCHAR_T* Name ) override;
	const WCHAR_T* ADDIN_API GetMethodName ( long Method, long Lang ) override;
	long ADDIN_API GetNParams ( long Method ) override;
	bool ADDIN_API GetParamDefValue ( long Method, long Parameter, tVariant* Default ) override;
	bool ADDIN_API HasRetVal ( long Method ) override;
	bool ADDIN_API CallAsProc ( long Method, tVariant* Params, long Count ) override;
	bool checkParams ( long Method, long Count );
	bool ADDIN_API CallAsFunc ( long Method, tVariant* Result, tVariant* Params, long Count ) override;
	void ADDIN_API SetLocale ( const WCHAR_T* Locale ) override;
	void ADDIN_API Done () override;
	void ShowError ( const char* Message ) const;
	template <typename T>
	void SetError ( const T& Message ) {
		if constexpr ( std::is_same_v<T, std::wstring> ) {
			LastError = Message;
		} else if constexpr ( std::is_same_v<T, const char*> ) {
			LastError = Chars::StringToWide ( std::string ( Message ) );
		} else {
			LastError = Chars::StringToWide ( Message );
		}
	}
	template <typename T>
	void SendError ( const T& Message ) {
		std::wstring text;
		if constexpr ( std::is_same_v<T, std::wstring> ) {
			text = Message;
		} else if constexpr ( std::is_same_v<T, const char*> ) {
			text = Chars::StringToWide ( std::string ( Message ) );
		} else {
			text = Chars::StringToWide ( Message );
		}
		baseConnector->ExternalEvent ( ExtensionID, ErrorSignature, Chars::ToWCHAR ( text.data () ).get () );
	}
protected:
	struct propertiesList {
		int Count { 0 };
		std::map<std::wstring, long> Properties;
		std::map<std::wstring, long> PropertiesRu;
		std::map<long, std::wstring> PropertyKeys;
		std::map<long, std::wstring> PropertyKeysRu;

		[[maybe_unused]] void Add ( const std::wstring& English, const std::wstring& Russian );
		long GetIndex ( const wchar_t* Name ) const;
	};

	struct methodsList {
		int Count { 0 };
		std::map<std::wstring, int> Methods;
		std::map<std::wstring, int> MethodsRu;
		std::map<long, std::wstring> MethodKeys;
		std::map<long, std::wstring> MethodKeysRu;
		std::map<long, bool> HasResult;
		std::map<long, int> Parameters;
		std::map<long, procedure> Procedures;
		std::map<long, function> Functions;

		void AddProcedure ( const wchar_t* English, const wchar_t* Russian, int Parameters, procedure Handler );
		void addMethod ( const wchar_t* English, const wchar_t* Russian, int Params, bool Function );
		void AddFunction ( const wchar_t* English, const wchar_t* Russian, int Parameters,
						   std::function<void ( _tVariant*, _tVariant* )> Handler );
	};

	WCHAR_T* ExtensionID;
	const long YellowBuffer { 32758 };
	propertiesList properties;
	methodsList methods;
	IAddInDefBase* baseConnector;
	imemorymanager* memoryManager;

	[[maybe_unused]] static long findName ( const wchar_t* Names[], const wchar_t* Name, uint32_t Size );
	void addError ( uint32_t Code, const wchar_t* Descriptor, long Id ) const;
	const WCHAR_T*
	getName ( std::map<long, std::wstring>& SetEn, std::map<long, std::wstring>& SetRu, long Index, long Lang ) const;
	static long
	getIndex ( std::map<std::wstring, int>& SetEn, std::map<std::wstring, int>& SetRu, const wchar_t* Name );
	void returnString ( tVariant* Result, const std::wstring& String ) const;
	static void returnBool ( tVariant* Result, bool Value );
	static double getNumber ( tVariant* Params );
private:
	static constexpr WCHAR_T ErrorSignature[] = { '#', '#', '#', 'E', '#', '#', '#', '\0' };
	std::wstring LastError;

	void version ( tVariant* Result );
	void isError ( tVariant* Result );
	void getLastError ( tVariant* Result );
};
#endif