local persistence = require "persistence"


_fs_ctx = _fs_ctx or {}
_cached_data = _cached_data or {}

local lru = {}

function lru:new(name,max,unload)
	local ctx = setmetatable({},{__index = self})
	ctx.head = nil
	ctx.tail = nil
	ctx.node_ctx = {}
	ctx.count = 0
	ctx.max = max or 100
	ctx.unload = unload
	ctx.name = name
	return ctx
end

function lru:insert(id)
	local node = self.node_ctx[id]
	if not node then
		self.count = self.count + 1
		if self.head == nil then
			self.head = self.tail = {pre = nil,nxt = nil,id = id}
			self.node_ctx[id] = self.head
		else
			local node = {id = id}
			node.pre = nil
			node.nxt = self.head
			self.head = node
			self.node_ctx[id] = node
		end

		if self.count > self.max then
			local node = self.tail
			self.unload(self.name,node.id)
			self.tail = node.pre
			self.tail.nxt = nil
			self.count = self.count - 1
		end
	else
		if not node.pre then
			return
		end
		local pre_node = node.pre
		local nxt_node = node.nxt
		pre_node.nxt = nxt_node
		nxt_node.pre = pre_node
		node.pre = nil
		node.nxt = self.head
		self.head = node
	end
end

local function get_fs(name)
	if not _fs_ctx[name] then
		_fs_ctx[name] = persistence:open(name)
	end
	return _fs_ctx[name]
end

local function unload(name,id)
	local cached_ctx = _cached_data[name]
	cached_ctx.ctx[id] = nil
end

local function update_cached(name,id,data)
	local cached_ctx = _cached_data[name]
	if not cached_ctx then
		cached_ctx = {ctx = {},lru = lru:new(name,100,unload)}
		_cached_data[name] = cached_ctx
	end
	cached_ctx.ctx[id] = {time = os.time(), data = data}
	cached_ctx.lru:insert(id)
end

local function find_cached(name,id)
	local cached_ctx = _cached_data[name]
	if not cached_ctx then
		return
	end
	if not cached_ctx.ctx[id] then
		return
	end
	return cached_ctx.ctx[id].data
end

function load_data(args)
	local name = args.name
	local id = args.id
	
	local data = find_cached(name,id)
	if not data then
		local fs = get_fs(name)
		data = fs:load(id)
		if data then
			update_cached(name,id,data)
		end
	end

	return data
end

function update_data(args)
	local name = args.name
	local id = args.id
	local data = args.data
	local fs = get_fs(name)
	fs:save(id,data)
	update_cached(name,id,data)
end