local cjson = require "cjson"
local nav_core = require "nav.core"
local finder_core = require "pathfinder.core"
local util = require "util"

local FILE = io.open("mesh.json","r")
local mesh_info = FILE:read("*a")
FILE:close()

local FILE = io.open("tile.json","r")
local tile_info = FILE:read("*a")
FILE:close()

local nav = nav_core.create(101,cjson.decode(mesh_info))
nav:load_tile(cjson.decode(tile_info))

util.time_diff("nav",function ()
	for i = 1,10000 do
		nav:find(22,109,124,18)
	end
end)

local finder = finder_core.create(101,"nav.tile")
util.time_diff("finder",function ()
	for i = 1,10000 do
		finder:find(22,109,124,18)
	end
end)
