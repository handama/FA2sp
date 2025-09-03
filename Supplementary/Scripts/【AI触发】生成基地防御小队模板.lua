--【AI触发】生成基地防御小队模板.lua
--喵---喵---喵---喵--

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
side=0
if parent_country == "Americans" or parent_country == "Alliance" or parent_country == "French" or parent_country == "British" or parent_country == "Germans" then
side=1
end
if parent_country == "Confederation" or parent_country == "Arabs" or parent_country == "Africans" or parent_country == "Russians" then
side=2
end
if parent_country == "YuriCountry" then
side=3
end



local input_units = input_box("输入特遣部队, 例如 3E1,2HTNK,5APOC")
local result = input_units:gsub(",", "")
name = ""..selected_house.."-"..result
trigger_name= "[刷兵]"..selected_house.."-"..result
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
t.name = "<"..name.." DEF>"
s.name = "<"..name.." DEF>"
task.name = "<"..name.." DEF>"
t.house = selected_house
t.task_force = task.id
t.script = s.id
t.aggressive = true
t.reinforce = true
t.is_base_defense = true
t.are_team_members_recruitable = true

print("==== 特遣部队如下 ====")
for idx, item in ipairs(parsedData) do
    print(string.format("%d) 数量: %-2s | 单位: %s", 
        idx, 
        item.number or "N/A", 
        item.text,
		task:add_number(item.number, item.text)
    ))
end

s:add_action(11, 11)

t:apply()
s:apply()
task:apply()

local ai_trigger_id = get_free_id()

write_string("AITriggerTypes", ai_trigger_id, t.name.. "," ..t.id..","..selected_house..",1,-1,<none>,0000000000000000000000000000000000000000000000000000000000000000,50.000000,30.000000,50.000000,1,0,"..side..",1,<none>,1,1,1")
write_string("AITriggerTypesEnable",ai_trigger_id, "yes")


print("=======================")
print("生成的小队脚本默认11-11（进入原地警戒状态）")
print("生成的小队默认勾选如下：")
print("基地防御小队√，侵略部队√，增援部队√，小队成员可被重组√")
print("生成的AI触发默认为启用")
print("=======================")
message_box("已成功执行脚本", "执行成功", 1)
