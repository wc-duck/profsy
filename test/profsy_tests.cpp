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

#include "greatest.h"
#include <profsy/profsy.h>

#include <malloc.h>

#define ARRAY_LENGTH(a) (sizeof(a)/sizeof(a[0]))

#if defined( _MSC_VER )
	#include <windows.h>
	#define SLEEP( ms ) SleepEx( ms, false )
#else
	#include <unistd.h>
	#define SLEEP( ms ) usleep( ms )
#endif

TEST profsy_setup_teardown()
{
	profsy_init_params ip;
	ip.threads_max = 16;
	ip.entries_max = 256;

	size_t needed_mem = profsy_calc_ctx_mem_usage( &ip );
	ASSERT( needed_mem > 0u );

	uint8_t* mem = (uint8_t*)malloc( needed_mem );

	profsy_init( &ip, mem );
	
	ASSERT( profsy_global_ctx() != 0x0 );
	ASSERT_EQ( profsy_global_ctx(), profsy_global_ctx() );
	ASSERT_EQ( mem, profsy_shutdown() );
	ASSERT_EQ( (profsy_ctx_t)0, profsy_global_ctx() );

	free( mem );
	return 0;
}

struct profsy_setup
{
	uint8_t* mem;

	profsy_setup( unsigned int max_entries )
		: mem( 0x0 )
	{
		if( setup( max_entries ) != 0 )
		{
			free( mem );
			mem = 0x0;
		}
	}

	~profsy_setup()
	{
		free( mem );
	}

	int setup( unsigned int max_entries )
	{
		profsy_init_params ip;
		ip.threads_max = 16;
		ip.entries_max = max_entries;

		size_t needed_mem = profsy_calc_ctx_mem_usage( &ip );
		ASSERT( needed_mem > 0 );

		mem = (uint8_t*)malloc( needed_mem );
		profsy_init( &ip, mem );

		ASSERT( profsy_global_ctx() != 0x0 );
		ASSERT_EQ( profsy_global_ctx(), profsy_global_ctx() );
		ASSERT_EQ( profsy_num_active_scopes(), 2u );
		return 0;
	}
};

TEST profsy_active_scopes_at_init()
{
	profsy_setup s( 256 );
	ASSERT( s.mem != 0x0 );

	// wcprof was setup with max ENTRIES_MAX scopes
	ASSERT_EQ( 258u, profsy_max_active_scopes() );

	// check that only root- and overflow-scope is allocated
	ASSERT_EQ( 2u,   profsy_num_active_scopes() );
	return 0;
}

TEST profsy_simple_scope_alloc()
{
	profsy_setup s( 256 );
	ASSERT( s.mem != 0x0 );

	{
		PROFSY_SCOPE( "test-scope1" );
		SLEEP(1);
	}
	// check that only root, overflow-scope + new one is allocated
	ASSERT_EQ( 3u, profsy_num_active_scopes() );

	// swap frame to calculate "stuff"
	profsy_swap_frame();

	ASSERT_EQ( profsy_num_active_scopes(), 3u );

	// check that scope was entered and left
	const profsy_scope_data* hierarchy[16];

	profsy_get_scope_hierarchy( hierarchy, 16 );

	const profsy_scope_data* root  = hierarchy[0];
	const profsy_scope_data* scope = hierarchy[1];

	ASSERT_STR_EQ( root->name, "main" );
	ASSERT      ( root->time > 0u );
	ASSERT      ( root->child_time > 0u );
	ASSERT_EQ   ( root->calls,          1u );
	ASSERT_EQ   ( root->depth,          0u );
	ASSERT_EQ   ( root->num_sub_scopes, 1u );

	ASSERT_STR_EQ( scope->name, "test-scope1" );
	ASSERT      ( scope->time > 0u );
	ASSERT_EQ   ( scope->child_time,     0u );
	ASSERT_EQ   ( scope->calls,          1u );
	ASSERT_EQ   ( scope->depth,          1u );
	ASSERT_EQ   ( scope->num_sub_scopes, 0u );

	// ASSERT_EQ( hierarchy[2], (const profsy_scope_data*)0x0 );
	return 0;
}

TEST profsy_deep_hierarchy()
{
	profsy_setup s( 256 );
	ASSERT( s.mem != 0x0 );

	static const char* SCOPE_NAMES[] = {
		"main",
		"test-scope1", "test-scope2", "test-scope3", "test-scope4",
		"test-scope5", "test-scope6", "test-scope7", "test-scope8"
	};

	{
		PROFSY_SCOPE( SCOPE_NAMES[1] ); SLEEP(1);
		PROFSY_SCOPE( SCOPE_NAMES[2] ); SLEEP(1);
		PROFSY_SCOPE( SCOPE_NAMES[3] ); SLEEP(1);
		PROFSY_SCOPE( SCOPE_NAMES[4] ); SLEEP(1);
		PROFSY_SCOPE( SCOPE_NAMES[5] ); SLEEP(1);
		PROFSY_SCOPE( SCOPE_NAMES[6] ); SLEEP(1);
		PROFSY_SCOPE( SCOPE_NAMES[7] ); SLEEP(1);
		PROFSY_SCOPE( SCOPE_NAMES[8] ); SLEEP(1);
	}

	// swap frame to calculate "stuff"
	profsy_swap_frame();

	ASSERT_EQ( profsy_num_active_scopes(), 10u );

	// check that scope was entered and left
	const profsy_scope_data* hierarchy[16];
	profsy_get_scope_hierarchy( hierarchy, 16 );

	for( unsigned int i = 0; i < ARRAY_LENGTH(SCOPE_NAMES); ++i )
	{
		const profsy_scope_data* scope = hierarchy[i];
		ASSERT_STR_EQ( scope->name,           SCOPE_NAMES[i] );
		ASSERT      ( scope->time > 0u );
		ASSERT_EQ   ( scope->calls,          1u );
		ASSERT_EQ   ( scope->depth,          i );
		ASSERT_EQ   ( scope->num_sub_scopes, 8u - i );

		if( i == ARRAY_LENGTH(SCOPE_NAMES) - 1 )
			ASSERT_EQ( scope->child_time, 0u );
		else
			ASSERT( scope->child_time > 0u );
	}
	return 0;
}

static void scoped_func1()
{
	PROFSY_SCOPE( "scoped_func1" );
	SLEEP(1);
}

static void scoped_func2()
{
	PROFSY_SCOPE( "scoped_func2" );
	SLEEP(1);
}

TEST profsy_two_paths()
{
	profsy_setup st( 256 );
	ASSERT( st.mem != 0x0 );

	{
		{
			PROFSY_SCOPE( "parent_scope1" );
			scoped_func1();
			scoped_func2();
		}

		{
			PROFSY_SCOPE( "parent_scope2" );
			scoped_func1();
			scoped_func2();
		}
	}

	// swap frame to calculate "stuff"
	profsy_swap_frame();

	ASSERT_EQ( profsy_num_active_scopes(), 8u ); // root + overflow + parent_scope1 + parent_scope2 + 2 * scoped_func2 + 2 * scoped_func1

	const profsy_scope_data* hierarchy[16];
	profsy_get_scope_hierarchy( hierarchy, 16 );

	const profsy_scope_data* s;

	s = hierarchy[0];
	ASSERT_STR_EQ( s->name,  "main" );
	ASSERT_EQ    ( s->depth, 0u );

	s = hierarchy[1];
	ASSERT_STR_EQ( s->name,  "parent_scope1" );
	ASSERT_EQ    ( s->depth, 1u );

	s = hierarchy[2];
	ASSERT_STR_EQ( s->name,  "scoped_func1" );
	ASSERT_EQ    ( s->depth, 2u );

	s = hierarchy[3];
	ASSERT_STR_EQ( s->name,  "scoped_func2" );
	ASSERT_EQ    ( s->depth, 2u );

	s = hierarchy[4];
	ASSERT_STR_EQ( s->name,  "parent_scope2" );
	ASSERT_EQ    ( s->depth, 1u );

	s = hierarchy[5];
	ASSERT_STR_EQ( s->name,  "scoped_func1" );
	ASSERT_EQ    ( s->depth, 2u );

	s = hierarchy[6];
	ASSERT_STR_EQ( s->name,  "scoped_func2" );
	ASSERT_EQ    ( s->depth, 2u );
	return 0;
}

TEST profsy_find_scope_non_exist()
{
	profsy_setup s( 256 );
	ASSERT( s.mem != 0x0 );
	ASSERT_EQ( -1, profsy_find_scope( "bloo.blaaa" ) ); // non existing scope!
	return 0;
}

TEST profsy_out_of_resources_is_tracked()
{
	profsy_setup st( 4 );
	ASSERT( st.mem != 0x0 );
	{
		PROFSY_SCOPE("s1");
		PROFSY_SCOPE("s2");
		PROFSY_SCOPE("s3");
		PROFSY_SCOPE("s4");
		PROFSY_SCOPE("s5"); // this scope will overflow...
		PROFSY_SCOPE("s6"); // this scope will overflow...
	}

	profsy_swap_frame();
	ASSERT_EQ( 6u, profsy_num_active_scopes() ); // max amount of scopes active...

	const profsy_scope_data* hierarchy[16];
	profsy_get_scope_hierarchy( hierarchy, 16 );

	const profsy_scope_data* s;
	s = hierarchy[0];
	ASSERT_STR_EQ( s->name,  "main" );
	ASSERT_EQ    ( s->calls, 1u );

	s = hierarchy[1]; ASSERT_STR_EQ( s->name,  "s1" ); ASSERT_EQ( s->calls, 1u );
	s = hierarchy[2]; ASSERT_STR_EQ( s->name,  "s2" ); ASSERT_EQ( s->calls, 1u );
	s = hierarchy[3]; ASSERT_STR_EQ( s->name,  "s3" ); ASSERT_EQ( s->calls, 1u );
	s = hierarchy[4]; ASSERT_STR_EQ( s->name,  "s4" ); ASSERT_EQ( s->calls, 1u );

	s = hierarchy[5];
	ASSERT_STR_EQ( s->name,  "overflow scope" );
	ASSERT_EQ    ( s->calls, 2u ); // 2 calls, "s3" and "s4"
	return 0;
}

static void test_it( bool do_scope )
{
	if( do_scope )
	{
		PROFSY_SCOPE( "s5" );
	}
}

TEST profsy_multi_overflow()
{
	profsy_setup st( 4 );
	ASSERT( st.mem != 0x0 );
	{
		for( int i = 0; i < 2; ++i )
		{
			{ PROFSY_SCOPE("s1"); test_it( i > 0 ); }
			{ PROFSY_SCOPE("s2"); test_it( i > 0 ); }
			{ PROFSY_SCOPE("s3"); test_it( i > 0 ); }
		}
	}

	profsy_swap_frame();
	ASSERT_EQ( 6u, profsy_num_active_scopes() ); // max amount of scopes active...

	const profsy_scope_data* hierarchy[16];
	profsy_get_scope_hierarchy( hierarchy, 16 );

	const profsy_scope_data* s;
	s = hierarchy[0]; ASSERT_STR_EQ( s->name, "main" ); ASSERT_EQ( s->calls, 1u );
	s = hierarchy[1]; ASSERT_STR_EQ( s->name,   "s1" ); ASSERT_EQ( s->calls, 2u );
	s = hierarchy[2]; ASSERT_STR_EQ( s->name,   "s5" ); ASSERT_EQ( s->calls, 1u );
	s = hierarchy[3]; ASSERT_STR_EQ( s->name,   "s2" ); ASSERT_EQ( s->calls, 2u );
	s = hierarchy[5]; ASSERT_STR_EQ( s->name,   "s3" ); ASSERT_EQ( s->calls, 2u );

	ASSERT_EQ( hierarchy[4], hierarchy[6] );
	s = hierarchy[4]; ASSERT_STR_EQ( s->name,  "overflow scope" ); ASSERT_EQ( s->calls, 2u ); // 2 calls, "s2->s5" and "s3->s5"
	return 0;
}

static void test_frame()
{
	{
		PROFSY_SCOPE("s1");
		SLEEP(10);

		{
			PROFSY_SCOPE("s2");
			SLEEP(10);

			{
				PROFSY_SCOPE("s3");
				SLEEP(10);
			}
		}
	}

	profsy_swap_frame();
}

TEST trace_simple()
{
	profsy_setup st( 8 );
	ASSERT( st.mem != 0x0 );
	// TODO: Trace should start on next swap_frame

	static const int NUM_FRAMES = 3;
	profsy_trace_entry trace[256];
	profsy_trace_begin( trace, (unsigned int)ARRAY_LENGTH(trace), NUM_FRAMES );

	ASSERT_FALSE( profsy_is_tracing() );
	profsy_swap_frame();

	for( int i = 0; i < NUM_FRAMES; ++i )
	{
		ASSERT( profsy_is_tracing() );
		test_frame();
	}
	ASSERT_FALSE( profsy_is_tracing() );

	uint64_t last_ts = 0;

	// check all timestamps
	for( int i = 0; i < NUM_FRAMES; ++i )
	{
		profsy_trace_entry* e = trace + ( i * 8 + 0 );
		ASSERT( e->ts > last_ts );
		last_ts  = e->ts;
	}

	uint16_t root_scope_id = (uint16_t)profsy_find_scope( "" );
	uint16_t s1_scope_id   = (uint16_t)profsy_find_scope( "s1" );
	uint16_t s2_scope_id   = (uint16_t)profsy_find_scope( "s1.s2" );
	uint16_t s3_scope_id   = (uint16_t)profsy_find_scope( "s1.s2.s3" );

	ASSERT_EQ( 0u, root_scope_id );
	ASSERT_EQ( 2u, s1_scope_id );
	ASSERT_EQ( 3u, s2_scope_id );
	ASSERT_EQ( 4u, s3_scope_id );

	struct
	{
		uint16_t event;
		uint16_t scope;
	} expect[] = {
		{ PROFSY_TRACE_EVENT_ENTER, root_scope_id },
		{ PROFSY_TRACE_EVENT_ENTER, s1_scope_id   },
		{ PROFSY_TRACE_EVENT_ENTER, s2_scope_id   },
		{ PROFSY_TRACE_EVENT_ENTER, s3_scope_id   },

		{ PROFSY_TRACE_EVENT_LEAVE, s3_scope_id   },
		{ PROFSY_TRACE_EVENT_LEAVE, s2_scope_id   },
		{ PROFSY_TRACE_EVENT_LEAVE, s1_scope_id   },
		{ PROFSY_TRACE_EVENT_LEAVE, root_scope_id },
	};

	for( int i = 0; i < NUM_FRAMES; ++i )
	{
		for( int j = 0; j < (int)ARRAY_LENGTH( expect ); ++j )
		{
			profsy_trace_entry* e = trace + ( i * 8 + j );
			ASSERT_EQ( e->event, expect[j].event );
			ASSERT_EQ( e->scope, expect[j].scope );
		}
	}

	ASSERT_EQ( trace[ NUM_FRAMES * 8 + 1 ].event, PROFSY_TRACE_EVENT_END ); // check end of trace-marker

	return 0;
}

TEST trace_overflow()
{
	profsy_setup st( 8 );
	ASSERT( st.mem != 0x0 );

	profsy_trace_entry trace[4];
	profsy_trace_begin( trace, (unsigned int)ARRAY_LENGTH(trace), 2 );

	ASSERT_FALSE( profsy_is_tracing() );
	profsy_swap_frame();

	ASSERT( profsy_is_tracing() );

	test_frame();
	profsy_swap_frame();

	ASSERT_FALSE( profsy_is_tracing() ); // this should be false after one frame since it would have overflowed already

	ASSERT_EQ( PROFSY_TRACE_EVENT_OVERFLOW, trace[ ARRAY_LENGTH(trace) - 1 ].event );
	return 0;
}

GREATEST_SUITE( profsy )
{
	RUN_TEST( profsy_setup_teardown );

	RUN_TEST( profsy_active_scopes_at_init );
	RUN_TEST( profsy_simple_scope_alloc );
	RUN_TEST( profsy_deep_hierarchy );
	RUN_TEST( profsy_two_paths );
	RUN_TEST( profsy_find_scope_non_exist );
	RUN_TEST( profsy_out_of_resources_is_tracked );
	RUN_TEST( profsy_multi_overflow );
}

GREATEST_SUITE( trace )
{
	RUN_TEST( trace_simple );
	RUN_TEST( trace_overflow );
}

GREATEST_MAIN_DEFS();

int main( int argc, char **argv )
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE( profsy );
    RUN_SUITE( trace );
    GREATEST_MAIN_END();
}
