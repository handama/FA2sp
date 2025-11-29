if message_box("是否为地图创建快照？", "创建快照", 2) == 1 then create_snapshot() end
create_script = message_box("是否为脚本创建三种难度副本？", "创建脚本", 2) == 1
box = multi_select_box:new("请选择小队")
for i,id in pairs(get_values("TeamTypes")) do
	box:add_option(id, get_string(id, "Name"))
end
box:sort_options(true)
selected_teams = box:do_modal()
for i,id in pairs(selected_teams) do
	local team = get_team(id)
	local task = get_task_force(team.task_force)
	local script = get_script(team.script)
	local ori_team_name = team.name
	local ori_task_name = task.name
	local ori_script_name = script.name
	
	team.name = ori_team_name.." - Easy"
	task.name = ori_task_name.." - Easy"
	if create_script then
		script.name = ori_script_name.." - Easy"
		script:apply()
	end
	task:apply()
	team:apply()
	
	team.name = ori_team_name.." - Medium"
	task.name = ori_task_name.." - Medium"
	task:change_id(get_free_id())
	task:apply()
	team.task_force = task.id
	if create_script then
		script.name = ori_script_name.." - Medium"
		script:change_id(get_free_id())
		team.script = script.id
		script:apply()
	end
	team:change_id(get_free_id())
	team:apply()
	
	team.name = ori_team_name.." - Hard"
	task.name = ori_task_name.." - Hard"
	task:change_id(get_free_id())
	task:apply()
	team.task_force = task.id
	if create_script then
		script.name = ori_script_name.." - Hard"
		script:change_id(get_free_id())
		team.script = script.id
		script:apply()
	end
	team:change_id(get_free_id())
	team:apply()
end