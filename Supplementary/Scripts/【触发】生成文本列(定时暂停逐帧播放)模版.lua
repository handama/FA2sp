--【触发】生成文本列(定时暂停逐帧播放)模版.lua
--喵---喵---喵---喵--

input_text_number = input_box("输入生成的文本触发数量(小于100)")
text_number = tonumber(input_text_number)
if text_number < 1 or text_number >= 100 then
message_box("输入数据不合要求, 默认将生成的文本触发数量数量调整为3", "输入内容非法", 1)
text_number=3
end

input_time_number = input_box("输入生成的文本时停间隔(单位：秒)")
time_number = tonumber(input_time_number)
if time_number <= 0  then
message_box("输入数据不合要求, 默认将文本时停间隔调整为4秒", "输入内容非法", 1)
time_number=4
end


--生成数据表变量用于调用--
local t = {}
for i = 1, 99 do
    t[i] = {
        name,
        id,
		tag,		
    }
end

--触发名称事件标签--
for i=1, text_number do
    t[i].name = "[文本(时停)] - "..string.format("%02d", text_number - i + 1)..""
	t[i].id = get_free_id()
write_string("Events", t[i].id, "1,13,0,0")
write_string("Triggers", t[i].id, "Neutral,<none>,"..t[i].name..",1,1,1,1,0")
	t[i].tag = get_free_id()
write_string("Tags", t[i].tag, "0,"..t[i].name.."1,"..t[i].id.."")
end
--触发行为--
for i=2, text_number do
write_string("Actions", t[i].id, "4,11,4,gui:sidebartext,0,0,0,0,A,110,0,"..time_number..",0,0,0,0,A,19,7,BeaconPlaced,0,0,0,0,A,53,2,"..t[i-1].id..",0,0,0,0,A.")
end
write_string("Actions", t[1].id, "3,11,4,gui:sidebartext,0,0,0,0,A,110,0,"..time_number..",0,0,0,0,A,19,7,BeaconPlaced,0,0,0,0,A.")

update_trigger()
message_box("已成功执行脚本", "执行成功", 1)
