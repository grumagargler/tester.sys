#ifndef __shooter_h__
#define __shooter_h__
#if __linux__
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <cstdlib>
#include <string>
#include <optional>
#include <regex>
#include <png.h>

class Shooter {
public:
	struct RawBuffer {
		char* Buffer;
		bool Copied;
		size_t Size;

		RawBuffer ();
		RawBuffer ( RawBuffer& Parent );
		RawBuffer ( RawBuffer&& Parent ) noexcept;
		~RawBuffer ();
		RawBuffer& operator= ( RawBuffer& Parent );
		RawBuffer& operator= ( RawBuffer&& Parent ) noexcept;
	};

	Shooter ();
	~Shooter ();
	void Minimize ( const std::wstring& Title );
	void Maximize ( const std::wstring& Title );
	std::optional<RawBuffer> Take ( const std::wstring& Title );
private:
	const int WaitingActivation { 500 };
	const int WaitingPause { 100 };

	static int errorHandler ( [[maybe_unused]] Display* Screen, XErrorEvent* Error );
	static auto now ();
	void activate ( Window Frame );
	void changeState ( Window Frame, bool Revoke, Atom State1, Atom State2 );
	std::optional<std::wstring> windowTitle ( Window Frame );
	std::optional<Window> findWindow ( const std::wstring& Pattern );
	class getPng {
	public:
		getPng () = delete;
		explicit getPng ( XImage* Image );
		RawBuffer operator() ();
	private:
		XImage* Image;
		png_structp PngStructure;
		png_infop PngInfo;
		unsigned char* DisplayRow;
		FILE* Stream;
		RawBuffer Buffer;

		void init ();
		void complete ();
	};

	Display* Monitor;
	const long RegularApplication { 1 };
};
#elif _WIN32
#pragma comment(lib, "gdiplus.lib")
#include "Windows.h"
#include <GdiPlus.h>
#include <Regex>
#include <optional>

struct SearchWindow {
	std::wregex Title;
	HWND Window;
};

class Shooter {
public:
	std::optional<IStream*> Window ( WCHAR* Title, bool Compressed );
private:
	SearchWindow searchWindow {};

	bool locate ( WCHAR* Title );
	static std::optional<CLSID> findEncoder ( const wchar_t* Format );
	void activate ();
	[[nodiscard]]
	std::unique_ptr<Gdiplus::Bitmap> getBitmap () const;
};

BOOL CALLBACK nextWindow ( HWND window, LPARAM info );

class Expander {
public:
    void Maximize ( WCHAR* Title ) {
        toggleWindow ( Title, true );
    }
    void Minimize ( WCHAR* Title ) {
        toggleWindow ( Title, false );
    }
private:
    SearchWindow searchWindow {};

    bool locate ( WCHAR* Title ) {
        searchWindow.Title = Title;
        EnumWindows ( WNDENUMPROC { &nextWindow }, reinterpret_cast<LPARAM>( &searchWindow ) );
        return searchWindow.Window != nullptr;
    }

    void toggleWindow ( WCHAR* Title, bool Maximize ) {
        if ( locate ( Title ) ) {
            ShowWindow ( searchWindow.Window, Maximize ? SW_MAXIMIZE : SW_MINIMIZE );
        }
    }
};
#endif
#endif
