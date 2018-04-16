
local event = require "event"
local channel = require "channel"
local cjson = require "cjson"
local http_parser = require "http.parser"

local http_status_msg = {
	[100] = "Continue",
	[101] = "Switching Protocols",
	[200] = "OK",
	[201] = "Created",
	[202] = "Accepted",
	[203] = "Non-Authoritative Information",
	[204] = "No Content",
	[205] = "Reset Content",
	[206] = "Partial Content",
	[300] = "Multiple Choices",
	[301] = "Moved Permanently",
	[302] = "Found",
	[303] = "See Other",
	[304] = "Not Modified",
	[305] = "Use Proxy",
	[307] = "Temporary Redirect",
	[400] = "Bad Request",
	[401] = "Unauthorized",
	[402] = "Payment Required",
	[403] = "Forbidden",
	[404] = "Not Found",
	[405] = "Method Not Allowed",
	[406] = "Not Acceptable",
	[407] = "Proxy Authentication Required",
	[408] = "Request Time-out",
	[409] = "Conflict",
	[410] = "Gone",
	[411] = "Length Required",
	[412] = "Precondition Failed",
	[413] = "Request Entity Too Large",
	[414] = "Request-URI Too Large",
	[415] = "Unsupported Media Type",
	[416] = "Requested range not satisfiable",
	[417] = "Expectation Failed",
	[500] = "Internal Server Error",
	[501] = "Not Implemented",
	[502] = "Bad Gateway",
	[503] = "Service Unavailable",
	[504] = "Gateway Time-out",
	[505] = "HTTP Version not supported",
}

local httpd_channel = channel:inherit()

function httpd_channel:init()
	self.parser = http_parser.new(0)
	self.response = {header = {}}
	self.cookie = {}
end

function httpd_channel:dispatch(method,url,header,body)
	self.callback(self,method,url,header,body)
end

function httpd_channel:data()
	while true do
		if not self.ctx then
			self.ctx = {header = {},body = {}}
			local data = self:read_util("\r\n\r\n")
			local ok,more = self.parser:execute(self.ctx,data)
			if not ok then
				print("http parser error",more)
				self:close_immediately()
				return
			end
			if not more then
				self:dispatch(self.ctx.method,self.ctx.url,self.ctx.header,table.concat(self.ctx.body,""))
				return
			end
		else
			local data = self:read()
			local ok,more = self.parser:execute(self.ctx,data)
			if not ok then
				self:close_immediately()
				return
			end
			if not more then
				self:dispatch(self.ctx.method,self.ctx.url,self.ctx.header,table.concat(self.ctx.body,""))
				return
			else
				break
			end
		end
	end
end

function httpd_channel:set_header(k,v)
	self.response.header[k] = v
end

function httpd_channel:set_cookie(k,v)
	self.cookie[k] = v
end

function httpd_channel:session_expire(time)
	self.cookie["expire"] = time
end

function httpd_channel:reply(statuscode,info)
	local content = {}
	local statusline = string.format("HTTP/1.1 %03d %s\r\n", statuscode, http_status_msg[statuscode] or "")
	table.insert(content,statusline)
	if next(self.cookie) then
		local list = {}
		for k,v in pairs(self.cookie) do
			table.insert(list,string.format("%s=%s",k,v))
		end
		self.response.header["Set-Cookie"] = table.concat(list,";")
	end
	for k,v in pairs(self.response.header) do
		table.insert(content,string.format("%s: %s\r\n", k,v))
	end

	if info then
		table.insert(content,string.format("Content-Length: %d\r\n\r\n", #info))
		table.insert(content,info)
	else
		table.insert(content,"\r\n")
	end
	self:write(table.concat(content,""))
	self:close()
end

local httpc_channel = channel:inherit()

function httpc_channel:init()
	self.parser = http_parser.new(1)
end

function httpc_channel:disconnect()
end

function httpc_channel:dispatch(method,url,header,body)
	-- self:close_immediately()
	if self.callback then
		self.callback(self,method,url,header,body)
	else
		event.wakeup(self.session,body)
	end
end

function httpc_channel:data()
	while true do
		if not self.ctx then
			self.ctx = {header = {},body = {}}
			local data = self:read_util("\r\n\r\n")
			local ok,more = self.parser:execute(self.ctx,data)
			if not ok then
				self:close_immediately()
				return
			end
			if not more then
				self:dispatch(self.ctx.method,self.ctx.url,self.ctx.header,table.concat(self.ctx.body,""))
				return
			end
		else
			local data = self:read()
			local ok,more = self.parser:execute(self.ctx,data)
			if not ok then
				self:close_immediately()
				return
			end
			if not more then
				self:dispatch(self.ctx.method,self.ctx.url,self.ctx.header,table.concat(self.ctx.body,""))
				return
			else
				break
			end
		end
	end
end

local _M = {}


function _M.listen(addr,callback)
	return event.listen(addr,0,function (listener,channel)
		channel.callback = callback
	end,httpd_channel,true)
end

local function escape(s)
	return (string.gsub(s, "([^A-Za-z0-9_])", function(c)
		return string.format("%%%02X", string.byte(c))
	end))
end

function _M.post(host,url,header,form,callback)
	local header = header or {}
	header["Content-Type"] = "application/x-www-form-urlencoded"

	local body = {}
	for k,v in pairs(form) do
		table.insert(body, string.format("%s=%s",escape(k),escape(v)))
	end

	local channel,err = event.connect(string.format("tcp://%s",host),0,false,httpc_channel)
	if not channel then
		return false,err
	end
	channel.callback = callback

	local header_content = ""
	for k,v in pairs(header) do
		header_content = string.format("%s%s:%s\r\n", header_content, k, v)
	end

	local data
	if next(body) then
		local content = table.concat(body,"&")
		data = string.format("%s %s HTTP/1.1\r\n%sContent-Length:%d\r\n\r\n", "POST", url, header_content, #content)
		channel:write(data)
		channel:write(content)
	else
		data = string.format("%s %s HTTP/1.1\r\n%sContent-Length:0\r\n\r\n", "POST", url, header_content)
		channel:write(data)
	end
end

function _M.post_master(method,content)
	local url = method
	local header = header or {}
	header["Content-Type"] = "application/json"
	local channel,err = event.connect(env.master_http,0,false,httpc_channel)
	if not channel then
		return false,err
	end
	channel.session = event.gen_session()

	local header_content = ""
	for k,v in pairs(header) do
		header_content = string.format("%s%s:%s\r\n", header_content, k, v)
	end

	if content then
		content = cjson.encode(content)
		local data = string.format("%s %s HTTP/1.1\r\n%sContent-Length:%d\r\n\r\n", "POST", url, header_content, #content)
		channel:write(data)
		channel:write(content)
	else
		local data = string.format("%s %s HTTP/1.1\r\n%sContent-Length:0\r\n\r\n", "POST", url, header_content)
		channel:write(data)
	end

	local result = event.wait(channel.session)
	return cjson.decode(result)
end

return _M