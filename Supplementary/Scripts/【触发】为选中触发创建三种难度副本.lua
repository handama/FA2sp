if message_box("是否为地图创建快照？", "创建快照", 2) == 1 then create_snapshot() end
box = multi_select_box:new("请选择触发")
for i,id in pairs(get_keys("Triggers")) do
	box:add_option(id, get_param("Triggers", id, 3))
end
box:sort_options(true)
selected_triggers = box:do_modal()
for i,id in pairs(selected_triggers) do
	local trigger = get_trigger(id)
	local ori_name = trigger.name
	local repeat_type = tostring(trigger.tags[1].type)
	trigger.easy=true
	trigger.medium=false
	trigger.hard=false
	trigger.name=ori_name.." - Easy"
	trigger.tags[1].name=ori_name.." - Easy 1"
	trigger:apply()
	
	trigger:change_id(get_free_id())
	trigger.easy=false
	trigger.medium=true	
	trigger.hard=false
	trigger.name=ori_name.." - Medium"
	trigger:delete_tags(false)
	trigger:add_tag("","",repeat_type)
	trigger:apply()
	
	trigger:change_id(get_free_id())
	trigger.easy=false
	trigger.medium=false
	trigger.hard=true	
	trigger.name=ori_name.." - Hard"
	trigger:delete_tags(false)
	trigger:add_tag("","",repeat_type)
	trigger:apply()
end