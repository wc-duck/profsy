#include <gtest/gtest.h>

#include <profsy/profsy.h>

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

	profsy_shutdown();

	EXPECT_EQ( (profsy_ctx_t)0, profsy_global_ctx() );

	free( mem );
}

struct profsy : public ::testing::Test
{
	static const unsigned int ENTRIES_MAX;

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

		EXPECT_EQ( profsy_num_active_scopes(), 1u );
	}

	virtual void TearDown()
	{
		profsy_shutdown();

		EXPECT_EQ( (profsy_ctx_t)0, profsy_global_ctx() );

		free( mem );
	}

	uint8_t* mem;
};

const unsigned int profsy::ENTRIES_MAX = 256;

TEST_F( profsy, active_scopes_at_init )
{
	// wcprof was setup with max ENTRIES_MAX scopes
	EXPECT_EQ( ENTRIES_MAX, profsy_max_active_scopes() );

	// check that only root-scope is allocated
	EXPECT_EQ( 1,           profsy_num_active_scopes() );
}

TEST_F( profsy, simple_scope_alloc )
{
	{
		PROFSY_SCOPE( "test-scope1" );
		for( int i = 0; i < 1000; ++i ); // replace with sleep()
	}
	// check that only root-scope + new one is allocated
	EXPECT_EQ( 2, profsy_num_active_scopes() );

	// swap frame to calculate "stuff"
	wpprof_swap_frame();

	EXPECT_EQ( profsy_num_active_scopes(), 2u );

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
		PROFSY_SCOPE( SCOPE_NAMES[1] ); for( int i = 0; i < 1000; ++i ); // replace with sleep()
		PROFSY_SCOPE( SCOPE_NAMES[2] ); for( int i = 0; i < 1000; ++i ); // replace with sleep()
		PROFSY_SCOPE( SCOPE_NAMES[3] ); for( int i = 0; i < 1000; ++i ); // replace with sleep()
		PROFSY_SCOPE( SCOPE_NAMES[4] ); for( int i = 0; i < 1000; ++i ); // replace with sleep()
		PROFSY_SCOPE( SCOPE_NAMES[5] ); for( int i = 0; i < 1000; ++i ); // replace with sleep()
		PROFSY_SCOPE( SCOPE_NAMES[6] ); for( int i = 0; i < 1000; ++i ); // replace with sleep()
		PROFSY_SCOPE( SCOPE_NAMES[7] ); for( int i = 0; i < 1000; ++i ); // replace with sleep()
		PROFSY_SCOPE( SCOPE_NAMES[8] ); for( int i = 0; i < 1000; ++i ); // replace with sleep()
	}

	// swap frame to calculate "stuff"
	wpprof_swap_frame();

	EXPECT_EQ( profsy_num_active_scopes(), 9u );

	// check that scope was entered and left
	const profsy_scope_data* hierarchy[16];
	profsy_get_scope_hierarchy( hierarchy, 16 );

	for( unsigned int i = 0; i < 9; ++i )
	{
		const profsy_scope_data* scope = hierarchy[i];
		EXPECT_STREQ( scope->name,           SCOPE_NAMES[i] );
		EXPECT_GT   ( scope->time,           0u );
		EXPECT_EQ   ( scope->calls,          1u );
		EXPECT_EQ   ( scope->depth,          i );
		EXPECT_EQ   ( scope->num_sub_scopes, 8u - i );

		if( i == 8 )
			EXPECT_EQ( scope->child_time, 0u );
		else
			EXPECT_GT( scope->child_time, 0u );
	}
}

static void scoped_func1()
{
	PROFSY_SCOPE( "scoped_func1" );
	for( int i = 0; i < 1000; ++i ); // replace with sleep()
}

static void scoped_func2()
{
	PROFSY_SCOPE( "scoped_func2" );
	for( int i = 0; i < 1000; ++i ); // replace with sleep()
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
	wpprof_swap_frame();

	EXPECT_EQ( profsy_num_active_scopes(), 7u ); // root + parent_scope1 + parent_scope2 + 2 * scoped_func2 + 2 * scoped_func1

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


int main( int argc, char** argv )
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
