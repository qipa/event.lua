local event = require "event"
local channel = require "channel"
local http = require "http"

event.fork(function ()
  
    local httpd = http.listen("tcp://0.0.0.0:8082",function (self,method,url,headers,body)
        table.print(headers)
        print("method",method)
        print("url",url)
        print("body",body)
        self:reply(200,"ok")
    end)

    if not httpd then
        event.error(string.format("httpd listen client failed"))
        os.exit(1)
    end
    event.error(string.format("httpd listen client success"))
end)
