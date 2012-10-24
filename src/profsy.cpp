#include <profsy/profsy.h>

#include <string.h>

#define ALIGN_UP( in, alignment ) (size_t)( ( (size_t)(in) + (size_t)(alignment) - 1 ) & ~( (size_t)(alignment) - 1 ) )

const unsigned int PROFSY_BUILTIN_SCOPES = 2; // "root"- and "overflow"-scopes

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

struct profsy_ctx
{
	uint8_t* mem;

	profsy_entry* entries;
	unsigned int  entries_used;
	unsigned int  entries_max;

	profsy_entry* root;
	profsy_entry* overflow; // All scopes allocated if we run out of entries will get registered in this scope
	profsy_entry* current;  // TODO: need to be per-thread later

	uint64_t frame_start;
};

static profsy_ctx* g_profsy_ctx;

static profsy_entry* profsy_alloc_entry( profsy_ctx_t ctx, const char* name )
{
	if( ctx->entries_used >= ctx->entries_max )
		return ctx->overflow;
	
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

size_t profsy_calc_ctx_mem_usage( const profsy_init_params* params )
{
	size_t needed_mem = 16; // we add 16 bytes to be able to 16-align it
	
	needed_mem += sizeof( profsy_ctx ); 
	needed_mem  = ALIGN_UP( needed_mem, 16 );
	needed_mem += ( params->entries_max + PROFSY_BUILTIN_SCOPES ) * sizeof( profsy_entry ); // + 2 for "root" and "overflow"
	
	return needed_mem;
}

void profsy_init( const profsy_init_params* params, uint8_t* in_mem )
{
	// TODO: check that profiler is not already initialized

	// Align memory!
	uint8_t* mem = (uint8_t*)ALIGN_UP( in_mem, 16 );
	
	profsy_ctx* ctx = ( profsy_ctx* )mem;
	mem += sizeof( profsy_ctx );
	mem  = (uint8_t*)ALIGN_UP( mem, 16 );
	
	ctx->mem          = in_mem;
	ctx->entries      = ( profsy_entry* )mem;
	ctx->entries_used = 0;
	ctx->entries_max  = params->entries_max + PROFSY_BUILTIN_SCOPES;
	
	memset( ctx->entries, 0x0, sizeof( profsy_entry ) * ctx->entries_max );

	// create root scope here!
	profsy_entry* root = profsy_alloc_entry( ctx, "root" );
	profsy_entry* over = profsy_alloc_entry( ctx, "overflow scope" );

	over->parent  = root;

	ctx->root     = root;
	ctx->overflow = over;
	ctx->current  = root;

	ctx->frame_start = PROFSY_CUSTOM_TICK_FUNC();
	
	g_profsy_ctx = ctx;
}

uint8_t* profsy_shutdown()
{
	// ASSERT( g_profiler != 0x0, "trying to shutdown uninitialized profiler!" );
	uint8_t* mem = g_profsy_ctx->mem;
	g_profsy_ctx = 0x0;
	return mem;
}

profsy_ctx_t profsy_global_ctx()
{
	return g_profsy_ctx;
}

// add functions to alloc scopes outside of macro

static profsy_entry* profsy_get_child_scope( profsy_ctx* ctx, profsy_entry* parent, const char* name )
{
	profsy_entry* e = parent->children;
	while( e > ctx->overflow && e->data.name != name )
		e = e->next_child;
	return e;
}

#include <stdio.h>

int profsy_scope_enter( const char* name )
{
	profsy_ctx_t ctx = g_profsy_ctx;

	if( ctx == 0x0 )
		return -1;

	profsy_entry* current = ctx->current;

	// search for scope in current open scope
	profsy_entry* e = profsy_get_child_scope( ctx, current, name );

	// not found!
	if( e == 0x0 )
	{
		// TODO: take lock here if multiple threads are to be supported

		// search again since some other thread might have created the scope!
		e = profsy_get_child_scope( ctx, current, name );
		if( e == 0x0 )
		{
			// alloc scope and link
			e = profsy_alloc_entry( ctx, name );

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
				if( current != ctx->overflow )
					current->children = e;
			}

			if( e == ctx->overflow )
				return (int)( e - ctx->entries );

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

	// count stuff
	// ASSERT( e != 0x0 );
	ctx->current = e;

	// TODO: break of to function
	return (int)(e - ctx->entries);
}

void profsy_scope_leave( int entry_index, uint64_t tick )
{
	profsy_ctx_t ctx = g_profsy_ctx;

	if( ctx == 0x0 )
		return;

	profsy_entry* entry = ctx->entries + entry_index;

	entry->calls += 1;
	entry->time  += tick;
	entry->parent->child_time += tick;

	ctx->current = entry->parent;
}

void profsy_swap_frame()
{
	profsy_ctx_t ctx = g_profsy_ctx;
	
	if( ctx == 0x0 )
		return;

	ctx->root->calls = 1; // TODO: TOK-Hack root to be one call
	ctx->root->time  = PROFSY_CUSTOM_TICK_FUNC() - ctx->frame_start; // TODO: TOK-Hack root to be one call

	for( unsigned int i = 0; i < ctx->entries_used; ++i )
	{
		profsy_entry* e    = ctx->entries + i;
		e->data.calls      = e->calls;
		e->data.time       = e->time;
		e->data.child_time = e->child_time;

		e->calls = e->time = e->child_time = 0;
	}

	ctx->frame_start = PROFSY_CUSTOM_TICK_FUNC();
}

unsigned int profsy_max_active_scopes() { return g_profsy_ctx == 0x0 ? 0 : g_profsy_ctx->entries_max; }
unsigned int profsy_num_active_scopes() { return g_profsy_ctx == 0x0 ? 0 : g_profsy_ctx->entries_used; }

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

	// ASSERT( num_child_scopes >= g_profiler->entries.allocated_elements() );

	uint32_t curr_scope = 0;
	profsy_append_hierarchy( ctx->root, child_scopes, &curr_scope );
}
