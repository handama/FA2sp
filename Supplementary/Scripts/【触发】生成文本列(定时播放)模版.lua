--【触发】生成文本列(定时播放)模版.lua
--喵---喵---喵---喵--

input_text_number = input_box("输入生成的文本触发数量(小于100)")
text_number = tonumber(input_text_number)
if text_number < 1 or text_number >= 100 then
message_box("输入数据不合要求, 默认将生成的文本触发数量数量调整为3", "输入内容非法", 1)
text_number=3
end

input_time_number = input_box("输入生成的文本触发时间间隔(单位：秒)")
time_number = tonumber(input_time_number)
if time_number < 0  then
message_box("输入数据不合要求, 默认将生成的文本触发时间间隔调整为10秒", "输入内容非法", 1)
time_number=10
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
    t[i].name = "[文本] - "..string.format("%02d", i)..""
	t[i].id = get_free_id()
write_string("Events", t[i].id, "1,13,0,"..time_number)
write_string("Triggers", t[i].id, "Neutral,<none>,"..t[i].name..",1,1,1,1,0")
	t[i].tag = get_free_id()
write_string("Tags", t[i].tag, "0,"..t[i].name.."1,"..t[i].id.."")
end
--触发行为--
for i=1, text_number-1 do
write_string("Actions", t[i].id, "2,11,4,gui:sidebartext,0,0,0,0,A,53,2,"..t[i+1].id..",0,0,0,0,A.")
end
write_string("Actions", t[text_number].id, "1,11,4,gui:sidebartext,0,0,0,0,A.")


update_trigger()
message_box("已成功执行脚本", "执行成功", 1)
