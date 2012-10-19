#ifndef PROFSY_INTERNAL_H_INCLUDED
#define PROFSY_INTERNAL_H_INCLUDED

#include <stdint.h>
#include <time.h>

#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#endif // defined(_MSC_VER)

inline uint64_t profsy_get_tick()
{
#if defined(GNUC)
		timespec start;
		clock_gettime( CLOCK_MONOTONIC, &start );
		return (uint64_t)start.tv_sec * (uint64)1000000000 + (uint64)start.tv_nsec;
#elif defined(_MSC_VER)	
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		return (uint64_t)t.QuadPart;
#else
	#error Unknown compiler
#endif
}

#endif // PROFSY_INTERNAL_H_INCLUDED