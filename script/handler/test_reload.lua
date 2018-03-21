

mrq = mrq or 1

local a = 19892
local function testlocal()
	print("fuck 1",a)
end

function test(self,a)
	mrq = mrq + 1
	print("test",self,a,mrq)
	testlocal()
end