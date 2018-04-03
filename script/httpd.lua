local event = require "event"
local channel = require "channel"
local http = require "http"
local handler = import "handler.http_handler"

local master_channel = channel:inherit()

function master_channel:disconnect()
    event.error(string.format("master ip:%s,port:%d down",self.ip,self.port))
    event.fork(function ()
        local result,err
        while not result do
            result,err = event.reconnect(self.ip,self.port,self)
            if not result then
                event.error(string.format("httpd connect master failed:%s",err))
                event.sleep(1)
            end
        end
        self:call("handler.master_handler","register","httpd")
    end)
end

event.fork(function ()
    local channel,err
    while not channel do
        channel,err = event.connect(string.format("tcp://127.0.0.1:%d",env.master),4,master_channel)
        if not channel then
            event.error(string.format("httpd connect master:%d failed:%s",env.master,err))
            event.sleep(1)
        end
    end
	event.error(string.format("httpd connect master:%d success",env.master))
    
    channel:call("handler.master_handler","register","httpd")



    local addr = string.format("tcp://%s:%d",env.httpd_ip,env.httpd_port)
    local httpd = http.listen(addr,function (channel,method,url,header,body)
        event.fork(function ()
            channel:set_header("Content-Type","text/html; charset=utf-8")

            local ok,info = pcall(handler.handler,channel,header,url,body)
            if not ok then
                -- channel:reply_header("Location","http://192.168.100.55:8082/index")
                channel:set_header("Refresh", "2;url='http://192.168.100.55:8082/index'");
                channel:reply(404,info)
            else
                channel:reply(200,info)
            end
        end)

    end)
    if not httpd then
        event.error(string.format("httpd listen client:%s failed",addr))
        os.exit(1)
    end
    event.error(string.format("httpd listen client:%s success",addr))
end)
