local util = require "util"

local str = "q我是"
print(utf8.len(str))

for p, c in utf8.codes(str) do
	print(p, utf8.char(c))
end

for word in string.gmatch(str,utf8.charpattern) do
	print(word)
end
