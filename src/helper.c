#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifdef USE_TC
#include "tcmalloc.h"
#include "heap-profiler.h"
#include "profiler.h"
#include "malloc_extension_c.h"
#include "malloc_hook_c.h"

#define RELEASE_MEMROY 			MallocExtension_ReleaseFreeMemory
#define CPU_PROFILER_START(f) 	ProfilerStart(f)
#define CPU_PROFILER_STOP 		ProfilerStop
#define HEAP_PROFILER_START(p) 	HeapProfilerStart(p)
#define HEAP_PROFILER_STOP 		HeapProfilerStop
#define HEAP_PROFILER_DUMP(r)   HeapProfilerDump(r)

#if (TC_VERSION_MAJOR == 1 && TC_VERSION_MINOR >= 6) || (TC_VERSION_MAJOR > 1)
#else
#error "Newer version of tcmalloc required"
#endif

#else

#define RELEASE_MEMROY() 			
#define CPU_PROFILER_START(f) 	(void)f
#define CPU_PROFILER_STOP() 		
#define HEAP_PROFILER_START(p)  (void)p	
#define HEAP_PROFILER_STOP() 
#define HEAP_PROFILER_DUMP(r)   (void)r 

#endif

int
memory_allocated(lua_State* L) {
	size_t allocated_bytes = 0;
#ifdef USE_TC
	MallocExtension_GetNumericProperty("generic.current_allocated_bytes", &allocated_bytes);
#endif
	lua_pushinteger(L,allocated_bytes);
	return 1;
}

int
memory_free(lua_State* L) {
	RELEASE_MEMROY();
	return 0;
}

int
cpu_profiler_start(lua_State* L) {
	const char* fname = lua_tostring(L,1);
	CPU_PROFILER_START(fname);
	return 0;
}

int
cpu_profiler_stop(lua_State* L) {
	CPU_PROFILER_STOP();
	return 0;
}

int
heap_profiler_start(lua_State* L) {
	const char* prefix = lua_tostring(L,1);
	HEAP_PROFILER_START(prefix);
	return 0;
}

int
heap_profiler_stop(lua_State* L) {
	HEAP_PROFILER_STOP();
	return 0;
}

int
heap_profiler_dump(lua_State* L) {
	const char* reason = lua_tostring(L,1);
	HEAP_PROFILER_DUMP(reason);
	return 0;
}

int
load_helper(lua_State* L) {
	const luaL_Reg lib[] = {
		{ "free" ,memory_free },
		{ "allocated" ,memory_allocated },
		{ NULL, NULL },
	};

	luaL_newlib(L, lib);

	const luaL_Reg cpu[] = {
		{ "start" ,cpu_profiler_start },
		{ "stop" ,cpu_profiler_stop },
		{ NULL, NULL },
	};
	luaL_newlib(L, cpu);
	lua_setfield(L,-2,"cpu");

	const luaL_Reg heap[] = {
		{ "start" ,heap_profiler_start },
		{ "stop" ,heap_profiler_stop },
		{ "dump" ,heap_profiler_dump },
		{ NULL, NULL },
	};
	luaL_newlib(L, heap);
	lua_setfield(L,-2,"heap");

	return 1;
}
