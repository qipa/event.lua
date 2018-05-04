local event = require "event"
local channel = require "channel"
local http = require "http"
local handler = import "handler.http_handler"

event.fork(function ()
    local httpd = http.listen("tcp://0.0.0.0:8082",function (channel,method,url,header,body)
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
