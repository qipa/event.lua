# lua rpc module base on libev

  a tiny,lightweight rpc module for lua base on libev,easy to use,easy to learn.
  
## listener
  see master.lua
  
## connector,coroutine,channel,rpc
  see console.lua

## mongo
  see mongo.lua

## redis
  see redis.lua

## http
  see http.lua

## udp
  see test_kcp_client.lua

## kcp
  see test_kcp_client.lua

## lua script reload
  see import

## lua debugger
  see debugger.lua

## async wait for popen process exit
  see event.run_process

## how to use

### listener

* #### event.listen
  `ip`,`port`,`accept_callback`,`channel_class or nil`

  when a new connection coming,accept_callback will be invoke,with arg 1 is listener userdata,arg 2 is the channel obj create by channel_class
  
### connector

* #### event.connect
  `ip`,`port`,`channel_class or nil`

  this api must be run in a fork,cause connect op is a call,
  when connect done,if u specify the channel_class,it will create a channel 
  object by the specify channel_class,otherwise,it will create by default channel class

### bind fd

* #### event.bind
  `fd`,`channel_class or nil`
  
  bind a exist fd with the channel class,if the channel_class is nil,
  it will bind by default channel class

### dns

* #### event.dns
  `host`
  
  resove host

### httpd

* #### http.listen
  `ip`,`port`,`callback`

  start a http server,when a request coming,it will invoke callback with arg 1 is httpd self userdata,arg 2 is request,arg 3 method("POST","GET" and so on),arg 4 url,arg 5 is headers,last is body

### request
  
* #### request.reply_header
  `key`,`value`

  set reply headers with key and value

* #### request.reply
  `statu code`,`reason`,`body`

  reply a httpd request

### create timer

* #### event.timer
  `ti`,`callback`

  create a timer,it will return a timer object,u can cancel with the function timer:cancel

### sleep

* #### event.sleep
  `ti`

  this api must run in a fork,cause it is a call.it will wakeup after ti second

### create a new coroutine

* #### event.fork
  `func`,`...`

  the func will run in a new coroutine after a loop with the args ...
  

### channel
  lua bind libevent bufferevent

* #### channel.read
  `num`

  read the specify num in bufferevent,if no enough,it will return nil

* #### channel.read_line

  read line data in bufferevent,if no line,it will return nil

* #### channel.close

  it will close the bufferevent,make sure the buffervent data send out,
  and the channel.disconnect will be invoke

* #### channel.close_immediately

  close the bufferevent immediately ignore the data in buffervent whether send out,
  the channel.disconnect will not invoke

* #### channel.send

  send data to remote,u can inherit channel class and rewrite the send behaviour

* #### channel.call
  
  call data to remote,u can inherit channel class and rewrite the call behaviour


### import

  lua script reload operation

* #### import
  `file`
  
  the script can reload after import

* #### reload
  `file`
  
  reload the specity lua script

* #### auto_reload
  
  it will reload all script which modify after last reload

## run httpd server sample
  
  git clone and make,then ./event_tc master to run a simple httpd server
  ```
  local master_channel = channel:inherit()

  function master_channel:disconnect()
      handler.leave(self)
  end

  local function accept(_,channel)
      handler.enter(channel)
  end

  local ok = event.listen("0.0.0.0",env.master,accept,master_channel)
  if not ok then
      event.error("listen error")
  end

  event.error(string.format("master listen on:%s",env.master))

  for i = 1,env.httpd_max do
    event.fork(function ()
      while true do
        event.run_process("./event_tc httpd")
        event.error("httpd down,try again")
        event.sleep(1)
      end
    end)
  end

  ```
  
## some useful tool

### gdb like lua debugger
  
  only debug script load by import.lua

  to do:trigger lua debugger in gdb

#### debugger(func,file,line)
  
  start a debugger while breakpoint in file@line
  
#### show help

  ![image](https://github.com/2109/pic/blob/master/5.png)
  
#### list source
  
  ![image](https://github.com/2109/pic/blob/master/6.png)
  
#### add a breakpoint and delete a break point

  ![image](https://github.com/2109/pic/blob/master/7.png)

#### show traceback and switch frame

  ![image](https://github.com/2109/pic/blob/master/8.png)

### lua call frame profiler
  
  see profiler.c

  ![image](https://github.com/2109/pic/blob/master/prof.png)

### lua.bt

  u can use gdb script lua.bt to print call stack in gdb

  ![image](https://github.com/2109/pic/blob/master/1.png)

  ![image](https://github.com/2109/pic/blob/master/2.png)

### breakout lua dead loop

  when a dead loop running,u can kill -10 pid to breakout the deadloop

  ![image](https://github.com/2109/pic/blob/master/3.png)




