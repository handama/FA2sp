box = multi_select_box:new("请选择触发")
for i,id in pairs(get_keys("Triggers")) do
	box:add_option(id, get_param("Triggers", id, 3))
end
box:sort_options(true)
selected_triggers = box:do_modal()
print("为下列触发添加了禁止自身行为：")
for i,id in pairs(selected_triggers) do
	trigger = get_trigger(id)
	trigger:add_action("54,2,"..id..",0,0,0,0,A")
	trigger:apply()
	print(get_param("Triggers", id, 3))
end