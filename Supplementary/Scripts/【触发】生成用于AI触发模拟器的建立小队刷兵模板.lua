--【触发】生成用于AI触发模拟器的建立小队刷兵模板.lua
--喵---喵---喵---喵--


ans = message_box("脚本一旦执行，中途无法取消。\n请注意要保持输入数据的正确。\n您是否继续？", "警告", 1)
if ans == 1 then

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

local input_units = input_box("输入特遣部队, 例如 3E1,2HTNK,5APOC")
local units = input_units:gsub(",", "")
name = ""..selected_house.."-"..units
trigger1_name= "[刷兵]"..selected_house.."-"..units.."-".."单次计时器"
trigger2_name= "[刷兵]"..selected_house.."-"..units.."-".."单次刷兵"

-- 分割字符串
local parts = {}
for part in input_units:gmatch("([^,]+)") do
    table.insert(parts, part)
end

-- 解析数字和文本
local parsedData = {}
for i, segment in ipairs(parts) do
    local numStr = segment:match("^(%d+)")
    local textPart = segment:match("^%d+(.*)$") or segment
    
    table.insert(parsedData, {
        number = tonumber(numStr),
        text = textPart
    })
end

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

print("==== 特遣部队如下 ====")
for idx, item in ipairs(parsedData) do
    print(string.format("%d) 数量: %-2s | 单位: %s", 
        idx, 
        item.number or "N/A", 
        item.text,
		task:add_number(item.number, item.text)
    ))
end

s:add_action(0, 0)

t:apply()
s:apply()
task:apply()

box2 = select_box:new("选择要使用的局部变量")
for i,var in pairs(get_values("VariableNames")) do
    box2:add_option(i,var)
end
selected_var_index= box2:do_modal()

local create_repeat=input_box("输入触发刷兵重复类型，即：0, 1, 2")

if create_repeat ~= "0" and create_repeat ~= "1" and create_repeat ~= "2" then
message_box("输入数据不合要求, 默认将重复类型调整为0", "输入内容非法", 1)
create_repeat = "0"
end

if create_repeat == "2" then
trigger1_name= "[刷兵]"..selected_house.."-"..units.."-".."重复计时器"
trigger2_name= "[刷兵]"..selected_house.."-"..units.."-".."重复刷兵"
end

local create_time=input_box("输入触发刷兵间隔（单位：秒）,例如 220")




local trigger1_id = get_free_id()
write_string("Events", trigger1_id, "1,13,0,"..create_time)
write_string("Triggers", trigger1_id, "Neutral,<none>,"..trigger1_name..",1,1,1,1,0")
local tag1_id = get_free_id()
write_string("Tags", tag1_id, ""..create_repeat..","..trigger1_name.." 1,"..trigger1_id)

local trigger2_id = get_free_id()
write_string("Events", trigger2_id, "2,13,0,0,36,0,"..selected_var_index)
write_string("Triggers", trigger2_id, "Neutral,<none>,"..trigger2_name..",1,1,1,1,0")
local tag1_id = get_free_id()
write_string("Tags", tag1_id, ""..create_repeat..","..trigger2_name.." 1,"..trigger2_id)



write_string("Actions", trigger1_id, "1,53,1,"..trigger2_id..",0,0,0,0,A.")
write_string("Actions", trigger2_id, "2,4,1,"..t.id..",0,0,0,0,A,54,1,"..trigger2_id..",0,0,0,0,A.")

update_trigger()

print("=======================")
print("生成的小队脚本默认0-0（攻击），请自行修改脚本")
print("只需修改刷兵触发的开关即可开启关闭刷兵状态，计时器触发不需要关闭")
print("生成的刷兵触发默认禁用，请手动允许触发")
print("=======================")
message_box("已成功执行脚本，生成的触发默认禁用，请手动允许触发", "执行成功", 1)
end