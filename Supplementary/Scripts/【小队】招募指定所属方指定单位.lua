box = select_box:new("选择所属方")
for i,house in pairs(get_values("Countries", "rules+map")) do
	box:add_option(house, translate_house(house))
end
if is_multiplay() then
	box:add_option("<Player @ A>")
	box:add_option("<Player @ B>")
	box:add_option("<Player @ C>")
	box:add_option("<Player @ D>")
	box:add_option("<Player @ E>")
	box:add_option("<Player @ F>")
	box:add_option("<Player @ G>")
	box:add_option("<Player @ H>")
end

selected_house = box:do_modal()
		
local units = get_values("AircraftTypes", "rules+map")
for k, v in pairs(get_values("InfantryTypes", "rules+map")) do
    units[#units+1] = v
end
for k, v in pairs(get_values("VehicleTypes", "rules+map")) do
    units[#units+1] = v
end
local box2 = select_box:new("选择单位")
for i, key in pairs(units) do
	if key ~= "" then
		box2:add_option(key, get_uiname(key))
	end
end
box2:sort_options()
local selected_unit = box2:do_modal()
local name = "[LoopRecruit]"..selected_house.."-"..selected_unit

local variable_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(variable_index), name..",0")

local t = team:new()
local s = script:new()
local task = task_force:new()
t.name = name
s.name = name
task.name = name
t.recruiter = true
t.house = selected_house
t.task_force = task.id
t.script = s.id

task:add_number(1, selected_unit)
s:add_action(39, variable_index)
s:add_action(31, 0)

t:apply()
s:apply()
task:apply()

local trigger_id = get_free_id()
write_string("Actions", trigger_id, "2,57,0,"..tostring(variable_index)..",0,0,0,0,A,4,1,"..t.id..",0,0,0,0,A")
write_string("Events", trigger_id, "1,36,0,"..tostring(variable_index))
write_string("Triggers", trigger_id, "Neutral,<none>,"..name..",0,1,1,1,0")

local tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name.." 1,"..trigger_id)

update_trigger()

message_box("已成功执行脚本，将局部变量\n"..tostring(variable_index).." - "..name.."\n设置为1即可启动。", "执行成功", 1)
