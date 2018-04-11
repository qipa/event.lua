LUA_PATH ?= ./3rd/lua
LUA_INC ?= ./3rd/lua/src
LUA_STATIC_LIB ?= ./3rd/lua/src/liblua.a

LIBEV_PATH ?= 3rd/libev
LIBEV_INC ?= 3rd/libev
LIBEV_SHARE_LIB ?= 3rd/libev/.libs/libev.so

TC_PATH ?= 3rd/gperftools
TC_INC ?= 3rd/gperftools/src/gperftools
TC_STATIC_LIB= 3rd/gperftools/.libs/libtcmalloc_and_profiler.a

EFENCE_PATH ?= ./3rd/electric-fence
EFENCE_STATIC_LIB ?= ./3rd/electric-fence/libefence.a

# $(warning $(SIGAR_LUA_SRC))
# $(shell find $(SIGAR_LUA_INC) -maxdepth 3 -type d) 
# $(foreach dir,$(SIGAR_LUA_DIR),$(wildcard $(dir)/*.c))
# find . -type f -exec dos2unix {} \;
# makefile自动找到第一个目标去执行
# .PHONY 声明变量是命令而非目标文件,在目标没有依赖，而又存在目标文件时

LUA_CLIB_PATH ?= ./.libs
LUA_CLIB_SRC ?= ./luaclib
LUA_CLIB = ev worker profiler dump serialize redis bson mongo util lfs cjson http ikcp fastaoi toweraoi pathfinder nav mysql protocolparser protocolcore filter

CONVERT_PATH ?= ./luaclib/convert
CONVERT_SRC ?= $(wildcard $(CONVERT_PATH)/*.cpp)
CONVERT_OBJ = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(CONVERT_SRC)))

MAIN_PATH ?= ./src
MAIN_SRC ?= $(wildcard $(MAIN_PATH)/*.c)
MAIN_OBJ = $(patsubst %.c,%.o,$(patsubst %.cc,%.o,$(MAIN_SRC)))

TARGET ?= event

CC=gcc
CFLAGS=-g -Wall -O3 -fno-omit-frame-pointer $(DEFINE)

LDFLAGS=-lrt -lm -ldl -lpthread -lssl -lunwind -lstdc++
STATIC_LIBS=$(LUA_STATIC_LIB) $(TC_STATIC_LIB) 
DEFINE=-DUSE_TC
SHARED=-fPIC --shared

.PHONY : all clean debug libc efence

all : \
	$(LIBEV_SHARE_LIB) \
	$(STATIC_LIBS) \
	$(TARGET) \
	$(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).so) 
	cp $(TARGET) $(TARGET).raw && strip $(TARGET).raw

$(LUA_STATIC_LIB) :
	cd $(LUA_PATH) && make linux

$(EFENCE_STATIC_LIB) :
	cd $(EFENCE_PATH) && make

$(TC_STATIC_LIB) :
	cd $(TC_PATH) && sh autogen.sh && ./configure && make

$(LIBEV_SHARE_LIB) :
	cd $(LIBEV_PATH) && ./configure && make

$(LUA_CLIB_PATH):
	mkdir $(LUA_CLIB_PATH)

$(CONVERT_PATH)/%.o:$(CONVERT_PATH)/%.cpp
	$(CC) $(CFLAGS) -fPIC -o $@ -c $<

$(MAIN_PATH)/%.o:$(MAIN_PATH)/%.c
	$(CC) $(CFLAGS) -o $@ -c $< -I$(LUA_INC) -I$(TC_INC) 

$(MAIN_PATH)/%.o:$(MAIN_PATH)/%.cc
	$(CC) $(CFLAGS) -o $@ -c $< -I$(LUA_INC) -I$(TC_INC) 

debug :
	$(MAKE) $(ALL) CFLAGS="-g -Wall -Wno-unused-value -fno-omit-frame-pointer" TC_STATIC_LIB="3rd/gperftools/.libs/libtcmalloc_debug.a" LDFLAGS="-lrt -lm -ldl -lprofiler -lpthread -lssl -lstdc++"

leak :
	$(MAKE) $(ALL) CC=clang CFLAGS="-g -Wall -Wno-unused-value -fno-omit-frame-pointer -fsanitize=address -fsanitize=leak" DEFINE="" STATIC_LIBS="$(LUA_STATIC_LIB) $(LIBEVENT_STATIC_LIB)" LDFLAGS="-lrt -lm -ldl -lpthread -lssl -lstdc++"

libc :
	$(MAKE) $(ALL) DEFINE="" STATIC_LIBS="$(LUA_STATIC_LIB) $(LIBEVENT_STATIC_LIB)" LDFLAGS="-lrt -lm -ldl -lpthread -lssl -lstdc++"

efence :
	$(MAKE) $(ALL) DEFINE="" STATIC_LIBS="$(LUA_STATIC_LIB) $(LIBEVENT_STATIC_LIB) $(EFENCE_STATIC_LIB)" LDFLAGS="-lrt -lm -ldl -lpthread -lssl -lstdc++"

$(TARGET) : $(MAIN_OBJ) $(STATIC_LIBS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -Wl,-E

$(LUA_CLIB_PATH)/ev.so : $(LUA_CLIB_SRC)/lua-ev.c $(LUA_CLIB_SRC)/lua-client.c $(LUA_CLIB_SRC)/common.c $(LUA_CLIB_SRC)/socket_event.c $(LUA_CLIB_SRC)/socket_util.c $(LUA_CLIB_SRC)/object_container.c $(LIBEV_SHARE_LIB) | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) -Wno-strict-aliasing $(SHARED) $^ -o $@ -I$(LUA_INC) -I$(LIBEV_INC)

$(LUA_CLIB_PATH)/worker.so : $(LUA_CLIB_SRC)/lua-worker.c $(LUA_CLIB_SRC)/message_queue.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/profiler.so : $(LUA_CLIB_SRC)/lua-profiler.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/dump.so : $(LUA_CLIB_SRC)/lua-dump.c ./3rd/lua-cjson/dtoa.c $(CONVERT_OBJ) | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) -Wno-uninitialized $(SHARED) $^ -o $@ -I$(LUA_INC) -I$(CONVERT_PATH)

$(LUA_CLIB_PATH)/serialize.so : $(LUA_CLIB_SRC)/lua-serialize.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/redis.so : $(LUA_CLIB_SRC)/lua-redis.c $(CONVERT_OBJ) | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC) -I$(CONVERT_PATH)

$(LUA_CLIB_PATH)/bson.so : $(LUA_CLIB_SRC)/lua-bson.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/mongo.so : $(LUA_CLIB_SRC)/lua-mongo.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/util.so : $(LUA_CLIB_SRC)/lua-util.c $(LUA_CLIB_SRC)/common.c $(CONVERT_OBJ) ./3rd/linenoise/linenoise.c  | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) -Wno-unused-value $(SHARED) $^ -o $@ -I$(LUA_INC) -I$(CONVERT_PATH) -I./3rd/linenoise

$(LUA_CLIB_PATH)/lfs.so : ./3rd/luafilesystem/src/lfs.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/cjson.so : $(filter-out ./3rd/lua-cjson/g_fmt.c ./3rd/lua-cjson/dtoa.c,$(foreach v, $(wildcard ./3rd/lua-cjson/*.c), $(v))) | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/http.so : $(LUA_CLIB_SRC)/lua-http-parser.c ./3rd/http-parser/http_parser.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC) -I./3rd/http-parser

$(LUA_CLIB_PATH)/ikcp.so : $(LUA_CLIB_SRC)/lua-ikcp.c $(LUA_CLIB_SRC)/ikcp.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/fastaoi.so : $(LUA_CLIB_SRC)/lua-fast-aoi.c $(LUA_CLIB_SRC)/pool.c $(LUA_CLIB_SRC)/object_container.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/toweraoi.so : $(LUA_CLIB_SRC)/lua-tower-aoi.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC) -I./3rd/klib

$(LUA_CLIB_PATH)/pathfinder.so : $(LUA_CLIB_SRC)/lua-pathfinder.c $(LUA_CLIB_SRC)/pathfinder.c $(LUA_CLIB_SRC)/minheap.c  | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/nav.so : $(LUA_CLIB_SRC)/lua-nav.c $(LUA_CLIB_SRC)/nav_loader.c $(LUA_CLIB_SRC)/nav_finder.c $(LUA_CLIB_SRC)/nav_tile.c $(LUA_CLIB_SRC)/minheap.c  | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/mysql.so : $(LUA_CLIB_SRC)/lua-mysql.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC) -lmysqlclient

$(LUA_CLIB_PATH)/protocolparser.so : $(LUA_CLIB_SRC)/lua-protocol-parser.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)

$(LUA_CLIB_PATH)/protocolcore.so : $(LUA_CLIB_SRC)/lua-protocol.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)
	
$(LUA_CLIB_PATH)/filter.so : $(LUA_CLIB_SRC)/lua-filter.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(LUA_INC)	-I./3rd/klib
	
clean :
	rm -rf $(TARGET) $(TARGET).raw
	rm -rf $(LUA_CLIB_PATH)
	rm -rf src/*.o
	rm -rf luaclib/convert/*.o

cleanall :
	make clean
	cd $(LUA_PATH) && make clean
	cd $(LIBEV_PATH) && make clean
	cd $(TC_PATH) && make distclean
	cd $(EFENCE_PATH) && make clean
	
