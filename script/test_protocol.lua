local event = require "event"
local util = require "util"
local protocol = require "protocol"

protocol.parse("other")
protocol.parse("test")
protocol.load()
protocol.dumpfile("./tmp/")

local test_protocol = {
	arg1 = 1,
	arglist = {1,2,3},
	cmds = {{id = 1,argnum = {6,6,6},cmd = "msg"}}
}

local count = 1024 * 1024

 local str =protocol.encode.c2s_command(test_protocol)
 print(string.len(str))
 table.print(protocol.decode.c2s_command(str))
 local now = util.time()
 local str
 for i = 1,count do
 	str =protocol.encode.c2s_command(test_protocol)
 end
 print("protocol encode",util.time() - now,string.len(str))

 local now = util.time()
 for i = 1,count do
 	protocol.decode.c2s_command(str)
 end
 print("protocol decode",util.time() - now,string.len(str))
