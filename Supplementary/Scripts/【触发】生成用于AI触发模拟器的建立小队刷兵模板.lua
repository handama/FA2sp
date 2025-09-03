--����������������AI����ģ�����Ľ���С��ˢ��ģ��.lua
--��---��---��---��--


ans = message_box("�ű�һ��ִ�У���;�޷�ȡ����\n��ע��Ҫ�����������ݵ���ȷ��\n���Ƿ������", "����", 1)
if ans == 1 then

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
trigger1_name= "[ˢ��]"..selected_house.."-"..units.."-".."���μ�ʱ��"
trigger2_name= "[ˢ��]"..selected_house.."-"..units.."-".."����ˢ��"

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

box2 = select_box:new("ѡ��Ҫʹ�õľֲ�����")
for i,var in pairs(get_values("VariableNames")) do
    box2:add_option(i,var)
end
selected_var_index= box2:do_modal()

local create_repeat=input_box("���봥��ˢ���ظ����ͣ�����0, 1, 2")

if create_repeat ~= "0" and create_repeat ~= "1" and create_repeat ~= "2" then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ��ظ����͵���Ϊ0", "�������ݷǷ�", 1)
create_repeat = "0"
end

if create_repeat == "2" then
trigger1_name= "[ˢ��]"..selected_house.."-"..units.."-".."�ظ���ʱ��"
trigger2_name= "[ˢ��]"..selected_house.."-"..units.."-".."�ظ�ˢ��"
end

local create_time=input_box("���봥��ˢ���������λ���룩,���� 220")




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
print("���ɵ�С�ӽű�Ĭ��0-0�����������������޸Ľű�")
print("ֻ���޸�ˢ�������Ŀ��ؼ��ɿ����ر�ˢ��״̬����ʱ����������Ҫ�ر�")
print("���ɵ�ˢ������Ĭ�Ͻ��ã����ֶ�������")
print("=======================")
message_box("�ѳɹ�ִ�нű������ɵĴ���Ĭ�Ͻ��ã����ֶ�������", "ִ�гɹ�", 1)
end