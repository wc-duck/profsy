#ifndef PROFSY_H_INCLUDED
#define PROFSY_H_INCLUDED

#include <profsy/profsy_internal.h>

#include <stdint.h>

#if !defined( PROFSY_CUSTOM_TICK_FUNC )
	#define PROFSY_CUSTOM_TICK_FUNC profsy_get_tick
#endif

struct profsy_init_params
{
	unsigned int entries_max;
};

typedef struct profsy_ctx* profsy_ctx_t;

struct profsy_scope_data
{
	const char* name;
	uint64_t time;
	uint64_t child_time;
	uint64_t calls;

	// stable time
	// variance

	uint16_t depth;
	uint16_t num_sub_scopes;
};

size_t profsy_calc_ctx_mem_usage( const profsy_init_params* params );

/**
 *
 */
void profsy_init( const profsy_init_params* params, uint8_t* mem );

/**
 *
 */
void profsy_shutdown();

/**
 *
 */
profsy_ctx_t profsy_global_ctx();

// TODO: add functions to alloc scopes outside of macro

/**
 *
 */
int profsy_scope_enter( const char* name );

/**
 *
 */
void profsy_scope_leave( int entry_id, uint64_t time );

/**
 *
 */
void wpprof_swap_frame();

/**
 * max active scopes
 */
unsigned int profsy_max_active_scopes();

/**
 * num active scopes
 */
unsigned int profsy_num_active_scopes();

/**
 * ret hierachy, constant between calls if active scopes has not changed. Can only go up!
 */
void profsy_get_scope_hierarchy( const profsy_scope_data** child_scopes, unsigned int num_child_scopes );

#if defined(__cplusplus)
struct __profsy_scope

{
	int      scope_id;
	uint64_t start;

	__profsy_scope( const char* scope_name )
		: scope_id( profsy_scope_enter( scope_name ) )
		, start( PROFSY_CUSTOM_TICK_FUNC() )
	{}

	~__profsy_scope() { profsy_scope_leave( scope_id, PROFSY_CUSTOM_TICK_FUNC() - start ); }
};

#define   PROFSY_JOIN_MACRO_TOKENS(a,b)     __PROFSY_JOIN_MACRO_TOKENS_DO1(a,b)
#define __PROFSY_JOIN_MACRO_TOKENS_DO1(a,b) __PROFSY_JOIN_MACRO_TOKENS_DO2(a,b)
#define __PROFSY_JOIN_MACRO_TOKENS_DO2(a,b) a##b

#define PROFSY_UNIQUE_SYM( var_name ) PROFSY_JOIN_MACRO_TOKENS( var_name, __LINE__ )

#define PROFSY_SCOPE( name ) __profsy_scope PROFSY_UNIQUE_SYM(__profile_scope_ )( name )

#endif // defined(__cplusplus)

#endif // PROFSY_H_INCLUDED
