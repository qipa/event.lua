local util = require "util"
local event = require "event"
local serialize = require "serialize.core"

local function test_simple_aoi()
	local core = fastaoi.new(1000,1000,2,5,1000)

	print('simple aoi time', (util.time() - now) / 100)

end

event.fork(function ()
	

	local tbl = {
		fuck = {
			career = "code",
			age = 123,
			h = 1.2343,
			w = 333.1
		},
		name = {
			career = "code",
			age = 123,
			h = 1.2343,
			w = 333.1
		}
	}

	local count = 1024 * 1024

	local now = util.time()
	local str
	for i = 1,count do
		str = serialize.tostring(tbl)
	end
	print("serialize.pack",util.time() - now)

	local now = util.time()
	for i = 1,count do
		serialize.unpack(str)
	end
	print("serialize.unpack",util.time() - now)


	local now = util.time()
	local str
	for i = 1,count do
		str = table.tostring(tbl)
	end
	print("table.tostring",util.time() - now)

	local now = util.time()
	for i = 1,count do
		table.decode(str)
	end
	print("table.decode",util.time() - now)

	local now = util.time()

	event.sleep(1)
	event.breakout()
end)
