#include <profsy/profsy_util.h>

static void profsy_util_dump_text( FILE* s, profsy_trace_entry* entries )
{
}

static void profsy_util_dump_chrome( FILE* s, profsy_trace_entry* entries )
{
	fprintf( s, "[\n" );

	profsy_trace_entry* e = entries;
	while( e->event < PROFSY_TRACE_EVENT_END )
	{
		profsy_scope_data* data = profsy_get_scope_data( (int)e->scope );

		fprintf( s, "{ cat:\"profsy\", pid:0, tid:0,"
					   "ts:%lu, "
					   "ph:\"%c\", "
					   "name:\"%s\", "
					   "args:{} },\n",
				      e->ts / 1000,
				      e->event == PROFSY_TRACE_EVENT_ENTER ? 'B' : 'E',
				      data->name );
		++e;
	}
	fprintf( s, "]\n" );
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
