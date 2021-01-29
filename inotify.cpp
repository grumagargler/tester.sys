#ifdef __linux__
#include "inotify.h"
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

namespace fs = std::filesystem;

namespace Inotify {
	Notification::Notification ( const Action& event, const std::filesystem::path& File, bool Directory )
			: Event ( event ), File ( File ), Directory ( Directory ) {};

	SystemEvent::SystemEvent ( uint32_t Mask, std::filesystem::path Path, bool Directory )
			: Mask ( Mask ), Path ( std::move ( Path ) ), Directory ( Directory ) {};

	Inotify::Inotify ()
			: EventMask ( IN_ALL_EVENTS ), IgnoreFolders ( std::vector<std::string> () ),
			  InotifyDescriptor ( 0 ), EventBuffer ( EventsLimit * ( EventSize + 16 ), 0 ),
			  PipeReadIndex ( 0 ), PipeWriteIndex ( 1 ), Stopped ( false ) {
		if ( pipe2 ( StopPipeDescriptor, O_NONBLOCK ) == -1 ) {
			std::stringstream errorStream;
			errorStream << "Can't initialize stop-pipe: " << strerror ( errno );
			throw std::runtime_error ( errorStream.str () );
		}
		InotifyDescriptor = inotify_init1 ( IN_NONBLOCK );
		if ( InotifyDescriptor == -1 ) {
			std::stringstream errorStream;
			errorStream << "Can't initialize inotify: " << strerror ( errno );
			throw std::runtime_error ( errorStream.str () );
		}
		EpollDescriptor = epoll_create1 ( 0 );
		if ( EpollDescriptor == -1 ) {
			std::stringstream errorStream;
			errorStream << "Can't initialize epoll: " << strerror ( errno );
			throw std::runtime_error ( errorStream.str () );
		}
		InotifyEpollEvent.events = EPOLLIN | EPOLLET;
		InotifyEpollEvent.data.fd = InotifyDescriptor;
		if ( epoll_ctl ( EpollDescriptor, EPOLL_CTL_ADD, InotifyDescriptor, &InotifyEpollEvent ) == -1 ) {
			std::stringstream errorStream;
			errorStream << "Can't assign inotify's filedescriptor to epoll: " << strerror ( errno );
			throw std::runtime_error ( errorStream.str () );
		}
		StopPipeEpollEvent.events = EPOLLIN | EPOLLET;
		StopPipeEpollEvent.data.fd = StopPipeDescriptor[ PipeReadIndex ];
		if ( epoll_ctl ( EpollDescriptor, EPOLL_CTL_ADD,
						 StopPipeDescriptor[ PipeReadIndex ], &StopPipeEpollEvent ) == -1 ) {
			std::stringstream errorStream;
			errorStream << "Can't assign pipe filedescriptor to epoll: " << strerror ( errno );
			throw std::runtime_error ( errorStream.str () );
		}
	}

	Inotify::~Inotify () {
		epoll_ctl ( EpollDescriptor, EPOLL_CTL_DEL, InotifyDescriptor, nullptr );
		epoll_ctl ( EpollDescriptor, EPOLL_CTL_DEL, StopPipeDescriptor[ PipeReadIndex ], nullptr );
		close ( InotifyDescriptor );
		close ( EpollDescriptor );
		close ( StopPipeDescriptor[ PipeReadIndex ] );
		close ( StopPipeDescriptor[ PipeWriteIndex ] );
	}

	void Inotify::Watch ( const std::filesystem::path& Path ) {
		checkPath ( Path );
		std::vector<std::filesystem::path> list;
		list.push_back ( Path );
		if ( fs::is_directory ( Path ) ) {
			std::error_code ec;
			fs::recursive_directory_iterator it ( Path, fs::directory_options::follow_directory_symlink, ec );
			fs::recursive_directory_iterator end;
			for ( ; it != end; it.increment ( ec ) ) {
				fs::path currentPath = *it;
				if ( fs::is_directory ( currentPath ) && !fs::is_symlink ( currentPath ) ) {
					list.push_back ( currentPath );
				}
			}
		}
		for ( auto& entry : list ) {
			WatchPath ( entry );
		}
	}

	void Inotify::checkPath ( const std::filesystem::path& Path ) {
		if ( !fs::exists ( Path ) ) {
			throw std::invalid_argument ( "Path not found and will not be monitored: " + Path.string () );
		}
	}

	void Inotify::WatchPath ( const std::filesystem::path& Path ) {
		if ( isIgnored ( Path ) ) {
			return;
		}
		checkPath ( Path );
		auto descriptor = inotify_add_watch ( InotifyDescriptor, Path.string ().c_str (), EventMask );
		if ( descriptor == -1 ) {
			std::stringstream errorStream;
			auto path = Path.string ();
			if ( errno == ENOSPC ) {
				errorStream << "The user limit on the total number of inotify watches was"
							   " reached or the kernel failed to allocate a needed resource: " << path;
				throw std::runtime_error ( errorStream.str () );
			} else {
				errorStream << "inotify_add_watch failed with error: " << strerror ( errno ) << ". Path: " << path;
				throw std::runtime_error ( errorStream.str () );
			}
		}
		savePath ( descriptor, Path );
	}

	void Inotify::savePath ( int Descriptor, const fs::path& Path ) {
		Folders.insert ( { Descriptor, Path } );
		Descriptors.insert ( { Path, Descriptor } );
	}

	[[maybe_unused]] [[maybe_unused]] void Inotify::ReleasePath ( const std::filesystem::path& File ) {
		int result = inotify_rm_watch ( InotifyDescriptor, Descriptors.at ( File ) );
		if ( result == -1 ) {
			std::stringstream errorStream;
			errorStream << "inotify_rm_watch failed with error: " << strerror ( errno )
						<< ". Path:" << File.string ();
			throw std::runtime_error ( errorStream.str () );
		}
	}

	void Inotify::Subscribe ( const std::vector<Action>& Events, const EventObserver& EventObserver ) {
		for ( auto event : Events ) {
			EventMask |= static_cast<std::uint32_t>(event);
			Observer[ event ] = EventObserver;
		}
	}

	std::optional<SystemEvent> Inotify::getNext () {
		std::vector<SystemEvent> newEvents;
		while ( EventQueue.empty () && !Stopped ) {
			auto length = readEvents ();
			fetchEvents ( length, newEvents );
			filterEvents ( newEvents );
		}
		if ( Stopped ) {
			return std::nullopt;
		}
		auto event = EventQueue.front ();
		EventQueue.pop ();
		return event;
	}

	void Inotify::Stop () {
		Stopped = true;
		sendStopSignal ();
	}

	void Inotify::sendStopSignal () {
		std::vector<std::uint8_t> buffer ( 1, 0 );
		write ( StopPipeDescriptor[ PipeWriteIndex ], buffer.data (), buffer.size () );
	}

	bool Inotify::isIgnored ( const std::filesystem::path& Path ) {
		auto path = Path.string ();
		for ( auto& directory : IgnoreFolders ) {
			auto pos = path.find ( directory );
			if ( pos != std::string::npos ) {
				return true;
			}
		}
		return false;
	}

	ssize_t Inotify::readEvents () {
		auto count = epoll_wait ( EpollDescriptor, EpollEvents, 1, -1 );
		if ( count < 1 ) {
			return 0;
		}
		auto descriptor = EpollEvents[ 0 ].data.fd;
		if ( descriptor == StopPipeDescriptor[ PipeReadIndex ] ) {
			return 0;
		}
		return read ( descriptor, EventBuffer.data (), EventBuffer.size () );
	}

	void Inotify::fetchEvents ( int Size, std::vector<SystemEvent>& Events ) {
		decltype ( EventSize ) i { 0 };
		auto buffer = EventBuffer.data ();
		while ( i < Size ) {
			inotify_event* event = ( ( struct inotify_event* ) &buffer[ i ] );
			i += EventSize + event->len;
			if ( event->mask & IN_IGNORED ) {
				deleteDescriptor ( event->wd );
			} else {
				auto path = Folders.at ( event->wd ) / std::string ( event->name );
				if ( !path.empty () ) {
					SystemEvent fsEvent ( event->mask, path, fs::is_directory ( path ) );
					Events.push_back ( fsEvent );
				}
			}
		}
	}

	void Inotify::deleteDescriptor ( int Descriptor ) {
		Descriptors.erase ( Folders.at ( Descriptor ) );
		Folders.erase ( Descriptor );
	}

	void Inotify::filterEvents ( std::vector<SystemEvent>& Events ) {
		for ( auto event = Events.begin (); event < Events.end (); ) {
			auto currentEvent = *event;
			if ( isIgnored ( currentEvent.Path ) ) {
				event = Events.erase ( event );
			} else {
				EventQueue.push ( currentEvent );
				++event;
			}
		}
	}

	void Inotify::Go () {
		while ( true ) {
			if ( Stopped ) {
				return;
			}
			waitForEvent ();
		}
	}

	void Inotify::waitForEvent () {
		auto systemEvent = getNext ();
		if ( !systemEvent ) {
			return;
		}
		auto event = static_cast<Action>(systemEvent->Mask);
		auto observer = listening ( event );
		if ( !observer ) {
			return;
		}
		auto& path = systemEvent->Path;
		auto isFolder = systemEvent->Directory;
		if ( isFolder && static_cast<bool>(event & Action::create ) ) {
			attachFolder ( path, observer );
		} else {
			( *observer ) ( Notification { event, path, isFolder } );
		}
	}

	EventObserver* Inotify::listening ( Action Event ) {
		auto nothing = Observer.end ();
		auto item = Observer.find ( Event );
		if ( item == nothing ) {
			item = Observer.find ( Action::all );
		}
		return item == nothing ? nullptr : &item->second;
	}

	void Inotify::attachFolder ( const std::filesystem::path& Folder, const EventObserver* Handler ) {
		( *Handler ) ( Notification { Action::create | Action::isDir, Folder, true } );
		namespace fs = std::filesystem;
		std::error_code error;
		fs::directory_iterator it ( Folder, fs::directory_options::follow_directory_symlink, error );
		fs::directory_iterator end;
		std::vector<fs::directory_entry> folders;
		for ( ; it != end; it.increment ( error ) ) {
			auto& path = *it;
			auto directory = ( fs::is_directory ( path ) && !fs::is_symlink ( path ) );
			if ( directory ) {
				folders.push_back ( path );
			} else {
				auto observer = listening ( Action::create );
				( *observer ) ( Notification { Action::create, path, false } );
			}
		}
		WatchPath ( Folder );
		for ( auto& folder : folders ) {
			attachFolder ( folder, Handler );
		}
	}
}
#endif