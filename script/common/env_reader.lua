
local k = ...

local FILE = assert(io.open("./.env","r"))

local env = {}

assert(load(FILE:read("*a"),"env","text",env))()

print(env[k])