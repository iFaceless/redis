cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(redis VERSION 5.0)
set(CMAKE_BUILD_TYPE "Debug")
set(EXECUTABLE_OUTPUT_PATH src)
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
# deps
add_library(linenoise deps/linenoise/linenoise.c)
add_library(hiredis STATIC
        deps/hiredis/hiredis.c
        deps/hiredis/net.c
        deps/hiredis/dict.c
        deps/hiredis/sds.c
        deps/hiredis/async.c
        deps/hiredis/read.c
        )
add_library(lua STATIC
        deps/lua/src/lapi.c
        deps/lua/src/lcode.c
        deps/lua/src/ldebug.c
        deps/lua/src/ldo.c
        deps/lua/src/ldump.c
        deps/lua/src/lfunc.c
        deps/lua/src/lgc.c
        deps/lua/src/llex.c
        deps/lua/src/lmem.c
        deps/lua/src/lobject.c
        deps/lua/src/lopcodes.c
        deps/lua/src/lparser.c
        deps/lua/src/lstate.c
        deps/lua/src/lstring.c
        deps/lua/src/ltable.c
        deps/lua/src/ltm.c
        deps/lua/src/lundump.c
        deps/lua/src/lvm.c
        deps/lua/src/lzio.c
        deps/lua/src/strbuf.c
        deps/lua/src/fpconv.c
        deps/lua/src/lauxlib.c
        deps/lua/src/lbaselib.c
        deps/lua/src/ldblib.c
        deps/lua/src/liolib.c
        deps/lua/src/lmathlib.c
        deps/lua/src/loslib.c
        deps/lua/src/ltablib.c
        deps/lua/src/lstrlib.c
        deps/lua/src/loadlib.c
        deps/lua/src/linit.c
        deps/lua/src/lua_cjson.c
        deps/lua/src/lua_struct.c
        deps/lua/src/lua_cmsgpack.c
        deps/lua/src/lua_bit.c
        )
# src/modules
add_library(helloworld SHARED src/modules/helloworld.c)
set_target_properties(helloworld PROPERTIES PREFIX "" SUFFIX ".so")
add_library(hellotype SHARED src/modules/hellotype.c)
set_target_properties(hellotype PROPERTIES PREFIX "" SUFFIX ".so")
add_library(helloblock SHARED src/modules/helloblock.c)
set_target_properties(helloblock PROPERTIES PREFIX "" SUFFIX ".so")
add_library(testmodule SHARED src/modules/testmodule.c)
set_target_properties(testmodule PROPERTIES PREFIX "" SUFFIX ".so")
# redis-server
set(SRC_SERVER
        src/server.c
        src/adlist.c
        src/ae.c
        src/anet.c
        src/dict.c
        src/sds.c
        src/zmalloc.c
        src/lzf_c.c
        src/lzf_d.c
        src/pqsort.c
        src/zipmap.c
        src/sha1.c
        src/ziplist.c
        src/release.c
        src/networking.c
        src/util.c
        src/object.c
        src/db.c
        src/replication.c
        src/rdb.c
        src/t_string.c
        src/t_list.c
        src/t_set.c
        src/t_zset.c
        src/evict.c
        src/defrag.c
        src/module.c
        src/quicklist.c
        src/expire.c
        src/childinfo.c
        src/redis-check-aof.c
        src/redis-check-rdb.c
        src/lazyfree.c
        src/geohash.c
        src/rax.c
        src/geohash_helper.c
        src/siphash.c
        src/geo.c
        src/t_hash.c
        src/config.c
        src/aof.c
        src/pubsub.c
        src/multi.c
        src/debug.c
        src/sort.c
        src/intset.c
        src/syncio.c
        src/cluster.c
        src/crc16.c
        src/endianconv.c
        src/slowlog.c
        src/scripting.c
        src/bio.c
        src/rio.c
        src/rand.c
        src/memtest.c
        src/crc64.c
        src/bitops.c
        src/sentinel.c
        src/notify.c
        src/setproctitle.c
        src/blocked.c
        src/hyperloglog.c
        src/latency.c
        src/sparkline.c
        src/t_stream.c
        src/lolwut.c
        src/lolwut5.c
        src/listpack.c
        src/localtime.c
        )
add_executable(redis-server ${SRC_SERVER})
target_include_directories(redis-server
        PRIVATE ${ROOT}/deps/linenoise
        PRIVATE ${ROOT}/deps/hiredis
        PRIVATE ${ROOT}/deps/lua/src
        )
target_link_libraries(redis-server
        PRIVATE pthread
        PRIVATE m
        PRIVATE dl
        PRIVATE lua
        PRIVATE linenoise
        PRIVATE hiredis)
# redis-cli
set(SRC_CLI
        src/anet.c
        src/sds.c
        src/adlist.c
        src/redis-cli.c
        src/zmalloc.c
        src/release.c
        src/anet.c
        src/ae.c
        src/crc64.c
        src/crc16.c
        src/dict.c
        src/siphash.c
        )
add_executable(redis-cli ${SRC_CLI})
target_include_directories(redis-cli
        PRIVATE ${ROOT}/deps/linenoise
        PRIVATE ${ROOT}/deps/hiredis
        PRIVATE ${ROOT}/deps/lua/src
        )
target_link_libraries(redis-cli
        PRIVATE pthread
        PRIVATE m
        PRIVATE linenoise
        PRIVATE hiredis
        )