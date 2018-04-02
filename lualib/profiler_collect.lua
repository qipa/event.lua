package.path = string.format("%s;%s",package.path,"./lualib/?.lua;./script/?.lua")
package.cpath = string.format("%s;%s",package.cpath,"./.libs/?.so")

local dump = require "dump.core"
local util = require "util"
local report = {}

local stdin = {}
local total = 0
local time = os.time()
function collect(...)
	total = total + 1
	local stack = {...}
	for i = #stack,1,-1 do
		local frame = stack[i]
		
		stdin[frame] = (stdin[frame] or 0) + 1

		local info = report[frame]
		if not info then
			report[frame] = {child = {},parent = {}}
			info = report[frame]
		end
		if i ~= #stack then
			info.parent[stack[i+1]] = true
		end
		local next_frame = stack[i-1]
		if next_frame then
			info.child[next_frame] = (info.child[next_frame] or 0) + 1
		end
	end
	local now = os.time()
	if now - time >= 1 then
		time = now
		local info = {}
		for frame,count in pairs(stdin) do
			table.insert(info,{frame = frame,percent = count / total})
		end
		table.sort(info,function (l,r)
			return l.percent > r.percent
		end)
		for _,detail in ipairs(info) do
			print(detail.frame,string.format("%f%%",detail.percent * 100))
		end
	end
end

function collect_start()
	print("collect_start")
end

function collect_over(file)
	print("collect_over")
	local report_str = dump.pack(report)
	local FILE = io.open(file,"w")
	FILE:write(report_str);
	FILE:close()
end

_G.collect = collect
_G.collect_start = collect_start
_G.collect_over = collect_over