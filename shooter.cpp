#include "shooter.h"
#include "regex.h"
#if __linux__
#include <codecvt>
#include <unistd.h>
#include <chrono>

Shooter::RawBuffer::RawBuffer () : Buffer ( nullptr ), Copied ( false ), Size ( 0 ) {}

Shooter::RawBuffer::RawBuffer ( RawBuffer& Parent ) : Buffer ( Parent.Buffer ), Copied ( false ), Size ( Parent.Size ) {
	Parent.Copied = true;
}

Shooter::RawBuffer::RawBuffer ( Shooter::RawBuffer&& Parent ) noexcept
		: Buffer ( Parent.Buffer ), Size ( Parent.Size ), Copied ( false ) {
	Parent.Copied = true;
}

Shooter::RawBuffer::~RawBuffer () {
	if ( !Copied && Buffer ) {
		free ( Buffer );
	}
}

Shooter::RawBuffer& Shooter::RawBuffer::operator= ( Shooter::RawBuffer& Parent ) {
	if ( this == &Parent ) {
		return *this;
	}
	Buffer = Parent.Buffer;
	Copied = Parent.Copied;
	Size = Parent.Size;
	Parent.Copied = true;
	return *this;
}

Shooter::RawBuffer& Shooter::RawBuffer::operator= ( Shooter::RawBuffer&& Parent ) noexcept {
	Buffer = Parent.Buffer;
	Size = Parent.Size;
	Copied = false;
	Parent.Copied = true;
	return *this;
}

Shooter::Shooter () {
	Monitor = XOpenDisplay ( nullptr );
	XSetErrorHandler ( errorHandler );
}

Shooter::~Shooter () {
	XCloseDisplay ( Monitor );
	XSetErrorHandler ( nullptr );
}

void Shooter::Minimize ( const std::wstring& Title ) {
	auto window = findWindow ( Title );
	if ( window == std::nullopt ) {
		return;
	}
	XIconifyWindow ( Monitor, window.value (), DefaultScreen ( Monitor ) );
}

void Shooter::Maximize ( const std::wstring& Title ) {
	auto window = findWindow ( Title );
	if ( window == std::nullopt ) {
		return;
	}
	auto state1 = XInternAtom ( Monitor, "_NET_WM_STATE_MAXIMIZED_VERT", false );
	auto state2 = XInternAtom ( Monitor, "_NET_WM_STATE_MAXIMIZED_HORZ", false );
	auto frame = window.value ();
	changeState ( frame, false, state1, state2 );
	activate ( frame );
}

std::optional<Shooter::RawBuffer> Shooter::Take ( const std::wstring& Title ) {
	auto window = findWindow ( Title );
	if ( window == std::nullopt ) {
		return std::nullopt;
	}
	XWindowAttributes attributes;
	XImage* image;
	auto frame = window.value ();
	auto waiting = WaitingActivation;
	while ( waiting ) {
		activate ( frame );
		try {
			XGetWindowAttributes ( Monitor, frame, &attributes );
			image = XGetImage ( Monitor, frame, 0, 0, attributes.width, attributes.height, AllPlanes, ZPixmap );
			break;
		} catch ( ... ) {
			usleep ( WaitingPause );
			waiting -= WaitingPause;
			continue;
		}
	}
	auto result = getPng { image } ();
	XDestroyImage( image );
	return result;
}

int Shooter::errorHandler ( [[maybe_unused]] Display* Screen, XErrorEvent* Error ) {
	throw *Error; // NOLINT(hicpp-exception-baseclass)
}

auto Shooter::now () {
	return std::chrono::duration_cast<std::chrono::milliseconds> (
			std::chrono::system_clock::now ().time_since_epoch () ).count ();
}

void Shooter::activate ( Window Frame ) {
	XEvent event;
	auto& structure = event.xclient;
	structure.type = ClientMessage;
	structure.serial = 0;
	structure.send_event = true;
	structure.window = Frame;
	structure.message_type = XInternAtom ( Monitor, "_NET_ACTIVE_WINDOW", false );
	structure.format = 32;
	structure.data = {};
	structure.data.l[ 0 ] = RegularApplication;
	structure.data.l[ 1 ] = now ();
	auto mask = SubstructureRedirectMask | SubstructureNotifyMask;
	XSendEvent ( Monitor, DefaultRootWindow( Monitor ), false, mask, &event );
}

void Shooter::changeState ( Window Frame, bool Revoke, Atom State1, Atom State2 ) {
	XEvent event;
	auto& structure = event.xclient;
	structure.type = ClientMessage;
	structure.serial = 0;
	structure.send_event = true;
	structure.display = Monitor;
	structure.window = Frame;
	structure.message_type = XInternAtom ( Monitor, "_NET_WM_STATE", false );
	structure.format = 32;
	structure.data = {};
	structure.data.l[ 0 ] = Revoke ? 0 : 1;
	structure.data.l[ 1 ] = State1;
	structure.data.l[ 2 ] = State2;
	structure.data.l[ 3 ] = RegularApplication;
	auto mask = SubstructureRedirectMask | SubstructureNotifyMask;
	XSendEvent ( Monitor, DefaultRootWindow( Monitor ), false, mask, &event );
}

std::optional<std::wstring> Shooter::windowTitle ( Window Frame ) {
	XTextProperty property;
	try {
		if ( !XGetWMName ( Monitor, Frame, &property ) ) {
			return std::nullopt;
		}
	} catch ( ... ) {
		return std::nullopt;
	}
	int rows;
	char** strings;
	try {
		Xutf8TextPropertyToTextList ( Monitor, &property, &strings, &rows );
		XFree ( property.value );
	} catch ( ... ) {
		return std::nullopt;
	}
	if ( !rows || !*strings ) {
		return std::nullopt;
	}
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	auto result = converter.from_bytes ( *strings );
	XFreeStringList ( strings );
	return result;
}

std::optional<Window> Shooter::findWindow ( const std::wstring& Pattern ) {
	auto rex { Regex::Init ( Pattern ) };
	Atom actualType;
	int format;
	unsigned long items { 0 };
	unsigned long readMore;
	unsigned char* data { nullptr };
	XGetWindowProperty ( Monitor,
						 DefaultRootWindow ( Monitor ),
						 XInternAtom ( Monitor, "_NET_CLIENT_LIST", false ),
						 0,
						 ~0L,
						 false,
						 AnyPropertyType,
						 &actualType,
						 &format,
						 &items,
						 &readMore,
						 &data );
	if ( !items || !data ) {
		return std::nullopt;
	}
	long* windows = reinterpret_cast<long*>(data);
	std::optional<std::wstring> title;
	bool found { false };
	std::wsmatch match;
	Window window;
	while ( items-- ) {
		window = static_cast<Window>(windows[ items ]);
		title = windowTitle ( window );
		if ( title != std::nullopt ) {
			if ( std::regex_search ( title.value (), match, rex ) ) {
				found = true;
				break;
			}
		}
	}
	XFree ( data );
	return found ? std::optional ( window ) : std::nullopt;
}

Shooter::getPng::getPng ( XImage* Image ) : Image ( Image ) {}

Shooter::RawBuffer Shooter::getPng::operator() () {
	init ();
	auto row = reinterpret_cast<unsigned char*>(Image->data);
	auto depth32bits = ( Image->depth == 32 );
	auto lsb = ( Image->byte_order == LSBFirst );
	auto bytes = Image->bytes_per_line - 4;
	for ( auto height = 0; height < Image->height; ++height ) {
		if ( lsb ) {
			for ( int i = 0, j = 0; j < bytes; i += 4 ) {
				DisplayRow[ j++ ] = row[ i + 2 ];
				DisplayRow[ j++ ] = row[ i + 1 ];
				DisplayRow[ j++ ] = row[ i ];
				DisplayRow[ j++ ] = depth32bits ? row[ i + 3 ] : 255;
			}
		} else {
			for ( int i = 0, j = 0; j < bytes; i += 4 ) {
				DisplayRow[ j++ ] = row[ i + 1 ];
				DisplayRow[ j++ ] = row[ i + 2 ];
				DisplayRow[ j++ ] = row[ i + 3 ];
				DisplayRow[ j++ ] = depth32bits ? row[ i ] : 255;
			}
		}
		row += Image->bytes_per_line;
		png_write_row ( PngStructure, DisplayRow );
	}
	complete ();
	return Buffer;
}

void Shooter::getPng::init () {
	PngStructure = png_create_write_struct ( PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr );
	if ( !PngStructure ) {
		throw std::runtime_error ( "Libpng structure initialization error" );
	}
	PngInfo = png_create_info_struct ( PngStructure );
	if ( !PngInfo || setjmp( png_jmpbuf ( PngStructure ) ) ) {
		throw std::runtime_error ( "Libpng info initialization error" );
	}
	DisplayRow = new unsigned char[Image->width * 4] ();
	Stream = open_memstream ( &Buffer.Buffer, &Buffer.Size );
	png_init_io ( PngStructure, Stream );
	png_set_IHDR ( PngStructure, PngInfo, Image->width, Image->height, 8,
				   PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
				   PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );
	png_write_info ( PngStructure, PngInfo );
}

void Shooter::getPng::complete () {
	png_write_end ( PngStructure, PngInfo );
	fflush ( Stream );
	fclose ( Stream );
	delete[] DisplayRow;
	png_free_data ( PngStructure, PngInfo, PNG_FREE_ALL, -1 );
	png_destroy_write_struct ( &PngStructure, &PngInfo );
}
#elif _WIN32
BOOL CALLBACK nextWindow ( HWND window, LPARAM info ) {
	auto next { true };
	wchar_t size = GetWindowTextLengthW ( window );
	if ( !size ) {
		return next;
	}
	auto title = new wchar_t [ size ] { 0 };
	auto i = GetWindowTextW ( window, title, size );
	if ( i > 0 ) {
		auto pattern = reinterpret_cast<SearchWindow*>( info )->Title;
		if ( regex_match ( title, pattern ) ) {
			reinterpret_cast<SearchWindow*>( info )->Window = window;
			next = false;
		}
	}
	delete[] title;
	return next;
}

std::optional<IStream*> Shooter::Window ( WCHAR* Title, bool Compressed ) {
	std::optional<IStream*> result;
	if ( !locate ( Title ) ) {
		return std::nullopt;
	}
	activate ();
	auto source = getBitmap ();
	if ( Compressed ) {
		auto encoder = findEncoder ( L"image/tiff" );
		if ( encoder ) {
			Gdiplus::EncoderParameters params {};
			params.Count = 1;
			auto& parameter = params.Parameter [ 0 ];
			parameter.Guid = Gdiplus::EncoderColorDepth;
			parameter.Type = Gdiplus::EncoderParameterValueTypeLong;
			parameter.NumberOfValues = 1;
			ULONG dep = 4;
			parameter.Value = &dep;
			IStream* stream;
			if ( CreateStreamOnHGlobal ( nullptr, true, &stream ) == S_OK ) {
				source->Save ( stream, &encoder.value(), &params );
				auto reducedBitmap = Gdiplus::Bitmap::FromStream ( stream );
				stream->Release ();
				encoder = findEncoder ( L"image/png" );
				if ( CreateStreamOnHGlobal ( nullptr, true, &stream ) == S_OK ) {
					auto ok = reducedBitmap->Save ( stream, &encoder.value () );
					if ( ok == Gdiplus::Status::Ok ) {
						stream->Seek ( LARGE_INTEGER { 0 }, STREAM_SEEK_SET, nullptr );
						result = stream;
					} else {
						stream->Release ();
					}
				}
				delete reducedBitmap;
			}
		}
	} else {
		const auto encoder = findEncoder ( L"image/png" );
		IStream* stream;
		if ( CreateStreamOnHGlobal ( nullptr, true, &stream ) == S_OK ) {
			auto ok = source->Save ( stream, &encoder.value () );
			if ( ok == Gdiplus::Status::Ok ) {
				stream->Seek ( LARGE_INTEGER { 0 }, STREAM_SEEK_SET, nullptr );
				result = stream;
			} else {
				stream->Release ();
			}
		}
	}
	return result;
}

bool Shooter::locate ( WCHAR* Title ) {
	searchWindow.Title = Title;
	EnumWindows ( WNDENUMPROC { &nextWindow }, reinterpret_cast<LPARAM>( &searchWindow ) );
	return searchWindow.Window != nullptr;
}

std::optional<CLSID> Shooter::findEncoder ( const wchar_t* Format ) {
	std::optional<CLSID> result;
	UINT count { 0 };
	UINT size { 0 };
	Gdiplus::GetImageEncodersSize ( &count, &size );
	if ( size == 0 ) {
		return std::nullopt;
	}
	auto codecs = static_cast<Gdiplus::ImageCodecInfo*> ( malloc ( size ) );
	if ( !codecs ) {
		return std::nullopt;
	}
	if ( GetImageEncoders ( count, size, codecs ) != Gdiplus::Status::Ok ) {
		return std::nullopt;
	}
	for ( UINT i = 0; i < count; ++i ) {
		auto codec = codecs [ i ];
		if ( wcscmp ( codec.MimeType, Format ) == 0 ) {
			result = codec.Clsid;
			break;
		}
	}
	free ( codecs );
	return result;
}
void Shooter::activate () {
	auto hCurWnd = GetForegroundWindow ();
	auto dwMyID = GetCurrentThreadId ();
	auto dwCurID = GetWindowThreadProcessId ( hCurWnd, nullptr );
	AttachThreadInput ( dwCurID, dwMyID, TRUE );
	auto& window = searchWindow.Window;
	SetWindowPos ( window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
	SetWindowPos ( window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
	SetForegroundWindow ( window );
	AttachThreadInput ( dwCurID, dwMyID, FALSE );
	SetFocus ( window );
	SetActiveWindow ( window );
}

std::unique_ptr<Gdiplus::Bitmap> Shooter::getBitmap () const {
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup ( &gdiplusToken, &gdiplusStartupInput, nullptr );
	RECT rect { 0 };
	GetWindowRect ( searchWindow.Window, &rect );
	auto desktop = GetDesktopWindow ();
	auto desktopdc = GetDC ( desktop );
	auto mydc = CreateCompatibleDC ( desktopdc );
	auto width = ( rect.right - rect.left == 0 ) ? GetSystemMetrics ( SM_CXSCREEN ) : rect.right - rect.left;
	auto height = ( rect.bottom - rect.top == 0 ) ? GetSystemMetrics ( SM_CYSCREEN ) : rect.bottom - rect.top;
	auto mybmp = CreateCompatibleBitmap ( desktopdc, width, height );
	auto oldbmp = static_cast<HBITMAP>( SelectObject ( mydc, mybmp ) );
	BitBlt ( mydc, 0, 0, width, height, desktopdc, rect.left, rect.top, SRCCOPY | CAPTUREBLT );
	SelectObject ( mydc, oldbmp );
	BITMAP bm;
	GetObject ( mybmp, sizeof ( BITMAP ), &bm );
	BitBlt ( mydc, bm.bmWidth, 0, bm.bmWidth, bm.bmHeight, desktopdc, 0, 0, SRCCOPY );
	std::unique_ptr<Gdiplus::Bitmap> result { Gdiplus::Bitmap::FromHBITMAP ( mybmp, nullptr ) };
	ReleaseDC ( desktop, desktopdc );
	DeleteObject ( mybmp );
	DeleteDC ( mydc );
	Gdiplus::GdiplusShutdown ( gdiplusToken );
	return result;
}
#endif