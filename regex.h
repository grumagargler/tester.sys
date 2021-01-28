#ifndef __regex_h__
#define __regex_h__
#include "extender.h"

class Regex : public Extender {
public:
	Regex ();
	static std::wregex Init ( const std::wstring& Pattern );
private:
	bool select ( tVariant* Params, tVariant* Result );
	bool test ( tVariant* Params, tVariant* Result );
	bool replace ( tVariant* Params, tVariant* Result );
};
#endif
