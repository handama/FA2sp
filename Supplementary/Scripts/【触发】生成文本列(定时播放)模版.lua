--�������������ı���(��ʱ����)ģ��.lua
--��---��---��---��--

input_text_number = input_box("�������ɵ��ı���������(С��100)")
text_number = tonumber(input_text_number)
if text_number < 1 or text_number >= 100 then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ����ɵ��ı�����������������Ϊ3", "�������ݷǷ�", 1)
text_number=3
end

input_time_number = input_box("�������ɵ��ı�����ʱ����(��λ����)")
time_number = tonumber(input_time_number)
if time_number < 0  then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ����ɵ��ı�����ʱ��������Ϊ10��", "�������ݷǷ�", 1)
time_number=10
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
    t[i].name = "[�ı�] - "..string.format("%02d", i)..""
	t[i].id = get_free_id()
write_string("Events", t[i].id, "1,13,0,"..time_number)
write_string("Triggers", t[i].id, "Neutral,<none>,"..t[i].name..",1,1,1,1,0")
	t[i].tag = get_free_id()
write_string("Tags", t[i].tag, "0,"..t[i].name.."1,"..t[i].id.."")
end
--������Ϊ--
for i=1, text_number-1 do
write_string("Actions", t[i].id, "2,11,4,gui:sidebartext,0,0,0,0,A,53,2,"..t[i+1].id..",0,0,0,0,A.")
end
write_string("Actions", t[text_number].id, "1,11,4,gui:sidebartext,0,0,0,0,A.")


update_trigger()
message_box("�ѳɹ�ִ�нű�", "ִ�гɹ�", 1)
