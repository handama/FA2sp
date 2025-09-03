--【触发】生成开场结束和任务目标播报模板.lua
--喵---喵---喵---喵--

input_obj_number = input_box("输入生成的任务目标数量(小于10)")
obj_number = tonumber(input_obj_number)
if obj_number < 1 or obj_number >= 10 then
message_box("输入数据不合要求, 默认将任务目标数量调整为3", "输入内容非法", 1)
obj_number=3
end



--开场触发名称--
trigger01_name= "[任务目标]0.00开场"
trigger02_name= "[任务目标]0.01难度困难"
trigger03_name= "[任务目标]0.02难度正常"
trigger04_name= "[任务目标]0.03难度简单"
trigger05_name= "[任务目标]0.04文本"
trigger06_name= "[任务目标]0.05允许操作"

trigger16_name= "[胜利]所有任务目标完成允许胜利"
trigger17_name= "[胜利]延迟10秒后胜利"
trigger18_name= "[测试]全图"

--开场触发事件和标签--
trigger01_id = get_free_id()
write_string("Events", trigger01_id, "1,8,0,0")
write_string("Triggers", trigger01_id, "Neutral,<none>,"..trigger01_name..",0,1,1,1,0")
tag01_id = get_free_id()
write_string("Tags", tag01_id, "0,"..trigger01_name.."1,"..trigger01_id.."")

--选择难度--困难--
trigger02_id = get_free_id()
write_string("Events", trigger02_id, "1,13,0,0")
write_string("Triggers", trigger02_id, "Neutral,<none>,"..trigger02_name..",1,0,0,1,0")
tag02_id = get_free_id()
write_string("Tags", tag02_id, "0,"..trigger02_name.."1,"..trigger02_id.."")
--选择难度--中等--
trigger03_id = get_free_id()
write_string("Events", trigger03_id, "1,13,0,0")
write_string("Triggers", trigger03_id, "Neutral,<none>,"..trigger03_name..",1,0,1,0,0")
tag03_id = get_free_id()
write_string("Tags", tag03_id, "0,"..trigger03_name.."1,"..trigger03_id.."")
--选择难度--简单--
trigger04_id = get_free_id()
write_string("Events", trigger04_id, "1,13,0,0")
write_string("Triggers", trigger04_id, "Neutral,<none>,"..trigger04_name..",1,1,0,0,0")
tag04_id = get_free_id()
write_string("Tags", tag04_id, "0,"..trigger04_name.."1,"..trigger04_id.."")
--文本--
trigger05_id = get_free_id()
write_string("Events", trigger05_id, "1,13,0,10")
write_string("Triggers", trigger05_id, "Neutral,<none>,"..trigger05_name..",1,1,1,1,0")
tag05_id = get_free_id()
write_string("Tags", tag05_id, "0,"..trigger05_name.."1,"..trigger05_id.."")
--允许输入--
trigger06_id = get_free_id()
write_string("Events", trigger06_id, "1,13,0,10")
write_string("Triggers", trigger06_id, "Neutral,<none>,"..trigger06_name..",1,1,1,1,0")
tag06_id = get_free_id()
write_string("Tags", tag06_id, "0,"..trigger06_name.."1,"..trigger06_id.."")


--生成数据表变量用于调用--
local t = {}
for i = 1, 9 do
    t[i] = {
        name0,
        name1,
        name2,
        id0,
        id1,
        id2,		
		tag,		
		var_name,
        var_index
    }
end

--任务目标 1 到 N 的触发名称事件标签--
for i=1, obj_number do
    t[i].name0 = "[任务目标]".. i ..".00任务目标".. i ..""
    t[i].name1 = "[任务目标]".. i ..".01任务目标".. i .."重复"	
    t[i].name2 = "[任务目标]".. i ..".02任务目标".. i .."完成"	
	t[i].var_name = "Mission0".. i .."_Acomplished"
--任务目标--
	t[i].id0 = get_free_id()
write_string("Events", t[i].id0, "1,13,0,10")
write_string("Triggers", t[i].id0, "Neutral,<none>,"..t[i].name0..",1,1,1,1,0")
	t[i].tag = get_free_id()
write_string("Tags", t[i].tag, "0,"..t[i].name0.."1,"..t[i].id0.."")
--任务目标重复--
    t[i].id1 = get_free_id()
write_string("Events", t[i].id1, "1,13,0,200")
write_string("Triggers", t[i].id1, "Neutral,<none>,"..t[i].name1..",1,1,1,1,0")
	t[i].tag = get_free_id()
write_string("Tags", t[i].tag, "2,"..t[i].name1.."1,"..t[i].id1.."")
--任务目标完成--
    t[i].id2 = get_free_id()
write_string("Events", t[i].id2, "1,13,0,10")
write_string("Triggers", t[i].id2, "Neutral,<none>,"..t[i].name2..",1,1,1,1,0")
    t[i].tag = get_free_id()
write_string("Tags", t[i].tag, "0,"..t[i].name2.."1,"..t[i].id2.."")
    t[i].var_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(t[i].var_index), t[i].var_name..",0")
end

--所有任务目标全部完成--
trigger16_id = get_free_id()
write_string("Events", trigger16_id, "1,13,0,0")
write_string("Triggers", trigger16_id, "Neutral,<none>,"..trigger16_name..",1,1,1,1,0")
tag16_id = get_free_id()
write_string("Tags", tag16_id, "0,"..trigger16_name.."1,"..trigger16_id.."")
--延迟10s胜利--
trigger17_id = get_free_id()
write_string("Events", trigger17_id, "1,13,0,10")
write_string("Triggers", trigger17_id, "Neutral,<none>,"..trigger17_name..",1,1,1,1,0")
tag17_id = get_free_id()
write_string("Tags", tag17_id, "0,"..trigger17_name.."1,"..trigger17_id.."")
--测试--全图--
trigger18_id = get_free_id()
write_string("Events", trigger18_id, "1,13,0,0")
write_string("Triggers", trigger18_id, "Neutral,<none>,"..trigger18_name..",0,1,1,1,0")
tag18_id = get_free_id()
write_string("Tags", tag18_id, "0,"..trigger18_name.."1,"..trigger18_id.."")

--任务目标 1 到 N 的触发行为--
for i=1, obj_number do
if i>1 then
write_string("Actions", t[i-1].id2, "5,11,4,mission:obj"..tostring(i-1).."comp,0,0,0,0,A,54,2,"..t[i-1].id1..",0,0,0,0,A,53,2,"..t[i].id0..",0,0,0,0,A,21,6,EVA_ObjectiveComplete,0,0,0,0,A,56,0,"..tostring(t[i-1].var_index)..",0,0,0,0,A.")
end
write_string("Actions", t[i].id0, "4,11,4,gui:sidebartext,0,0,0,0,A,53,2,"..t[i].id1..",0,0,0,0,A,53,2,"..t[i].id2..",0,0,0,0,A,19,7,BeaconPlaced,0,0,0,0,A.")
write_string("Actions", t[i].id1, "2,11,4,gui:sidebartext,0,0,0,0,A,19,7,BeaconPlaced,0,0,0,0,A.")
write_string("Actions", t[i].id2, "5,11,4,mission:obj"..tostring(i).."comp,0,0,0,0,A,54,2,"..t[i].id1..",0,0,0,0,A,53,2,"..trigger16_id..",0,0,0,0,A,21,6,EVA_ObjectiveComplete,0,0,0,0,A,56,0,"..tostring(t[i].var_index)..",0,0,0,0,A.")
end

--开场和结束的一些行为--
write_string("Actions", trigger01_id, "6,21,6,EVA_EstablishBattlefieldControl,0,0,0,0,A,11,4,name:testersmap,0,0,0,0,A,53,2,"..trigger02_id..",0,0,0,0,A,53,2,"..trigger03_id..",0,0,0,0,A,53,2,"..trigger04_id..",0,0,0,0,A,46,0,0,0,0,0,0,A.")
write_string("Actions", trigger02_id, "2,11,4,txt_hard,0,0,0,0,A,53,2,"..trigger05_id..",0,0,0,0,A.")
write_string("Actions", trigger03_id, "2,11,4,txt_normal,0,0,0,0,A,53,2,"..trigger05_id..",0,0,0,0,A.")
write_string("Actions", trigger04_id, "2,11,4,txt_easy,0,0,0,0,A,53,2,"..trigger05_id..",0,0,0,0,A.")
write_string("Actions", trigger05_id, "2,11,4,gui:sidebartext,0,0,0,0,A,53,2,"..trigger06_id..",0,0,0,0,A.")
write_string("Actions", trigger06_id, "3,11,4,gui:sidebartext,0,0,0,0,A,47,0,0,0,0,0,0,A,21,6,EVA_BattlefieldControlOnline,0,0,0,0,A.")


write_string("Actions",trigger16_id,"4,11,4,gui:sidebartext,0,0,0,0,A,21,6,EVA_ObjectiveComplete,0,0,0,0,A,53,2,"..trigger17_id..",0,0,0,0,A,19,7,Cheer,0,0,0,0,A.")
write_string("Actions",trigger17_id,"1,69,0,0,0,0,0,0,A.")
write_string("Actions",trigger18_id,"1,16,0,0,0,0,0,0,A.")

update_trigger()
message_box("已成功执行脚本，只需修改文本和任务目标完成的条件即可", "执行成功", 1)
