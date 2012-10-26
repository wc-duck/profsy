/*
   DOC ME HERE ASWELL!.
   version 0.1, october, 2012

   Copyright (C) 2012- Fredrik Kihlander

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
      claim that you wrote the original software. If you use this software
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.

   Fredrik Kihlander
*/

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
#if defined(__GNUC__)
		timespec start;
		clock_gettime( CLOCK_MONOTONIC, &start );
		return (uint64_t)start.tv_sec * (uint64_t)1000000000 + (uint64_t)start.tv_nsec;
#elif defined(_MSC_VER)	
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		return (uint64_t)t.QuadPart;
#else
	#error Unknown compiler
#endif
}

#endif // PROFSY_INTERNAL_H_INCLUDED
