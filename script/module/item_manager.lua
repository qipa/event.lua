local event = require "event"
local cjson = require "cjson"
local model = require "model"
local util = require "util"

local database_collection = import "module.database_collection"
local module_item = import "module.item.item"
local module_material = import "module.item.material"
local module_equipment = import "module.item.equipment"
local module_currency = import "module.item.currency"


cls_item_mgr = database_collection.cls_collection:inherit("item_mgr")

function __init__(self)
	self.cls_item_mgr:save_field("uid")
	self.cls_item_mgr:save_field("mgr")
end

function cls_item_mgr:create(uid)
	self.uid = uid
	self.mgr = {}
	self.cid_ctx = {}
	self.slot_count = 0
end

function cls_item_mgr:init(user)
	user:register_event(self,"ENTER_GAME","enter_game")
	user:register_event(self,"LEAVE_GAME","leave_game")
end

function cls_item_mgr:destroy()

end

function cls_item_mgr:enter_game(user)
	local total = 0
	for uid,item in pairs(self.mgr) do
		local same_info = self.cid_ctx[item.cid]
		if not same_info then
			same_info = {}
			self.cid_ctx[item.cid] = same_info
		end
		same_info[item.uid] = true
		total = total + 1
	end
	self.slot_count = total

	local mb = {}
	for uid,item in pairs(self.mgr) do
		table.insert(mb,item:get_info())
	end
end

function cls_item_mgr:leave_game(user)
	user:deregister_event(self,"ENTER_GAME")
	user:deregister_event(self,"LEAVE_GAME")
end

function cls_item_mgr:insert_item_by_cid(cid,amount)
	local cfg = config.item_cfg[cid]
	local overlap = cfg.overlap

	local update_info = {}

	local same_info = self.cid_ctx[cid]
	if same_info then
		for uid in pairs(same_info) do
			local item = self.mgr[uid]
			if overlap ~= -1 then
				local space = overlap - item.amount
				if space > 0 then
					if space > amount then
						item.amount = item.amount + amount
					else
						amount = amount - space
						item.amount = space
					end
				end
			else
				item.amount = item.amount + amount
				amount = 0
			end

			update_info[item.uid] = true

			if amount == 0 then
				break
			end
		end
	else
		same_info = {}
		self.cid_ctx[cid] = same_info
	end

	while amount > 0 do
		local item
		if cfg.type == common.ITEM_TYPE.EQUIPMENT then
			amount = amount - 1
			item = module_equipment.cls_equipment:new(cid,1)
		elseif cfg.type == common.ITEM_TYPE.MATERIAL then
			local count = amount
			if count > cfg.overlap then
				count = cfg.overlap
			end
			amount = amount - count
			item = module_material.cls_material:new(cid,count)
		elseif cfg.type == common.ITEM_TYPE.CURRENCY then
			item = module_currency.cls_currency:new(cid,amount)
			amount = 0
		end

		if self.slot_count >= 50 then

		else
			self.mgr[item.uid] = item
			same_info[item.uid] = true
			update_info[item.uid] = true
		end
	end

	return update_info
end

function cls_item_mgr:insert_items_by_cid(list)
	local result = {}
	for _,info in pairs(list) do
		local update_info = self:insert_items_by_cid(info.cid,info.amount)
		for uid in pairs(update_info) do
			result[uid] = true
		end
	end
end

function cls_item_mgr:insert_item(item)

end

function cls_item_mgr:insert_items(list)

end

function cls_item_mgr:delete_item_by_cid(cid,amount)
	local same_info = self.cid_ctx[cid]
	if not same_info then
		return
	end

	local update_info = {}

	while amount > 0 do
		for uid in pairs(same_info) do
			local item = self.mgr[tostring(uid)]
			if item.amount > amount then
				item.amount = item.amount - amount
				amount = 0
			else
				amount = amount - item.amount
				self.mgr[tostring(uid)] = nil
				self.slot_count = self.slot_count - 1
				same_info[uid] = nil
			end
			update_info[item.uid] = true
		end
	end

	return update_info
end

function cls_item_mgr:delete_items_by_cid(list)
	local result = {}
	for _,info in pairs(list) do
		local update_info = self:delete_item_by_cid(info.cid,info.amount)
		for uid in pairs(update_info) do
			result[uid] = true
		end
	end
end

function cls_item_mgr:delete_item_by_uid(uid,amount)

end

function cls_item_mgr:delete_items_by_uid(list)

end