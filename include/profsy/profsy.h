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

static const uint16_t PROFSY_TRACE_EVENT_ENTER    = 0;
static const uint16_t PROFSY_TRACE_EVENT_LEAVE    = 1;
static const uint16_t PROFSY_TRACE_EVENT_END      = 2;
static const uint16_t PROFSY_TRACE_EVENT_OVERFLOW = 3;

struct profsy_trace_entry
{
	uint64_t time_stamp;
	uint16_t event;
	uint16_t scope;
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
uint8_t* profsy_shutdown();

/**
 *
 */
profsy_ctx_t profsy_global_ctx();

// TODO: add functions to alloc scopes outside of macro

/**
 *
 */
int profsy_scope_enter( const char* name, uint64_t time );

/**
 *
 */
void profsy_scope_leave( int scope_id, uint64_t start, uint64_t end );

/**
 *
 */
void profsy_swap_frame();

/**
 * 
 */
void profsy_trace_begin( profsy_trace_entry* entries, 
						 unsigned int        num_entries, 
						 unsigned int        frames_to_capture );

/**
 *
 */
bool profsy_is_tracing();

/**
 * max active scopes
 */
unsigned int profsy_max_active_scopes();

/**
 * num active scopes
 */
unsigned int profsy_num_active_scopes();

/**
 *
 */
int profsy_find_scope( const char* scope_path );

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
		: start( PROFSY_CUSTOM_TICK_FUNC() )
	{
		scope_id = profsy_scope_enter( scope_name, start );
	}

	~__profsy_scope() { profsy_scope_leave( scope_id, start, PROFSY_CUSTOM_TICK_FUNC() ); }
};

#define   PROFSY_JOIN_MACRO_TOKENS(a,b)     __PROFSY_JOIN_MACRO_TOKENS_DO1(a,b)
#define __PROFSY_JOIN_MACRO_TOKENS_DO1(a,b) __PROFSY_JOIN_MACRO_TOKENS_DO2(a,b)
#define __PROFSY_JOIN_MACRO_TOKENS_DO2(a,b) a##b

#define PROFSY_UNIQUE_SYM( var_name ) PROFSY_JOIN_MACRO_TOKENS( var_name, __LINE__ )

#define PROFSY_SCOPE( name ) __profsy_scope PROFSY_UNIQUE_SYM(__profile_scope_ )( name )

#endif // defined(__cplusplus)

#endif // PROFSY_H_INCLUDED
