#pragma once
#include "extender.h"
// X11 headers define macros that break httplib's enum names.
#ifdef Success
#undef Success
#endif
#ifdef None
#undef None
#endif
#include <httplib.h>
#include <thread>
#include <optional>

class HTTPServer : public Extender {
public:
	HTTPServer ();
	~HTTPServer () override;

private:
	struct BindError {
		enum class Source { none, invalidServer, socket };
		Source source { Source::none };
		int code { 0 };
		std::string message;
	};
	httplib::Server server;
	std::mutex mutex, router;
	std::jthread worker;
	std::optional<std::string> data;
	std::condition_variable condition;
	bool waiting;

	void init ();
	bool start ( tVariant* Params );
	void send ( tVariant* Params );
	BindError lastError ();
	std::optional<BindError> startServer ( std::string host, int port );
	void stop ();
};
