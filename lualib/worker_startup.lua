package.path = string.format("%s;%s",package.path,"./lualib/?.lua;./script/?.lua")
package.cpath = string.format("%s;%s",package.cpath,"./.libs/?.so")

require "lfs"
local util = require "util"
local dump = require "dump.core"
local worker = require "worker"
local serialize = require "serialize.core"
local import = require "import"

table.print = util.dump
-- table.encode = serialize.pack
-- table.decode = serialize.unpack
-- table.tostring = serialize.tostring
table.encode = dump.pack
table.decode = dump.unpack
table.tostring = dump.tostring

_G.import = import.import

local worker_core,args = ...

worker.dispatch(worker_core,source,session,data,size)