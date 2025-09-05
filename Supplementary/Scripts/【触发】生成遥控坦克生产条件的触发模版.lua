--【触发】生成遥控坦克生产条件的触发模版.lua
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

local name= "[AIS]"..selected_house.."_ROBO"
local name_on = "[AI触发模拟]"..selected_house.."_遥控坦克_on"
local name_off1 = "[AI触发模拟]"..selected_house.."_遥控坦克_off_1"
local name_off2 = "[AI触发模拟]"..selected_house.."_遥控坦克_off_2"

local variable_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(variable_index), name..",0")

local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,56,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "3,32,0,356,37,0,"..tostring(variable_index)..",58,0,"..selected_house)
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_on..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_on.." 1,"..trigger_id)


local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,57,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,57,0,356,36,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_off1..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_off1.." 1,"..trigger_id)


local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,57,0,"..tostring(variable_index)..",0,0,0,0,A")
write_string("Events", trigger_id, "2,58,0,"..selected_house..",36,0,"..tostring(variable_index).."")
write_string("Triggers", trigger_id, ""..selected_house..",<none>,"..name_off2..",0,1,1,1,0")
tag_id = get_free_id()
write_string("Tags", tag_id, "2,"..name_off2.." 1,"..trigger_id)
update_trigger()




message_box("已成功执行脚本", "执行成功", 1)
