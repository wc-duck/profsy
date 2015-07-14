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

#include <profsy/profsy.h>

#include <string.h>

#define ALIGN_UP( in, alignment ) (size_t)( ( (size_t)(in) + (size_t)(alignment) - 1 ) & ~( (size_t)(alignment) - 1 ) )

// TODO: currently thread one overflow scope per thread, do we need that or could we have one that is
//       non threadsafe and registers "as good as it can"?
const unsigned int PROFSY_BUILTIN_SCOPES = 2; // "<thread-name>"- and "overflow"-scopes

// TODO: replace ptrs in profsy_entry and profsy_thread to uint16 to save lots of bytes!

struct profsy_entry
{
	profsy_scope_data data;

	uint64_t time;
	uint64_t child_time;
	uint64_t calls;

	profsy_entry* parent;
	profsy_entry* children;
	profsy_entry* next_child;
};


struct profsy_thread
{
	const char*   name;
	profsy_entry* root;     // root scope for this thread.
	profsy_entry* overflow; // overflow scope for this thread.
	profsy_entry* current;  // current scope for this thread.
};

struct profsy_ctx
{
	uint8_t* mem;

	profsy_thread* threads;
	int threads_used;
	int threads_max;

	profsy_entry* entries;
	unsigned int  entries_used;
	unsigned int  entries_max;

	uint64_t frame_start;

	profsy_trace_entry* trace_to_activate;
	profsy_trace_entry* active_trace;
	unsigned int max_active_trace;
	unsigned int num_active_trace;
	unsigned int active_trace_frame;
	unsigned int num_trace_frames;
};

static profsy_ctx* g_profsy_ctx;

static profsy_entry* profsy_alloc_entry( profsy_ctx_t ctx, int thread_id, const char* name )
{
	if( ctx->entries_used >= ctx->entries_max )
		return ctx->threads[thread_id].overflow;
	
	profsy_entry* entry = ctx->entries + ctx->entries_used++;
	
	entry->parent              = 0x0;
	entry->children            = 0x0;
	entry->next_child          = 0x0;
	entry->calls               = 0;
	entry->time                = 0;
	
	entry->data.name           = name;
	entry->data.depth          = 0;
	entry->data.num_sub_scopes = 0;
	
	return entry;
}

static int profsy_alloc_thread_ctx( profsy_ctx_t ctx, const char* thread_name )
{
	// ... !!! some lock here !!! ...
	int thread_id = ctx->threads_used++;
	if( thread_id >= ctx->threads_max )
		return -1;

	// ... !!! embed some context id in threadid to be able to detect new ctx !!! ...
	profsy_thread* thread = ctx->threads + thread_id;
	thread->name     = thread_name;
	thread->root     = profsy_alloc_entry( ctx, thread_id, thread_name );
	thread->overflow = profsy_alloc_entry( ctx, thread_id, "overflow scope" );
	thread->overflow->parent = thread->root;
	thread->current = thread->root;
	return thread_id;
}

size_t profsy_calc_ctx_mem_usage( const profsy_init_params* params )
{
	size_t needed_mem = 16; // we add 16 bytes to be able to 16-align it
	
	needed_mem += sizeof( profsy_ctx );
	needed_mem  = ALIGN_UP( needed_mem, 16 );
	needed_mem += ( params->threads_max * sizeof( profsy_thread ) );
	needed_mem  = ALIGN_UP( needed_mem, 16 );
	needed_mem += ( params->entries_max + PROFSY_BUILTIN_SCOPES * params->threads_max ) * sizeof( profsy_entry ); // + 2 for "root" and "overflow"
	
	return needed_mem;
}

void profsy_init( const profsy_init_params* params, uint8_t* in_mem )
{
	// TODO: check that profiler is not already initialized

	// Align memory!
	uint8_t* mem = (uint8_t*)ALIGN_UP( in_mem, 16 );
	
	profsy_ctx* ctx = ( profsy_ctx* )mem;
	ctx->mem          = in_mem;

	mem = (uint8_t*)ALIGN_UP( mem + sizeof( profsy_ctx ), 16 );
	ctx->threads      = ( profsy_thread* )mem;
	ctx->threads_used = 0;
	ctx->threads_max  = params->threads_max;

	mem = (uint8_t*)ALIGN_UP( mem + params->threads_max * sizeof( profsy_thread ), 16 );
	ctx->entries      = ( profsy_entry* )mem;
	ctx->entries_used = 0;
	ctx->entries_max  = params->entries_max + PROFSY_BUILTIN_SCOPES;
	
	memset( ctx->threads, 0x0, sizeof( profsy_thread ) * ctx->threads_max );
	memset( ctx->entries, 0x0, sizeof( profsy_entry )  * ctx->entries_max );

	profsy_alloc_thread_ctx( ctx, "main" );

	ctx->active_trace       = 0x0;
	ctx->trace_to_activate  = 0x0;
	ctx->max_active_trace   = 0;
	ctx->num_active_trace   = 0;
	ctx->active_trace_frame = 0;
	ctx->num_trace_frames   = 0;

	ctx->frame_start = PROFSY_CUSTOM_TICK_FUNC();
	
	g_profsy_ctx = ctx;
}

uint8_t* profsy_shutdown()
{
	uint8_t* mem = g_profsy_ctx->mem;
	g_profsy_ctx = 0x0;
	return mem;
}

profsy_ctx_t profsy_global_ctx()
{
	return g_profsy_ctx;
}

int profsy_create_thread_ctx( const char* thread_name )
{
	profsy_ctx_t ctx = g_profsy_ctx;
	if( ctx == 0x0 )
		return -1;

	// ... !!! some lock here !!! ...
	int thread_id = ctx->threads_used++;
	if( thread_id >= ctx->threads_max )
		return -1;

	profsy_thread* thread = ctx->threads + thread_id;
	thread->name     = thread_name;
	thread->root     = profsy_alloc_entry( ctx, thread_id, thread_name );
	thread->overflow = profsy_alloc_entry( ctx, thread_id, "overflow scope" );
	thread->overflow->parent = thread->root;
	thread->current = thread->root;
	return thread_id;
}

// add functions to alloc scopes outside of macro

static profsy_entry* profsy_get_child_scope( profsy_ctx* ctx, int thread_id, profsy_entry* parent, const char* name )
{
	profsy_entry* e = parent->children;
	while( e > ctx->threads[thread_id].overflow && e->data.name != name )
		e = e->next_child;
	return e;
}

static void profsy_trace_add( profsy_ctx* ctx, uint64_t tick, uint16_t event, uint16_t scope_id )
{
	if( ctx->active_trace == 0x0 )
		return; // ... no active trace!

	unsigned int next_trace = ctx->num_active_trace++;
	if( next_trace >= ctx->max_active_trace )
		return; // ... no entries in trace-buffer left

	profsy_trace_entry* te = ctx->active_trace + next_trace;
	te->ts    = tick;
	te->event = event;
	te->scope = scope_id;
}

static void profsy_trace_close( profsy_ctx* ctx, uint64_t tick )
{
	unsigned int next_trace = ctx->num_active_trace++;
	uint16_t event = PROFSY_TRACE_EVENT_END;
	if( next_trace >= ctx->max_active_trace )
	{
		next_trace = ctx->max_active_trace - 1;
		event = PROFSY_TRACE_EVENT_OVERFLOW;
	}

	profsy_trace_entry* te = ctx->active_trace + next_trace;
	te->ts    = tick;
	te->event = event;
	te->scope = (uint16_t)0;


	ctx->active_trace = 0x0; // Trace is now done!
}

int profsy_scope_enter_thread( int thread_id, const char* name, uint64_t tick )
{
	profsy_ctx_t ctx = g_profsy_ctx;

	if( ctx == 0x0 )
		return -1;

	profsy_entry* current  = ctx->threads[thread_id].current;
	profsy_entry* overflow = ctx->threads[thread_id].overflow;

	// search for scope in current open scope
	profsy_entry* e = profsy_get_child_scope( ctx, thread_id, current, name );

	// not found!
	if( e == 0x0 )
	{
		// TODO: take lock here if multiple threads are to be supported

		// search again since some other thread might have created the scope!
		e = profsy_get_child_scope( ctx, thread_id, current, name );
		if( e == 0x0 )
		{
			// alloc scope and link
			e = profsy_alloc_entry( ctx, thread_id, name );

			// insert at tail to get order where scopes was registered.
			if( current->children )
			{
				profsy_entry* next = current->children;
				while( next->next_child )
					next = next->next_child;
				next->next_child = e;
			}
			else
			{
				if( current != overflow )
					current->children = e;
			}

			if( e != overflow )
			{
				e->data.depth = (uint16_t)(current->data.depth + 1);
				e->parent = current;

				profsy_entry* parent = e->parent;
				while( parent != 0 )
				{
					parent->data.num_sub_scopes++;
					parent = parent->parent;
				}
			}
		}
	}

	// count stuff
	if( e != overflow )
		ctx->threads[thread_id].current = e;

	int scope_id = (int)(e - ctx->entries);

	// ... add trace if tracing
	profsy_trace_add( ctx, tick, PROFSY_TRACE_EVENT_ENTER, (uint16_t)scope_id );

	return scope_id;
}

void profsy_scope_leave_thread( int thread_id, int scope_id, uint64_t start, uint64_t end )
{
	profsy_ctx_t ctx = g_profsy_ctx;

	if( ctx == 0x0 )
		return;

	profsy_entry* entry = ctx->entries + scope_id;

	uint64_t diff = end - start;

	entry->calls += 1;
	entry->time  += diff;
	entry->parent->child_time += diff;

	ctx->threads[thread_id].current = entry->parent;

	// ... add trace if tracing
	profsy_trace_add( ctx, end, PROFSY_TRACE_EVENT_LEAVE, (uint16_t)scope_id );
}

int profsy_scope_enter( const char* name, uint64_t tick )
{
	int thread_id = 0; // fetch this from a tls.
	return profsy_scope_enter_thread( thread_id, name, tick );
}

void profsy_scope_leave( int scope_id, uint64_t start, uint64_t end )
{
	int thread_id = 0; // fetch this from a tls.
	profsy_scope_leave_thread( thread_id, scope_id, start, end );
}

void profsy_swap_frame()
{
	profsy_ctx_t ctx = g_profsy_ctx;
	
	if( ctx == 0x0 )
		return;

	for( int i = 0; i < ctx->threads_used; ++i )
	{
		ctx->threads[i].root->calls = 1; // TODO: TOK-Hack root to be one call
		ctx->threads[i].root->time  = PROFSY_CUSTOM_TICK_FUNC() - ctx->frame_start; // TODO: TOK-Hack root to be one call
	}

	for( unsigned int i = 0; i < ctx->entries_used; ++i )
	{
		profsy_entry* e    = ctx->entries + i;
		e->data.calls      = e->calls;
		e->data.time       = e->time;
		e->data.child_time = e->child_time;

		e->calls = e->time = e->child_time = 0;
	}

	ctx->frame_start = PROFSY_CUSTOM_TICK_FUNC();

	// ... add trace if tracing
	profsy_trace_add( ctx, ctx->frame_start, PROFSY_TRACE_EVENT_LEAVE, (uint16_t)0 );

	// if should start trace
	if( ctx->trace_to_activate != 0x0 )
	{
		ctx->active_trace = ctx->trace_to_activate;
		ctx->trace_to_activate = 0x0;
	}

	// ... add trace if tracing
	profsy_trace_add( ctx, ctx->frame_start, PROFSY_TRACE_EVENT_ENTER, (uint16_t)0 );
	
	// if is tracing...
	if( ctx->active_trace != 0x0 )
	{
		++ctx->active_trace_frame;
		if( ctx->active_trace_frame > ctx->num_trace_frames )
			profsy_trace_close( ctx, ctx->frame_start );
	}
}

void profsy_trace_begin( profsy_trace_entry* entries, unsigned int num_entries, unsigned int frames_to_capture )
{
	profsy_ctx_t ctx = g_profsy_ctx;
	if( ctx == 0x0 )
		return;

	// this will reset trace one is already running.
	ctx->trace_to_activate  = entries;
	ctx->max_active_trace   = num_entries;
	ctx->num_active_trace   = 0;
	ctx->active_trace_frame = 0;
	ctx->num_trace_frames   = frames_to_capture;
}

bool profsy_is_tracing()
{
	profsy_ctx_t ctx = g_profsy_ctx;
	return ctx != 0x0 && ctx->active_trace != 0x0;
}

unsigned int profsy_max_active_scopes() { return g_profsy_ctx == 0x0 ? 0 : g_profsy_ctx->entries_max; }
unsigned int profsy_num_active_scopes() { return g_profsy_ctx == 0x0 ? 0 : g_profsy_ctx->entries_used; }

int profsy_find_scope( const char* scope_path )
{
	profsy_ctx_t ctx = g_profsy_ctx;
	if( ctx == 0x0 )
		return -1;

	int thread_id = 0; // pass to function or name root-scope to <thread_name>?

	if( *scope_path == '\0' )
		return (int)( ctx->threads[thread_id].root - ctx->entries );

	const char* search = scope_path;
	const char* end    = search + strlen( search );

	profsy_entry* e = ctx->threads[thread_id].root;

	while( search < end )
	{
		const char* dot = strchr( search, '.' );
		if( dot == 0x0 )
			dot = end;

		profsy_entry* found = 0x0;

		for( profsy_entry* child = e->children; child != 0x0 && found == 0x0; child = child->next_child )
			if( strncmp( child->data.name, search, (size_t)( dot - search ) ) == 0 )
				found = child;

		if( found == 0x0 )
			return -1;

		e = found;
		search = dot + 1;
	}

	return (int)( e - ctx->entries );
}

profsy_scope_data* profsy_get_scope_data( int scope_id )
{
	profsy_ctx_t ctx = g_profsy_ctx;

	if( ctx == 0x0 || (unsigned int)scope_id >= ctx->entries_max )
		return 0x0;

	return &ctx->entries[scope_id].data;
}

static void profsy_append_hierarchy( const profsy_entry* entry, const profsy_scope_data** child_scopes, unsigned int* num_child_scopes )
{
	child_scopes[(*num_child_scopes)++] = &entry->data;

	for( profsy_entry* child = entry->children;
		 child != 0x0;
		 child = child->next_child )
		profsy_append_hierarchy( child, child_scopes, num_child_scopes );
}

void profsy_get_scope_hierarchy( const profsy_scope_data** child_scopes, unsigned int /*num_child_scopes*/ )
{
	profsy_ctx_t ctx = g_profsy_ctx;
	
	if( ctx == 0x0 )
		return;

	uint32_t thread_start = 0;

	for( int i = 0; i < ctx->threads_used; ++i )
	{
		uint32_t curr_scope = 0;
		profsy_thread* thread = ctx->threads + i;
		profsy_append_hierarchy( thread->root, child_scopes + thread_start, &curr_scope );
		child_scopes[thread_start + curr_scope] = &thread->overflow->data;
		thread_start += curr_scope + 1;
	}
}
