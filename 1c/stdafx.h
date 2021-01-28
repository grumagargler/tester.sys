// stdafx.h : include File for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef __stfafx_h__
#define __stfafx_h__
#ifdef _WINDOWS
#include <Windows.h>
#endif //_WINDOWS
#if defined(__linux__) || defined(__APPLE__)
#define __linux__
#endif
#endif
