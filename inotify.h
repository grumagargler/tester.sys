#ifndef __inotify_h__
#define __inotify_h__
#ifdef __linux__
#include <cassert>
#include <filesystem>
#include <optional>
#include <cerrno>
#include <exception>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <ctime>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#define EventsLimit 1024
#define EventSize ( sizeof ( inotify_event ) )

namespace Inotify {
	enum class Action : std::uint32_t {
		create = IN_CREATE,
		remove = IN_DELETE,
		removeSelf = IN_DELETE_SELF,
		modify = IN_MODIFY,
		moveSelf = IN_MOVE_SELF,
		movedFrom = IN_MOVED_FROM,
		movedTo = IN_MOVED_TO,
		isDir = IN_ISDIR,
		unmount = IN_UNMOUNT,
		overflow = IN_Q_OVERFLOW,
		ignored = IN_IGNORED,
		all = IN_ALL_EVENTS
	};

	constexpr Action operator& ( Action Left, Action Right ) {
		return static_cast<Action>(
				static_cast<std::underlying_type<Action>::type>(Left)
				& static_cast<std::underlying_type<Action>::type>(Right));
	}

	constexpr Action operator| ( Action Left, Action Right ) {
		return static_cast<Action>(
				static_cast<std::underlying_type<Action>::type>(Left)
				| static_cast<std::underlying_type<Action>::type>(Right));
	}

	struct Notification {
		const Action Event;
		const bool Directory;
		const std::filesystem::path& File;

		Notification ( const Action& event, const std::filesystem::path& File, bool Directory );
	};

	struct SystemEvent {
		uint32_t Mask;
		std::filesystem::path Path;
		bool Directory;

		SystemEvent ( uint32_t Mask, std::filesystem::path Path, bool Directory );
	};

	using EventObserver = std::function<void ( Notification )>;

	class Inotify {
	public:
		std::vector<std::string> IgnoreFolders;

		Inotify ();
		~Inotify ();
		void Watch ( const std::filesystem::path& Path );
		void WatchPath ( const std::filesystem::path& Path );
		void Subscribe ( const std::vector<Action>& Events, const EventObserver& EventObserver );
		void Go ();
		void Stop ();
		[[maybe_unused]] void ReleasePath ( const std::filesystem::path& File );
	private:
		std::map<Action, EventObserver> Observer;
		uint32_t EventMask;
		std::queue<SystemEvent> EventQueue;
		std::map<int, std::filesystem::path> Folders;
		std::map<std::filesystem::path, int> Descriptors;
		int InotifyDescriptor;
		std::atomic<bool> Stopped;
		int EpollDescriptor;
		epoll_event InotifyEpollEvent;
		epoll_event StopPipeEpollEvent;
		epoll_event EpollEvents[1];
		std::vector<uint8_t> EventBuffer;
		int StopPipeDescriptor[2];
		const int PipeReadIndex;
		const int PipeWriteIndex;

		void waitForEvent ();
		ssize_t readEvents ();
		void fetchEvents ( int Size, std::vector<SystemEvent>& Events );
		void filterEvents ( std::vector<SystemEvent>& Events );
		void sendStopSignal ();
		bool isIgnored ( const std::filesystem::path& Path );
		EventObserver* listening ( Action Event );
		void attachFolder ( const std::filesystem::path& Folder, const EventObserver* Handler );
		std::optional<SystemEvent> getNext ();
		static void checkPath ( const std::filesystem::path& Path );
		void savePath ( int Descriptor, const std::filesystem::path& Path );
		void deleteDescriptor ( int Descriptor );
	};
}
#endif
#endif
