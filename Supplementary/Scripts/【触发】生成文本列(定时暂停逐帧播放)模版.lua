--�������������ı���(��ʱ��ͣ��֡����)ģ��.lua
--��---��---��---��--

input_text_number = input_box("�������ɵ��ı���������(С��100)")
text_number = tonumber(input_text_number)
if text_number < 1 or text_number >= 100 then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ����ɵ��ı�����������������Ϊ3", "�������ݷǷ�", 1)
text_number=3
end

input_time_number = input_box("�������ɵ��ı�ʱͣ���(��λ����)")
time_number = tonumber(input_time_number)
if time_number <= 0  then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ��ı�ʱͣ�������Ϊ4��", "�������ݷǷ�", 1)
time_number=4
end


--�������ݱ�������ڵ���--
local t = {}
for i = 1, 99 do
    t[i] = {
        name,
        id,
		tag,		
    }
end

--���������¼���ǩ--
for i=1, text_number do
    t[i].name = "[�ı�(ʱͣ)] - "..string.format("%02d", text_number - i + 1)..""
	t[i].id = get_free_id()
write_string("Events", t[i].id, "1,13,0,0")
write_string("Triggers", t[i].id, "Neutral,<none>,"..t[i].name..",1,1,1,1,0")
	t[i].tag = get_free_id()
write_string("Tags", t[i].tag, "0,"..t[i].name.."1,"..t[i].id.."")
end
--������Ϊ--
for i=2, text_number do
write_string("Actions", t[i].id, "4,11,4,gui:sidebartext,0,0,0,0,A,110,0,"..time_number..",0,0,0,0,A,19,7,BeaconPlaced,0,0,0,0,A,53,2,"..t[i-1].id..",0,0,0,0,A.")
end
write_string("Actions", t[1].id, "3,11,4,gui:sidebartext,0,0,0,0,A,110,0,"..time_number..",0,0,0,0,A,19,7,BeaconPlaced,0,0,0,0,A.")

update_trigger()
message_box("�ѳɹ�ִ�нű�", "ִ�гɹ�", 1)
