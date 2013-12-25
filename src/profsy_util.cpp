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
