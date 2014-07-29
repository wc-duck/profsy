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

#include <gtest/gtest.h>

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

TEST( basic, setup_teardown )
{
	profsy_init_params ip;
	ip.entries_max = 256;

	size_t needed_mem = profsy_calc_ctx_mem_usage( &ip );
	EXPECT_GT( needed_mem, 0u );

	uint8_t* mem = (uint8_t*)malloc( needed_mem );

	profsy_init( &ip, mem );
	
	EXPECT_NE( (profsy_ctx_t)0, profsy_global_ctx() );
	EXPECT_EQ( profsy_global_ctx(), profsy_global_ctx() );

	EXPECT_EQ( mem, profsy_shutdown() );

	EXPECT_EQ( (profsy_ctx_t)0, profsy_global_ctx() );

	free( mem );
}

template<unsigned int ENTRIES_MAX>
struct profsy_ : public ::testing::Test
{
	virtual void SetUp()
	{
		profsy_init_params ip;
		ip.entries_max = ENTRIES_MAX;
		
		size_t needed_mem = profsy_calc_ctx_mem_usage( &ip );
		EXPECT_GT( needed_mem, 0u );

		mem = (uint8_t*)malloc( needed_mem );

		profsy_init( &ip, mem );
		
		EXPECT_NE( (profsy_ctx_t)0, profsy_global_ctx() );
		EXPECT_EQ( profsy_global_ctx(), profsy_global_ctx() );

		EXPECT_EQ( profsy_num_active_scopes(), 2u );
	}

	virtual void TearDown()
	{
		EXPECT_EQ( mem, profsy_shutdown() );
		EXPECT_EQ( (profsy_ctx_t)0, profsy_global_ctx() );

		free( mem );
	}

	uint8_t* mem;
};

typedef profsy_<4>   profsy_4;
typedef profsy_<256> profsy;

TEST_F( profsy, active_scopes_at_init )
{
	// wcprof was setup with max ENTRIES_MAX scopes
	EXPECT_EQ( 258u, profsy_max_active_scopes() );

	// check that only root- and overflow-scope is allocated
	EXPECT_EQ( 2u,   profsy_num_active_scopes() );
}

TEST_F( profsy, simple_scope_alloc )
{
	{
		PROFSY_SCOPE( "test-scope1" );
		SLEEP(1);
	}
	// check that only root, overflow-scope + new one is allocated
	EXPECT_EQ( 3u, profsy_num_active_scopes() );

	// swap frame to calculate "stuff"
	profsy_swap_frame();

	EXPECT_EQ( profsy_num_active_scopes(), 3u );

	// check that scope was entered and left
	const profsy_scope_data* hierarchy[16];

	profsy_get_scope_hierarchy( hierarchy, 16 );

	const profsy_scope_data* root  = hierarchy[0];
	const profsy_scope_data* scope = hierarchy[1];

	EXPECT_STREQ( root->name,           "root" );
	EXPECT_GT   ( root->time,           0u );
	EXPECT_GT   ( root->child_time,     0u );
	EXPECT_EQ   ( root->calls,          1u );
	EXPECT_EQ   ( root->depth,          0u );
	EXPECT_EQ   ( root->num_sub_scopes, 1u );

	EXPECT_STREQ( scope->name,           "test-scope1" );
	EXPECT_GT   ( scope->time,           0u );
	EXPECT_EQ   ( scope->child_time,     0u );
	EXPECT_EQ   ( scope->calls,          1u );
	EXPECT_EQ   ( scope->depth,          1u );
	EXPECT_EQ   ( scope->num_sub_scopes, 0u );

	// EXPECT_EQ( hierarchy[2], (const profsy_scope_data*)0x0 );
}

TEST_F( profsy, deep_hierarchy )
{
	static const char* SCOPE_NAMES[] = {
		"root",
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

	EXPECT_EQ( profsy_num_active_scopes(), 10u );

	// check that scope was entered and left
	const profsy_scope_data* hierarchy[16];
	profsy_get_scope_hierarchy( hierarchy, 16 );

	for( unsigned int i = 0; i < ARRAY_LENGTH(SCOPE_NAMES); ++i )
	{
		const profsy_scope_data* scope = hierarchy[i];
		EXPECT_STREQ( scope->name,           SCOPE_NAMES[i] );
		EXPECT_GT   ( scope->time,           0u );
		EXPECT_EQ   ( scope->calls,          1u );
		EXPECT_EQ   ( scope->depth,          i );
		EXPECT_EQ   ( scope->num_sub_scopes, 8u - i );

		if( i == ARRAY_LENGTH(SCOPE_NAMES) - 1 )
			EXPECT_EQ( scope->child_time, 0u );
		else
			EXPECT_GT( scope->child_time, 0u );
	}
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

TEST_F( profsy, two_paths )
{
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

	EXPECT_EQ( profsy_num_active_scopes(), 8u ); // root + overflow + parent_scope1 + parent_scope2 + 2 * scoped_func2 + 2 * scoped_func1

	const profsy_scope_data* hierarchy[16];
	profsy_get_scope_hierarchy( hierarchy, 16 );

	const profsy_scope_data* s;

	s = hierarchy[0];
	EXPECT_STREQ( s->name,  "root" );
	EXPECT_EQ   ( s->depth, 0u );

	s = hierarchy[1];
	EXPECT_STREQ( s->name,  "parent_scope1" );
	EXPECT_EQ   ( s->depth, 1u );

	s = hierarchy[2];
	EXPECT_STREQ( s->name,  "scoped_func1" );
	EXPECT_EQ   ( s->depth, 2u );

	s = hierarchy[3];
	EXPECT_STREQ( s->name,  "scoped_func2" );
	EXPECT_EQ   ( s->depth, 2u );

	s = hierarchy[4];
	EXPECT_STREQ( s->name,  "parent_scope2" );
	EXPECT_EQ   ( s->depth, 1u );

	s = hierarchy[5];
	EXPECT_STREQ( s->name,  "scoped_func1" );
	EXPECT_EQ   ( s->depth, 2u );

	s = hierarchy[6];
	EXPECT_STREQ( s->name,  "scoped_func2" );
	EXPECT_EQ   ( s->depth, 2u );
}

TEST_F( profsy, find_scope_non_exist )
{
	EXPECT_EQ( -1, profsy_find_scope( "bloo.blaaa" ) ); // non existing scope!
}

TEST_F( profsy_4, out_of_resources_is_tracked )
{
	{
		PROFSY_SCOPE("s1");
		PROFSY_SCOPE("s2");
		PROFSY_SCOPE("s3");
		PROFSY_SCOPE("s4");
		PROFSY_SCOPE("s5"); // this scope will overflow...
		PROFSY_SCOPE("s6"); // this scope will overflow...
	}

	profsy_swap_frame();
	EXPECT_EQ( 6u, profsy_num_active_scopes() ); // max amount of scopes active...

	const profsy_scope_data* hierarchy[16];
	profsy_get_scope_hierarchy( hierarchy, 16 );

	const profsy_scope_data* s;
	s = hierarchy[0];
	EXPECT_STREQ( s->name,  "root" );
	EXPECT_EQ   ( s->calls, 1u );

	s = hierarchy[1]; EXPECT_STREQ( s->name,  "s1" ); EXPECT_EQ( s->calls, 1u );
	s = hierarchy[2]; EXPECT_STREQ( s->name,  "s2" ); EXPECT_EQ( s->calls, 1u );
	s = hierarchy[3]; EXPECT_STREQ( s->name,  "s3" ); EXPECT_EQ( s->calls, 1u );
	s = hierarchy[4]; EXPECT_STREQ( s->name,  "s4" ); EXPECT_EQ( s->calls, 1u );

	s = hierarchy[5];
	EXPECT_STREQ( s->name,  "overflow scope" );
	EXPECT_EQ   ( s->calls, 2u ); // 2 calls, "s3" and "s4"
}

static void test_it( bool do_scope )
{
	if( do_scope )
	{
		PROFSY_SCOPE( "s5" );
	}
}

TEST_F( profsy_4, multi_overflow )
{
	{
		for( int i = 0; i < 2; ++i )
		{
			{ PROFSY_SCOPE("s1"); test_it( i > 0 ); }
			{ PROFSY_SCOPE("s2"); test_it( i > 0 ); }
			{ PROFSY_SCOPE("s3"); test_it( i > 0 ); }
		}
	}

	profsy_swap_frame();
	EXPECT_EQ( 6u, profsy_num_active_scopes() ); // max amount of scopes active...

	const profsy_scope_data* hierarchy[16];
	profsy_get_scope_hierarchy( hierarchy, 16 );

	const profsy_scope_data* s;
	s = hierarchy[0]; EXPECT_STREQ( s->name,  "root" ); EXPECT_EQ( s->calls, 1u );
	s = hierarchy[1]; EXPECT_STREQ( s->name,    "s1" ); EXPECT_EQ( s->calls, 2u );
	s = hierarchy[2]; EXPECT_STREQ( s->name,    "s5" ); EXPECT_EQ( s->calls, 1u );
	s = hierarchy[3]; EXPECT_STREQ( s->name,    "s2" ); EXPECT_EQ( s->calls, 2u );
	s = hierarchy[5]; EXPECT_STREQ( s->name,    "s3" ); EXPECT_EQ( s->calls, 2u );

	EXPECT_EQ( hierarchy[4], hierarchy[6] );
	s = hierarchy[4]; EXPECT_STREQ( s->name,  "overflow scope" ); EXPECT_EQ( s->calls, 2u ); // 2 calls, "s2->s5" and "s3->s5"
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

typedef profsy_<8> trace;
TEST_F( trace, simple )
{
	// TODO: Trace should start on next swap_frame

	static const int NUM_FRAMES = 3;
	profsy_trace_entry trace[256];
	profsy_trace_begin( trace, ARRAY_LENGTH(trace), NUM_FRAMES );

	EXPECT_FALSE( profsy_is_tracing() );
	profsy_swap_frame();

	for( int i = 0; i < NUM_FRAMES; ++i )
	{
		EXPECT_TRUE( profsy_is_tracing() );
		test_frame();
	}
	EXPECT_FALSE( profsy_is_tracing() );

	uint64_t last_ts = 0;

	// check all timestamps
	for( int i = 0; i < NUM_FRAMES; ++i )
	{
		profsy_trace_entry* e = trace + ( i * 8 + 0 );
		EXPECT_GE( e->ts, last_ts );
		last_ts  = e->ts;
	}

	uint16_t root_scope_id = (uint16_t)profsy_find_scope( "" );
	uint16_t s1_scope_id   = (uint16_t)profsy_find_scope( "s1" );
	uint16_t s2_scope_id   = (uint16_t)profsy_find_scope( "s1.s2" );
	uint16_t s3_scope_id   = (uint16_t)profsy_find_scope( "s1.s2.s3" );

	EXPECT_EQ( 0u, root_scope_id );
	EXPECT_EQ( 2u, s1_scope_id );
	EXPECT_EQ( 3u, s2_scope_id );
	EXPECT_EQ( 4u, s3_scope_id );

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
			EXPECT_EQ( e->event, expect[j].event );
			EXPECT_EQ( e->scope, expect[j].scope );
		}
	}

	EXPECT_EQ( trace[ NUM_FRAMES * 8 + 1 ].event, PROFSY_TRACE_EVENT_END ); // check end of trace-marker
}

TEST_F( trace, overflow )
{
	profsy_trace_entry trace[4];
	profsy_trace_begin( trace, ARRAY_LENGTH(trace), 2 );

	EXPECT_FALSE( profsy_is_tracing() );
	profsy_swap_frame();

	EXPECT_TRUE( profsy_is_tracing() );

	test_frame();
	profsy_swap_frame();

	EXPECT_FALSE( profsy_is_tracing() ); // this should be false after one frame since it would have overflowed already

	EXPECT_EQ( PROFSY_TRACE_EVENT_OVERFLOW, trace[ ARRAY_LENGTH(trace) - 1 ].event );
}

int main( int argc, char** argv )
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
