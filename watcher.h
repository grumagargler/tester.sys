#ifndef __watcher_h__
#define __watcher_h__
#include <thread>
#include <atomic>
#include <filesystem>
#include "extender.h"
#ifdef __linux__
#include "inotify.h"
#endif

class Watcher : public Extender {
public:
	class Observer {
	public:
#ifdef _WIN32
		const long watcherBuffer {
				16380 * 2 }; // https://qualapps.blogspot.ca/2010/05/understanding-readdirectorychangesw_19.html
		FILE_NOTIFY_INFORMATION* info;
#endif

		Observer ( Watcher* Parent, const wchar_t* Folder, IAddInDefBase* Connector );
#ifdef __linux__
		void Start () const;
#elif _WIN32
		void Start ();
#endif
	private:
		struct Actions {
			static constexpr WCHAR_T Added[] = { '1', '\0' };
			static constexpr WCHAR_T Removed[] = { '2', '\0' };
			static constexpr WCHAR_T Changed[] = { '3', '\0' };
			static constexpr WCHAR_T RenamedOld[] = { '4', '\0' };
			static constexpr WCHAR_T RenamedNew[] = { '5', '\0' };
			static constexpr WCHAR_T FolderAdded[] = { '6', '\0' };
			static constexpr WCHAR_T FolderRemoved[] = { '7', '\0' };
			static constexpr WCHAR_T FolderChanged[] = { '8', '\0' };
			static constexpr WCHAR_T FolderRenamedOld[] = { '9', '\0' };
			static constexpr WCHAR_T FolderRenamedNew[] = { '0', '\0' };
			static constexpr WCHAR_T WatcherEntryRemoved[] = { 'a', '\0' };
			static constexpr WCHAR_T WatcherEntryMoved[] = { 'b', '\0' };
			static constexpr WCHAR_T VolumeUnmounted[] = { 'c', '\0' };
			static constexpr WCHAR_T WatcherOverflow[] = { 'd', '\0' };
			static constexpr WCHAR_T WatcherDisconnected[] = { 'e', '\0' };
		};

		Watcher* Parent;
		std::filesystem::path Folder;
		std::wstring File;
		IAddInDefBase* Connector;
		std::vector<std::byte> Buffer;

#ifdef __linux__
		void observing () const;
#elif _WIN32
		void observing ();
#endif

#ifdef __linux__
		void sendMessage ( const Inotify::Notification* Notification ) const;
		static const WCHAR_T* actionToName ( const Inotify::Action& Event );
		static bool hasEvent ( const Inotify::Action& Source, const Inotify::Action& Event );
#elif _WIN32
		void sendMessage ( DWORD Offset = 0 );
		[[nodiscard]]
		const wchar_t* actionToName () const;
#endif
	};

#ifdef __linux__
	Inotify::Inotify Notifier;
#elif _WIN32
	HANDLE FolderID;
#endif
	std::thread* Thread;
	bool Active;

	Watcher ();
	~Watcher () override;
	static void StartObserver ( Watcher* Parent, const std::wstring& Folder, IAddInDefBase* Connector );
	void StopWatching ();
	void StartWatching ( tVariant* Params );
	void Watch ( tVariant* Params );
};
#endif
