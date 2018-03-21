#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h> 

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

static void
signal_deadloop(int sig) {
	g_breakout = 1;
}

static void
register_signal(int sig,void (*handler)(int)) {
	struct sigaction sa;
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);
	sigaction(sig, &sa, NULL);
}

static void
ignore_signal(int sig) {
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, 0);
}

int dlclose(void* handle) {
	return 0;
}

/*
TCMALLOC_DEBUG=<level>      调试级别，取值为1-2
MALLOCSTATS=<level>         设置显示内存使用状态级别，取值为1-2
HEAPPROFILE=<pre>           指定内存泄露检查的数据导出文件
HEAPCHECK=<type>            堆检查类型，type=normal/strict/draconian
CPUPROFILE=./cpu.prof ./engine_tc
RUNNING_ON_VALGRIND
pprof engine_tc cpu.prof --inuse_objects --lines
pprof --pdf engine_tc cpu.prof --inuse_objects --lines > callgrind.pdf
pprof --callgrind engine_tc cpu.prof --inuse_objects --lines > callgrind.engine
Efence
EF_ALIGNMENT：这是Efence malloc分配空间的内存对齐字节数。这个变量的默认值是sizeof(int)，32位字长的CPU对应的该值是4。这个值也是Efence能够检测的内存越界的最小值。
EF_PROTECT_BELOW： 默认情况下Efence是把inaccessible的页面置于分配的空间之后，所以检测到的是高地址方向的越界访问。把这个值设为1可以检测到低地址的越界访问
EF_PROTECT_FREE： 通常free后的内存块会被放到内存池，等待重新被申请分配。把这个值设为1后，free后的内存块就不会被重新分配出去，而是也被设置为inaccessible，所以Efence能够发现程序再次访问这块已经free的内存
EF_ALLOW_MALLOC_0： Efence默认会捕捉malloc(0)的情况。把该值设为1后则不会捕捉申请0字节内存的情况
EF_FILL： 分配内存后Efence会将每一byte初始化成这个值(0-255)。当这个值被设成-1时，内存的值不固定
EF_ALIGNMENT=1 ./engine master  
for clang leak check
ASAN_OPTIONS='detect_leaks=1' report_objects=true:log_threads=true
valgrind --tool=memcheck --leak-check=full
valgrind --tool=callgrind 
*/

int main(int argc,const char* argv[]) {
	assert(argc >= 2);

	ignore_signal(SIGPIPE);
	ignore_signal(SIGPROF);
	register_signal(SIGUSR1,signal_deadloop);

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	lua_getglobal(L,"package");
	lua_pushstring(L,"./.libs/?.so");
	lua_setfield(L,-2,"cpath");
	lua_pop(L,1);

	lua_getglobal(L,"require");
	lua_pushstring(L,"worker.core");
	int status = lua_pcall(L,1,1,0);
	if (status != LUA_OK)  {
		fprintf(stderr,"load worker.core error:%s\n",lua_tostring(L,-1));
		exit(1);
	}
	lua_getfield(L,-1,"create");

	int i = 1;
	for(;i < argc;i++) {
		lua_pushstring(L,argv[i]);
	}

	status = lua_pcall(L,argc-1,0,0);
	if (status != LUA_OK)  {
		fprintf(stderr,"call startup error:%s\n",lua_tostring(L,-1));
		exit(1);
	}

	lua_close(L);

	return 0;
}