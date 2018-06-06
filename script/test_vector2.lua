local util = require "util"


local vt1 = util.vector2:new(1,1)
local vt2 = util.vector2:new(1,0)

print(vt1:angle(vt2))

local vt1 = util.vector2:new(1,0)
local vt2 = util.vector2:new(1,1.732)

print(vt1:angle(vt2))

print(vt1 == vt2)

print(vt1 == vt1)

local vt = vt1 + vt2

print(util.dot2segment(1,0,1,0,2.1,1))

table.print(util.vector2.rotation({0,1},{0,0},90))
print(util.rotation(0,1,0,0,-45))