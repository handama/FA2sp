--��������������Ļ����ģ��.lua
--��---��---��---��--

input_text_number = input_box("������2���ı�������ÿ�����ɵ��ı���������(С��100)")
text_number = tonumber(input_text_number)
if text_number < 1 or text_number >= 100 then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ����ɵ��ı�����������������Ϊ3", "�������ݷǷ�", 1)
text_number=3
end

input_time_number = input_box("�������ɵ��ı��������(��λ����)")
time_number = tonumber(input_time_number)
if time_number <= 0  then
message_box("�������ݲ���Ҫ��, Ĭ�Ͻ��ı������������Ϊ10��", "�������ݷǷ�", 1)
time_number=10
end


--�����ı����ֲ��������ı�������--
var_name = "Text_Lock"
lock_name = "[�ı���]��ʱ����"
local variable_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(variable_index), var_name..",0")
lock_id = get_free_id()

write_string("Events", lock_id, "1,13,0,"..tostring(time_number)..",37,0,"..variable_index)

write_string("Triggers", lock_id, "Neutral,<none>,"..lock_name..",0,1,1,1,0")
	lock_tag = get_free_id()
write_string("Tags", lock_tag, "2,"..lock_name.."1,"..lock_id.."")
write_string("Actions", lock_id, "2,57,0,"..tostring(variable_index)..",0,0,0,0,A,54,2,"..lock_id..",0,0,0,0,A.")

--��������Ŀ�겥���ظ��ı�--
trigger_obj_1_name = "[����Ŀ��]�ظ����� 01��ʱѭ��"
trigger_obj_2_name = "[����Ŀ��]�ظ����� 02�ı���ʾ"
trigger_obj_1_id = get_free_id()
write_string("Events", trigger_obj_1_id, "1,13,0,200")
write_string("Triggers", trigger_obj_1_id, "Neutral,<none>,"..trigger_obj_1_name..",1,1,1,1,0")
	trigger_obj_1_tag = get_free_id()
write_string("Tags", trigger_obj_1_tag, "2,"..trigger_obj_1_name.."1,"..trigger_obj_1_id.."")

trigger_obj_2_id = get_free_id()
write_string("Events", trigger_obj_2_id, "1,37,0,"..tostring(variable_index))
write_string("Triggers", trigger_obj_2_id, "Neutral,<none>,"..trigger_obj_2_name..",1,1,1,1,0")
	trigger_obj_2_tag = get_free_id()
write_string("Tags", trigger_obj_2_tag, "2,"..trigger_obj_2_name.."1,"..trigger_obj_2_id.."")

--��Ϊ--
write_string("Actions", trigger_obj_1_id, "1,53,0,"..trigger_obj_2_id..",0,0,0,0,A.")

write_string("Actions", trigger_obj_2_id, "4,11,4,gui:sidebartext,0,0,0,0,A,53,2,"..lock_id..",0,0,0,0,A,54,2,"..trigger_obj_2_id..",0,0,0,0,A,56,0,"..tostring(variable_index)..",0,0,0,0,A.")



--�������ݱ�������ڵ���--
local t = {}
for i = 1, 99 do
    t[i] = {
        name1,
        id1,
		tag1,	
	    name2,
        id2,
		tag2	
    }
end

--�ı�����A�����¼���ǩ--
for i=1, text_number do
    t[i].name1 = "[�ı�A] - "..string.format("%02d", i)..""
	t[i].id1 = get_free_id()
write_string("Events", t[i].id1, "1,13,0,"..time_number)
if i == 1 then
write_string("Events", t[i].id1, "2,13,0,"..time_number..",37,0,"..variable_index)
end
write_string("Triggers", t[i].id1, "Neutral,<none>,"..t[i].name1..",1,1,1,1,0")
t[i].tag1 = get_free_id()
write_string("Tags", t[i].tag1, "0,"..t[i].name1.."1,"..t[i].id1.."")
end	
--�ı�����A��Ϊ--
for i=1, text_number-1 do
write_string("Actions", t[i].id1, "3,11,4,gui:sidebartext,0,0,0,0,A,53,2,"..t[i+1].id1..",0,0,0,0,A,56,0,"..tostring(variable_index)..",0,0,0,0,A.")
end
write_string("Actions", t[text_number].id1, "2,11,4,gui:sidebartext,0,0,0,0,A,53,2,"..lock_id..",0,0,0,0,A.")

--�ı�����B�����¼���ǩ--
for i=1, text_number do
    t[i].name2 = "[�ı�B] - "..string.format("%02d", i)..""
	t[i].id2 = get_free_id()
write_string("Events", t[i].id2, "1,13,0,"..time_number)
if i == 1 then
write_string("Events", t[i].id2, "2,13,0,"..time_number..",37,0,"..variable_index)
end
write_string("Triggers", t[i].id2, "Neutral,<none>,"..t[i].name2..",1,1,1,1,0")
t[i].tag2 = get_free_id()
write_string("Tags", t[i].tag2, "0,"..t[i].name2.."1,"..t[i].id2.."")
end
--�ı�����B��Ϊ--
for i=1, text_number-1 do
write_string("Actions", t[i].id2, "3,11,4,gui:sidebartext,0,0,0,0,A,53,2,"..t[i+1].id2..",0,0,0,0,A,56,0,"..tostring(variable_index)..",0,0,0,0,A.")
end
write_string("Actions", t[text_number].id2, "2,11,4,gui:sidebartext,0,0,0,0,A,53,2,"..lock_id..",0,0,0,0,A.")


update_trigger()
message_box("�ѳɹ�ִ�нű�", "ִ�гɹ�", 1)
