local event = require "event"
local util = require "util"

local prof = ...

local FILE = io.open(prof,"r")
local report = table.decode(FILE:read("*a"))
FILE:close()

table.print(report)
local function get_frame(frame)
	local frame_file_line,frame_func = frame:match("(.+)@(.+)")
	if frame_file_line ~= "[C]" then
		local func_type,func_info = frame_func:match("(.+) (.+)")
		if func_type == "function" then
			return frame_func
		elseif func_type == "global" or func_type == "upvalue" or func_type == "field" or func_type == "local" or func_type == "method" then
			local frame_file,frame_line = frame_file_line:match("(.+):(.+)")
			return string.format("%s@%s",frame_file,frame_func)
		end
	end
	return frame
end

event.fork(function ()
	event.sleep(0)
	while true do
		local line = util.readline(">>")
		if line then
			if line == "quit" or line == "q" then
				event.breakout()
				break
			end

			if line == "top" then
				local frame_stack = {}
				local frame_set = {}
				for frame,info in pairs(report) do
					if next(info.parent) == nil then
						table.insert(frame_set,frame)
						table.insert(frame_stack,frame)
						break
					end
				end

				while #frame_set > 0 do
					local next_set = {}
					table.insert(frame_stack,"========================")
					for _,last_frame in pairs(frame_set) do
						local info = report[last_frame]
						if not info then
							break
						end
						local total = {file = nil,count = {}}
						for frame,count in pairs(info.child) do
							local file,func = frame:match("(.+)@(.+)")
							total.count[func] = (total.count[func] or 0) + count
							total.file = file
							table.insert(frame_stack,string.format("%s [%d]",frame,count))
							table.insert(next_set,frame)
						end
					end
					frame_set = next_set
				end

				print(table.concat(frame_stack,"\n"))
			elseif line == "callgrind" then
				
				local frame_set = {}
				for frame,info in pairs(report) do
					if next(info.parent) == nil then
						table.insert(frame_set,frame)
						break
					end
				end

				local frame_map = {}
				local forward_map = {}
				local cached_map = {}
				while #frame_set > 0 do
					local next_set = {}
					for _,last_frame in pairs(frame_set) do

						local info = report[last_frame]
						if not info then
							break
						end
						if not frame_map[last_frame] then
							frame_map[last_frame] = true
							local parent_frame = get_frame(last_frame)

							for frame,count in pairs(info.child) do
								
								local child_frame = get_frame(frame)

								local cached = string.format("%s -> %s",last_frame,frame)
								if not cached_map[cached] then
									cached_map[cached] = true
									local forward = string.format("\t\"%s\" -> \"%s\" [label=\"%%s\"];\n",parent_frame,child_frame)
									forward_map[forward] = (forward_map[forward] or 0) + count
								end
								
								table.insert(next_set,frame)
							end
						end
						
					end
					frame_set = next_set
				end

				local forword_list = {}
				for forward,count in pairs(forward_map) do
					table.insert(forword_list,string.format(forward,count))
				end

				local dot = "digraph g {\n"
				dot = dot..table.concat(forword_list,"")
				dot = dot.."}\n"
				local FILE = io.open("./prof.dot","w")
				FILE:write(dot)
				FILE:close()
				print(event.run_process("dot -Tpng prof.dot -o prof.png"))
			end
		end
	end
end)

