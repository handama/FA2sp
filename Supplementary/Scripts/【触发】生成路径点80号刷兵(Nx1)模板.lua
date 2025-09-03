--������������·����80��ˢ��(Nx1)ģ��.lua
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

input = input_box('������·���㣬֧������������������룬��\n20-24,19,30')
wp_p = {}
for _, part in pairs(split_string(input, ",")) do
    local range_parts = split_string(part, "-")
    if #range_parts == 2 then
        local start_num, end_num = tonumber(range_parts[1]), tonumber(range_parts[2])
        if start_num and end_num then
            for i = start_num, end_num do
                table.insert(wp_p, i)
            end
        end
    else
        local num = tonumber(part)
        if num then
            table.insert(wp_p, num)
        end
    end
end
local input_units = input_box("������ǲ����, ���� 3E1,2HTNK,5APOC")
local units = input_units:gsub(",", "")

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

name = ""..selected_house.."-"..units

local team = team:new()
local script = script:new()
local task = task_force:new()
team.name = name
script.name = name
task.name = name
team.recruiter = true
team.house = selected_house
team.task_force = task.id
team.script = script.id

print("==== ��ǲ�������� ====")
for idx, item in ipairs(parsedData) do
    print(string.format("%d) ����: %-2s | ��λ: %s", 
        idx, 
        item.number or "N/A", 
        item.text,
		task:add_number(item.number, item.text)
    ))
end

script:add_action(0, 0)

team:apply()
script:apply()
task:apply()




local create_repeat=input_box("���봥��ˢ���ظ����ͣ�����0, 1, 2")

if create_repeat ~= "0" and create_repeat ~= "1" and create_repeat ~= "2" then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ��ظ����͵���Ϊ0", "�������ݷǷ�", 1)
create_repeat = "0"
end



local create_time=input_box("���봥��ˢ���������λ���룩,���� 220")

name = ""..selected_house.."-"..units


--�������ݱ�������ڵ���--
local t = {}
for i = 1, 99 do
    t[i] = {
        name,
        id,
		tag,		
    }
end

for i =1,#wp_p do
t[i].name= "[ˢ��]"..selected_house.."-"..units.." #"..wp_p[i]
if create_repeat == "2" then
t[i].name= "[ˢ��]"..selected_house.."-"..units.." #"..wp_p[i].."-".."�ظ�"
end
t[i].id = get_free_id()
write_string("Events", t[i].id, "1,13,0,"..create_time)
write_string("Triggers", t[i].id, "Neutral,<none>,"..t[i].name..",1,1,1,1,0")
t[i].tag = get_free_id()
write_string("Tags", t[i].tag, ""..create_repeat..","..t[i].name.." 1,"..t[i].id)
write_string("Actions", t[i].id, "1,80,1,"..team.id..",0,0,0,0,"..waypoint_to_string(wp_p[i]))
end

update_trigger()

print("=======================")
for i = 1,#wp_p do
print("���ɵ�ˢ��·����Ϊ #"..wp_p[i])
end
print("���ɵ�С�ӽű�Ĭ��0-0�����������������޸Ľű�")
print("���ɵĴ���Ĭ�Ͻ��ã����ֶ�������")
print("=======================")
message_box("�ѳɹ�ִ�нű������ɵĴ���Ĭ�Ͻ��ã����ֶ�������", "ִ�гɹ�", 1)
