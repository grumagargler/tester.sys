#ifndef __root_h__
#define __root_h__
#include "extender.h"
#include "shooter.h"

class Root : public Extender {
public:
	Root ();
private:
	bool shoot ( tVariant* Params, tVariant* Result );
	bool maximize ( tVariant* Params );
	bool minimize ( tVariant* Params );
	static void pause ( tVariant* Params );
	void getEnvironment ( tVariant* Params, tVariant* Result );
	static void gotoConsole ( [[maybe_unused]] tVariant* Params );
#ifdef __linux__
	void getPicture ( Shooter::RawBuffer& Buffer, tVariant* Result ) const;
#elif _WIN32
	void getPicture ( IStream* Image, tVariant* Result ) const;
#endif
};
#endif