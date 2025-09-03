--�����������ɽ���С��ˢ��ģ��.lua
--��---��---��---��--

box = select_box:new("ѡ��������")
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


local input_units = input_box("������ǲ����, ���� 3E1,2HTNK,5APOC")
local units = input_units:gsub(",", "")
name = ""..selected_house.."-"..units
trigger_name= "[ˢ��]"..selected_house.."-"..units

-- �ָ��ַ���
local parts = {}
for part in input_units:gmatch("([^,]+)") do
    table.insert(parts, part)
end

-- �������ֺ��ı�
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

print("==== ��ǲ�������� ====")
for idx, item in ipairs(parsedData) do
    print(string.format("%d) ����: %-2s | ��λ: %s", 
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

local create_repeat=input_box("���봥��ˢ���ظ����ͣ�����0, 1, 2")

if create_repeat ~= "0" and create_repeat ~= "1" and create_repeat ~= "2" then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ��ظ����͵���Ϊ0", "�������ݷǷ�", 1)
create_repeat = "0"
end

if create_repeat == "2" then
trigger_name= "[ˢ��]"..selected_house.."-"..units.."-".."�ظ�"
end

local create_time=input_box("���봥��ˢ���������λ���룩,���� 220")




local trigger_id = get_free_id()
write_string("Actions", trigger_id, "1,4,1,"..t.id..",0,0,0,0,A")
write_string("Events", trigger_id, "1,13,0,"..create_time)
write_string("Triggers", trigger_id, "Neutral,<none>,"..trigger_name..",1,1,1,1,0")

local tag_id = get_free_id()
write_string("Tags", tag_id, ""..create_repeat..","..trigger_name.." 1,"..trigger_id)

update_trigger()

print("=======================")
print("���ɵ�С�ӽű�Ĭ��0-0�����������������޸Ľű�")
print("���ɵĴ���Ĭ�Ͻ��ã����ֶ�������")
print("=======================")
message_box("�ѳɹ�ִ�нű������ɵĴ���Ĭ�Ͻ��ã����ֶ�������", "ִ�гɹ�", 1)
