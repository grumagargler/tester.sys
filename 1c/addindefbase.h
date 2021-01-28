#ifndef __addindefbase_h__
#define __addindefbase_h__

/*
 *          Warning!!!
 *       DO NOT ALTER THIS FILE!
 */

#include "types.h"

struct IInterface {
};

enum Interfaces {
	eIMsgBox = 0,
	eIPlatformInfo,

#if defined(__ANDROID__)

	eIAndroidComponentHelper,

#endif

};

////////////////////////////////////////////////////////////////////////////////
/**
 * This class serves as representation of a platform for external 
 * components External components use it to communicate with a platform.
 *
 */
/// Base interface for object components.
class IAddInDefBase {
public:
	virtual ~IAddInDefBase () {}
	/// Adds the error message
	/**
	 *  @param wcode - error code
	 *  @param source - source of error
	 *  @param descr - description of error
	 *  @param scode - error code (HRESULT)
	 *  @return the result of
	 */
	virtual bool ADDIN_API AddError ( unsigned short wcode, const WCHAR_T* source,
									  const WCHAR_T* descr, long scode ) = 0;

	/// Reads a property value
	/**
	 *  @param wszPropName -property name
	 *  @param pVal - value being returned
	 *  @param pErrCode - error code (if any error occured)
	 *  @param errDescriptor - error description (if any error occured)
	 *  @return the result of read.
	 */
	virtual bool ADDIN_API Read ( WCHAR_T* wszPropName,
								  tVariant* pVal,
								  long* pErrCode,
								  WCHAR_T** errDescriptor ) = 0;
	/// Writes a property value
	/**
	 *  @param wszPropName - property name
	 *  @param pVar - new property value
	 *  @return the result of write.
	 */
	virtual bool ADDIN_API Write ( WCHAR_T* wszPropName,
								   tVariant* pVar ) = 0;

	///Registers profile components
	/**
	 *  @param wszProfileName - profile name
	 *  @return the result of
	 */
	virtual bool ADDIN_API RegisterProfileAs ( WCHAR_T* wszProfileName ) = 0;

	/// Changes the depth of event Buffer
	/**
	 *  @param lDepth - new depth of event Buffer
	 *  @return the result of
	 */
	virtual bool ADDIN_API SetEventBufferDepth ( long lDepth ) = 0;
	/// Returns the depth of event Buffer
	/**
	 *  @return the depth of event Buffer
	 */
	virtual long ADDIN_API GetEventBufferDepth () = 0;
	/// Registers external event
	/**
	 *  @param wszSource - source of event
	 *  @param wszMessage - event message
	 *  @param wszData - message parameters
	 *  @return the result of
	 */
	virtual bool ADDIN_API ExternalEvent ( const WCHAR_T* wszSource,
										   const WCHAR_T* wszMessage,
										   const WCHAR_T* wszData ) = 0;
	/// Clears event Buffer
	/**
	 */
	virtual void ADDIN_API CleanEventBuffer () = 0;

	/// Sets status line contents
	/**
	 *  @param wszStatusLine - new status line contents
	 *  @return the result of
	 */
	virtual bool ADDIN_API SetStatusLine ( WCHAR_T* wszStatusLine ) = 0;
	/// Resets the status line contents
	/**
	 *  @return the result of
	 */
	virtual void ADDIN_API ResetStatusLine () = 0;
};

class IAddInDefBaseEx :
		public IAddInDefBase {
public:
	virtual ~IAddInDefBaseEx () {}

	virtual IInterface* ADDIN_API GetInterface ( Interfaces iface ) = 0;
};

struct IMsgBox :
		public IInterface {
	virtual bool ADDIN_API Confirm ( const WCHAR_T* queryText, tVariant* retVal ) = 0;

	virtual bool ADDIN_API Alert ( const WCHAR_T* text ) = 0;
};

struct IPlatformInfo :
		public IInterface {
	enum AppType {
		eAppUnknown = -1,
		eAppThinClient = 0,
		eAppThickClient,
		eAppWebClient,
		eAppServer,
		eAppExtConn,
		eAppMobileClient,
		eAppMobileServer,
	};

	struct AppInfo {
		const WCHAR_T* AppVersion;
		const WCHAR_T* UserAgentInformation;
		AppType Application;
	};

	virtual const AppInfo* ADDIN_API GetPlatformInfo () = 0;
};
#endif
