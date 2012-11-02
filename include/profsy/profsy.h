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

#ifndef PROFSY_H_INCLUDED
#define PROFSY_H_INCLUDED

#include <profsy/profsy_internal.h>

#include <stdint.h>

#if !defined( PROFSY_CUSTOM_TICK_FUNC )
	#define PROFSY_CUSTOM_TICK_FUNC profsy_get_tick
#endif

static const uint16_t PROFSY_TRACE_EVENT_ENTER    = 0;
static const uint16_t PROFSY_TRACE_EVENT_LEAVE    = 1;
static const uint16_t PROFSY_TRACE_EVENT_END      = 2;
static const uint16_t PROFSY_TRACE_EVENT_OVERFLOW = 3;

/**
 * parameters for initializing profsy
 */
struct profsy_init_params
{
	unsigned int entries_max; //< maximum amount of entries that can be allocated by profsy, all other scopes will get registered as "overflow"
};

/**
 * structure describing on entry in a stream of trace-events
 */
struct profsy_trace_entry
{
	uint64_t ts;    //< timestamp when event occurred.
	uint16_t event; //< event that occured.
	uint16_t scope; //< the scope that was involved in the event.
};

/**
 * 
 */
typedef struct profsy_ctx* profsy_ctx_t;

/**
 * structure describing state of a scope that was measured between
 * the last two profsy_swap_frame()
 */
struct profsy_scope_data
{
	const char* name;    //< name of scope
	uint64_t time;       //< time spent in scope
	uint64_t child_time; //< time spent in child-scopes
	uint64_t calls;      //< number of calls made to this scopes

	// stable time
	// variance

	uint16_t depth;          // depth of scope in call-hierarchy
	uint16_t num_sub_scopes; // number of child-scopes
};

/**
 * calculate the amount of memory needed by profsy_init to initialize profsy.
 * @param parmas initialization-parameters that will also be sent to profsy_init
 * @return the amount of memory needed by profsy_init
 */
size_t profsy_calc_ctx_mem_usage( const profsy_init_params* params );

/**
 * initializes profsy.
 * @param params initialization-parameters
 * @param mem a pointer to memory that will be used by profsy until profsy_shutdown().
 *            this is expected to be greater or equal in size to the return-value of
 *            profsy_calc_ctx_mem_usage() with the same init-params.
 */
void profsy_init( const profsy_init_params* params, uint8_t* mem );

/**
 * shutdown profsy and stop using its assigned memory
 * @return the memory-buffer earlier used by profsy, should be the same as the mem-parameter to profsy_init()
 */
uint8_t* profsy_shutdown();

/**
 * @return a handle to the global profsy-context
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
 * mark end of frame and start of the next one.
 * in this call profsy will reset all counters, start/stop-tracing etc.
 */
void profsy_swap_frame();

/**
 * tell profsy to start a trace at the next call to profsy_swap_frame()
 * @note profsy will assume that the entries-buffer will be valid until profsy_is_tracing()
 * returns false.
 * @param entries buffer where trace-result will be reported. If buffer is filled, last element
 *                will be marked as PROFSY_TRACE_EVENT_OVERFLOW
 * @param num_entries size of entries-buffer
 * @param frames_to_capture the number of profsy_swap_frame()-calls to capture trace for.
 */
void profsy_trace_begin( profsy_trace_entry* entries, 
						 unsigned int        num_entries, 
						 unsigned int        frames_to_capture );

/**
 * return the status of tracing.
 * profsy will assume that the entries-buffer sent to profsy_trace_begin() is valid until this 
 * return false.
 * @return trace status
 */
bool profsy_is_tracing();

/**
 * @return max active scopes
 */
unsigned int profsy_max_active_scopes();

/**
 * @return num active scopes
 */
unsigned int profsy_num_active_scopes();

/**
 * return the index of a path with a specific path in the call hierarchy.
 * @example profsy_find_scope( "" ) -> will return index of "root"
 * @example profsy_find_scope( "scope1" ) -> will return index of "scope1"
 * @example profsy_find_scope( "scope1/scope2" ) -> will return index of "scope2" if called under "scope1"
 * @return the index of scope with path
 */
int profsy_find_scope( const char* scope_path );

/**
 * @param scope_id id of scope to return data for
 * @return scope-data for scope with specific id
 */
profsy_scope_data* profsy_get_scope_data( int scope_id );

/**
 * used to generate a hierarchy-structure of the nodes for output. The result is a list that can be printed
 * from top to bottom and get a call-graph of all registered scopes.
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

/**
 * macro to define a scope within c++-code.
 * @param name name of scope as a constant string, profsy will assue that the name is valid until profsy_shutdown() is called.
 */
#define PROFSY_SCOPE( name ) __profsy_scope __PROFSY_UNIQUE_SYM(__profile_scope_ )( name )

#endif // defined(__cplusplus)

#endif // PROFSY_H_INCLUDED
