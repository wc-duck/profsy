/*
   Profsy - a simple "drop-in" profiler for realtime, frame-based, applications, in other words games!

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

#include <profsy/profsy_util.h>

#if defined(__GNUC__)
#  include <unistd.h>
#elif defined(_MSC_VER)
#  include <process.h>
#endif

#define PROFSY_STRINGIFY( ... ) #__VA_ARGS__

static const char PROFSY_CHROME_TRACE_ENTRY[] =
	PROFSY_STRINGIFY( { "cat":"profsy",
						"pid":%d,
						"tid":"main",
						"ts":%lu,
						"ph":"%c",
						"name":"%s",
						"args":{} } );

static int profsy_getpid()
{
#if defined(__GNUC__)
	return getpid();
#elif defined(_MSC_VER)
	return _getpid();
#else
	return 0;
#endif
}

static void profsy_util_dump_text( FILE* s, profsy_trace_entry* entries )
{
}

static void profsy_util_dump_chrome( FILE* s, profsy_trace_entry* entries )
{
	int pid = profsy_getpid();

	fprintf( s, "{ \"traceEvents\" : [\n" );

	profsy_trace_entry* e = entries;
	while( e->event < PROFSY_TRACE_EVENT_END )
	{
		profsy_scope_data* data = profsy_get_scope_data( (int)e->scope );
		fprintf( s, PROFSY_CHROME_TRACE_ENTRY,
					pid,
				    e->ts / 1000,
				    e->event == PROFSY_TRACE_EVENT_ENTER ? 'B' : 'E',
				    data->name );
		++e;
		if( e->event < PROFSY_TRACE_EVENT_END )
			fprintf( s, ",\n" );
	}

	fprintf( s, "\n] }\n" );
}

void profsy_util_dump_to_stream( FILE* s, profsy_trace_entry* entries, unsigned int format )
{
	switch( format )
	{
		case PROFSY_UTIL_DUMP_FORMAT_TEXT:   profsy_util_dump_text( s, entries ); break;
		case PROFSY_UTIL_DUMP_FORMAT_CHROME: profsy_util_dump_chrome( s, entries ); break;
	}
}

void profsy_util_dump_to_file( const char* filename, profsy_trace_entry* entries, unsigned int format )
{
	FILE* f = fopen( filename, "wt" );
	if( f == 0x0 )
		return;
	profsy_util_dump_to_stream( f, entries, format );
	fclose( f );
}
