

agent_user = {
	save_version = {{unique = true,index = {uid = 1}}},
	base_info = {{unique = true,index = {uid = 1}}},
	task_mgr = {{unique = true,index = {uid = 1}}},
}

scene_user = {
	save_version = {{unique = true,index = {uid = 1}}},
	scene_info = {{unique = true,index = {uid = 1}}}
}

world_user = {
	save_version = {{unique = true,index = {uid = 1}}}
}

login_user = {
	role_list = {{unique = true,index = {account = 1}}},
	save_version = {{unique = true,index = {uid = 1}}},
}

common = {
	login_version = {{unique = true,index = {uid = 1}}},
	agent_version = {{unique = true,index = {uid = 1}}},
	world_version = {{unique = true,index = {uid = 1}}},
	id_builder = {{unique = true,index = {id = 1,key = 1}}}, --复合索引,通id和id,key查询都通过索引,key查询就不是通过索引(explain)
}
