--【触发】生成模拟AI触发条件的触发模版.lua
--喵---喵---喵---喵--

print("注意：生成的触发事件的建筑序号，")
print("是原版尤里的复仇的建筑列表索引，")
print("请仔细核对！！！") 

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
parent_country = get_string(selected_house, "ParentCountry", selected_house)

selected_Infantry_building_index = 3
selected_Vehicle_building_index = 7
selected_Navy_building_index = 23
selected_Airforce_building_index = 105

if parent_country == "Americans" or parent_country == "Alliance" or parent_country == "French" or parent_country == "British" or parent_country == "Germans" then
selected_Infantry_building_index = 3
selected_Vehicle_building_index = 7
selected_Navy_building_index = 23
selected_Airforce_building_index = 105
end
if parent_country == "Confederation" or parent_country == "Arabs" or parent_country == "Africans" or parent_country == "Russians" then
selected_Infantry_building_index = 11
selected_Vehicle_building_index = 14
selected_Navy_building_index = 61
selected_Airforce_building_index = 105
end
if parent_country == "YuriCountry" then
selected_Infantry_building_index = 302
selected_Vehicle_building_index = 303
selected_Navy_building_index = 304
selected_Airforce_building_index = 105
end


local name= "[AIS]"..selected_house.."_Infantry"
local name_on = "[AI触发模拟] "..selected_house.."_步兵_on"
local name_off = "[AI触发模拟] "..selected_house.."_步兵_off"

local variable_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(variable_index), name..",0")

local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,56,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,32,0,"..selected_Infantry_building_index..",37,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_on..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_on.." 1,"..trigger_id)


local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,57,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,57,0,"..selected_Infantry_building_index..",36,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_off..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_off.." 1,"..trigger_id)


local name= "[AIS]"..selected_house.."_Vehicle"
local name_on = "[AI触发模拟]"..selected_house.."_载具_on"
local name_off = "[AI触发模拟]"..selected_house.."_载具_off"

local variable_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(variable_index), name..",0")

local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,56,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,32,0,"..selected_Vehicle_building_index..",37,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_on..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_on.." 1,"..trigger_id)


local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,57,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,57,0,"..selected_Vehicle_building_index..",36,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_off..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_off.." 1,"..trigger_id)


local name= "[AIS]"..selected_house.."_Navy"
local name_on = "[AI触发模拟]"..selected_house.."_海军_on"
local name_off = "[AI触发模拟]"..selected_house.."_海军_off"

local variable_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(variable_index), name..",0")

local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,56,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,32,0,"..selected_Navy_building_index..",37,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_on..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_on.." 1,"..trigger_id)


local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,57,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,57,0,"..selected_Navy_building_index..",36,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_off..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_off.." 1,"..trigger_id)


local name= "[AIS]"..selected_house.."-Airforce"
local name_on = "[AI触发模拟]"..selected_house.."_空军_on"
local name_off = "[AI触发模拟]"..selected_house.."_空军_off"

local variable_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(variable_index), name..",0")

local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,56,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,32,0,"..selected_Airforce_building_index..",37,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_on..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_on.." 1,"..trigger_id)


local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,57,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,57,0,"..selected_Airforce_building_index..",36,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_off..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_off.." 1,"..trigger_id)





update_trigger()
message_box("已成功执行脚本", "执行成功", 1)
