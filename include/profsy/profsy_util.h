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

#ifndef PROFSY_UTIL_H_INCLUDED
#define PROFSY_UTIL_H_INCLUDED

#include <profsy/profsy.h>

#include <stdio.h>

static const unsigned int PROFSY_UTIL_DUMP_FORMAT_TEXT = 0;
static const unsigned int PROFSY_UTIL_DUMP_FORMAT_CHROME = 1;

static const unsigned int PROFSY_UTIL_DUMP_MODE_LINE = 0;
static const unsigned int PROFSY_UTIL_DUMP_MODE_CHUNK = 1;

/**
 *
 */
void profsy_util_dump( profsy_trace_entry* entries, 
					   unsigned int format, 
					   unsigned int mode, 
					   void ( callback* )( const uint8* data ), 
					   void* userdata );

/**
 *
 */
void profsy_util_dump_to_stream( FILE* s, profsy_trace_entry* entries, unsigned int format );

/**
 *
 */
void profsy_util_dump_to_file( const char* filename, profsy_trace_entry* entries, unsigned int format );

#endif // PROFSY_UTIL_H_INCLUDED
