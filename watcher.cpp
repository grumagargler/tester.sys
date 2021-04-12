#include <thread>
#include <atomic>
#ifdef __linux__
#include "inotify.h"
#include "chars.h"
#elif _WIN32
#include <filesystem>
#endif
#include "watcher.h"

Watcher::Watcher () : Extender ( L"Watcher" ), Active ( false ), Paused ( false ), Thread ( nullptr ) {
	methods.AddProcedure ( L"Start", L"Старт", 1, [ & ] ( tVariant* Params ) {
		watch ( Params );
		return true;
	} );
	methods.AddProcedure ( L"Pause", L"Пауза", 0, [ & ] ( tVariant* Params ) {
		return pause ();
	} );
	methods.AddProcedure ( L"Resume", L"Продолжать", 0, [ & ] ( tVariant* Params ) {
		return resume ();
	} );
	methods.AddProcedure ( L"Stop", L"Стоп", 0, [ & ] ( tVariant* Params ) {
		stopWatching ();
		return true;
	} );
}

Watcher::~Watcher () {
	stopWatching ();
}

void Watcher::stopWatching () {
	if ( Thread && Active ) {
		deactivate ();
#ifdef __linux__
		Notifier.Stop ();
#elif _WIN32
		CancelIoEx ( FolderID, nullptr );
		CloseHandle ( FolderID );
#endif
		Thread->detach ();
		delete Thread;
		Thread = nullptr;
	}
}

void Watcher::watch ( tVariant* Params ) {
	stopWatching ();
	startWatching ( Params );
}

void Watcher::startWatching ( tVariant* Params ) {
	std::wstring folder { Chars::WCHARToWide ( Params->pwstrVal ) };
	Thread = new std::thread ( startObserver, this, folder, baseConnector );
}

void Watcher::startObserver ( Watcher* Parent, const std::wstring& Folder, IAddInDefBase* Connector ) {
	Observer resident { Parent, Folder.data (), Connector };
	resident.Start ();
}

Watcher::Observer::Observer ( Watcher* Parent, const wchar_t* Folder, IAddInDefBase* Connector )
		: Parent ( Parent ), Folder ( Folder ), Connector ( Connector ) {}

void Watcher::Observer::Start () {
#if _WIN32
	Buffer.resize ( watcherBuffer );
	auto folderID = CreateFileW ( Folder.c_str (), FILE_LIST_DIRECTORY,
								  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
								  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr );
	if ( folderID == INVALID_HANDLE_VALUE ) {
		Parent->SendError ( std::system_error ( GetLastError (), std::system_category () ).what () );
		return;
	}
	Parent->FolderID = folderID;
#endif
	Parent->activate ();
	observing ();
}

#ifdef __linux__
void Watcher::Observer::observing () const {
	using namespace Inotify;
	EventObserver handler = [ & ] ( const Notification& notification ) {
		if ( Parent->Paused ) {
			return;
		}
		sendMessage ( &notification );
	};
	auto events = { Action::create,
					Action::create | Action::isDir,
					Action::modify,
					Action::modify | Action::isDir,
					Action::remove,
					Action::remove | Action::isDir,
					Action::movedFrom,
					Action::movedFrom | Action::isDir,
					Action::movedTo,
					Action::movedTo | Action::isDir,
					Action::removeSelf | Action::isDir,
					Action::moveSelf | Action::isDir,
					Action::unmount,
					Action::ignored,
					Action::overflow
	};
	auto& runner = Parent->Notifier;
	try {
		runner.Watch ( Folder );
		runner.Subscribe ( events, handler );
	} catch ( std::exception& E ) {
		Parent->SendError ( E.what () );
	}
	while ( true ) {
		try {
			runner.Go ();
			break;
		} catch ( std::exception& E ) {
			Parent->SendError ( E.what () );
		}
	}
}
#elif _WIN32
void Watcher::Observer::observing () {
	DWORD returned;
	auto data = Buffer.data ();
	auto size = Buffer.size ();
	auto id = Parent->FolderID;
	auto filter = FILE_NOTIFY_CHANGE_FILE_NAME
				  | FILE_NOTIFY_CHANGE_DIR_NAME
				  | FILE_NOTIFY_CHANGE_SIZE
				  | FILE_NOTIFY_CHANGE_LAST_WRITE;
	while ( Parent->Active ) {
		if ( ReadDirectoryChangesW ( id, data, size, true, filter, &returned, nullptr, nullptr ) ) {
			if ( !Parent->Paused ) {
				sendMessage ();
			}
		}
	}
}
#endif

#ifdef __linux__
void Watcher::Observer::sendMessage ( const Inotify::Notification* Notification ) const {
	Connector->ExternalEvent ( Parent->ExtensionID, actionToName ( Notification->Event ),
							   Chars::ToWCHAR ( Notification->File.string ().data () ).get () );
}

bool Watcher::Observer::hasEvent ( const Inotify::Action& Source, const Inotify::Action& Event ) {
	return static_cast<std::uint32_t> ( Event & Source );
}

#elif _WIN32
void Watcher::Observer::sendMessage ( DWORD Offset ) {
	info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>( &Buffer [ Offset ] );
	File = Folder / std::wstring ( info->FileName, 0, info->FileNameLength / sizeof ( wchar_t ) );
	Connector->ExternalEvent ( Parent->ExtensionID, actionToName (), File.c_str () );
	if ( info->NextEntryOffset ) sendMessage ( Offset + info->NextEntryOffset );
}
#endif

#ifdef __linux__
const WCHAR_T* Watcher::Observer::actionToName ( const Inotify::Action& Event ) {
	using namespace Inotify;
	auto directory = hasEvent ( Event, Action::isDir );
	if ( hasEvent ( Event, Action::movedFrom ) )
		return directory ? Actions::FolderRenamedOld : Actions::RenamedOld;
	if ( hasEvent ( Event, Action::movedTo ) )
		return directory ? Actions::FolderRenamedNew : Actions::RenamedNew;
	if ( hasEvent ( Event, Action::create ) )
		return directory ? Actions::FolderAdded : Actions::Added;
	if ( hasEvent ( Event, Action::remove ) )
		return directory ? Actions::FolderRemoved : Actions::Removed;
	if ( hasEvent ( Event, Action::removeSelf ) )
		return Actions::WatcherEntryRemoved;
	if ( hasEvent ( Event, Action::modify ) )
		return directory ? Actions::FolderChanged : Actions::Changed;
	if ( hasEvent ( Event, Action::moveSelf ) )
		return Actions::WatcherEntryMoved;
	if ( hasEvent ( Event, Action::unmount ) )
		return Actions::VolumeUnmounted;
	if ( hasEvent ( Event, Action::overflow ) )
		return Actions::WatcherOverflow;
	if ( hasEvent ( Event, Action::ignored ) )
		return Actions::WatcherDisconnected;
	return nullptr;
}

#elif _WIN32
const wchar_t* Watcher::Observer::actionToName () const {
	auto Directory = std::filesystem::is_directory ( File );
	switch ( info->Action ) {
		case 1:
			return Directory ? Actions::FolderAdded : Actions::Added;
		case 2:
			return Directory ? Actions::FolderRemoved : Actions::Removed;
		case 3:
			return Directory ? Actions::FolderChanged : Actions::Changed;
		case 4:
			return Directory ? Actions::FolderRenamedOld : Actions::RenamedOld;
		case 5:
			return Directory ? Actions::FolderRenamedNew : Actions::RenamedNew;
	}
	return nullptr;
}
#endif

bool Watcher::pause () {
	if ( !checkActivity () ) {
		return false;
	}
	Paused = true;
	return true;
}

bool Watcher::checkActivity () {
	if ( Active ) {
		return true;
	}
	SetError<std::wstring> ( L"Watcher is not yet started" );
	return false;
}

bool Watcher::resume () {
	if ( !checkActivity () ) {
		return false;
	}
	Paused = false;
	return true;
}

void Watcher::activate () {
	Active = true;
	Paused = false;
}

void Watcher::deactivate () {
	Active = false;
	Paused = false;
}
