#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

#if defined(_WIN32) || defined(__CYGWIN__)
	#define PLATFORM_WINDOWS
#elif defined(__APPLE__)
	#define PLATFORM_MACOS
#elif defined(__linux__)
	#define PLATFORM_LINUX
#elif defined(__FreeBSD__)
	#define PLATFORM_FREEBSD
	#define PLATFORM_BSD
#elif defined(__OpenBSD__)
	#define PLATFORM_OPENBSD
	#define PLATFORM_BSD
#else
	#define PLATFORM_UNKNOWN
#endif

#ifdef PLATFORM_WINDOWS
	#define IS_WINDOWS true
#else
	#define IS_WINDOWS false
#endif

#ifdef PLATFORM_MACOS
	#define IS_MACOS true
#else
	#define IS_MACOS false
#endif

#ifdef PLATFORM_LINUX
	#define IS_LINUX true
#else
	#define IS_LINUX false
#endif

#ifdef PLATFORM_FREEBSD
	#define IS_FREEBSD true
#else
	#define IS_FREEBSD false
#endif

#ifdef PLATFORM_OPENBSD
	#define IS_OPENBSD true
#else
	#define IS_OPENBSD false
#endif

#ifdef PLATFORM_BSD
	#define IS_BSD true
#else
	#define IS_BSD false
#endif

#endif
