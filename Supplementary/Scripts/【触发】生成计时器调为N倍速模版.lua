--�����������ɼ�ʱ����ΪN����ģ��.lua
--��---��---��---��--


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
t[1].name = "[��ʱ��]0.01��ʾ����"
t[2].name = "[��ʱ��]0.02ռλ1(���ڶ���֡����ɾ��)"
t[3].name = "[��ʱ��]0.03��ʱ������"
t[4].name = "[��ʱ��]0.04ռλ2(���ڶ���֡����ɾ��)"
t[5].name = "[��ʱ��]0.05��ʱ��֡����"
t[6].name = "[��ʱ��]0.11��ʱ��1s����1s"
t[7].name = "[��ʱ��]0.12��ʱ��1s����2s"
t[8].name = "[��ʱ��]0.21��ʱ��1s����1s"
t[9].name = "[��ʱ��]0.22��ʱ��1s����2s"
t[10].name = "[��ʱ��]0.31������ʱ��Ϊ����������1����"
t[11].name = "[��ʱ��]0.32������ʱ��Ϊ����������2����"
t[16].name = "[��ʱ��]0.33������ʱ��Ϊ����������3����"
t[12].name = "[��ʱ��]0.41������ʱ��Ϊ�����ŷ���1����"
t[13].name = "[��ʱ��]0.42������ʱ��Ϊ�����ŷ���2����"
t[17].name = "[��ʱ��]0.43������ʱ��Ϊ�����ŷ���3����"
t[14].name = "[��ʱ��]0.50������ʱ��Ϊ��������ͣ"
t[15].name = "[��ʱ��]0.60��ʾ�ر�"
		
	t[1].id = get_free_id()
write_string("Events", t[1].id, "1,13,0,0")
write_string("Triggers", t[1].id, "Neutral,<none>,"..t[1].name..",1,1,1,1,0")
	t[1].tag = get_free_id()
write_string("Tags", t[1].tag, "0,"..t[1].name.."1,"..t[1].id.."")

	t[2].id = get_free_id()
write_string("Events", t[2].id, "1,13,0,0")
write_string("Triggers", t[2].id, "Neutral,<none>,"..t[2].name..",1,1,1,1,0")
	t[2].tag = get_free_id()
write_string("Tags", t[2].tag, "0,"..t[2].name.."1,"..t[2].id.."")

	t[3].id = get_free_id()
write_string("Events", t[3].id, "1,13,0,0")
write_string("Triggers", t[3].id, "Neutral,<none>,"..t[3].name..",1,1,1,1,0")
	t[3].tag = get_free_id()
write_string("Tags", t[3].tag, "2,"..t[3].name.."1,"..t[3].id.."")

	t[4].id = get_free_id()
write_string("Events", t[4].id, "1,13,0,0")
write_string("Triggers", t[4].id, "Neutral,<none>,"..t[4].name..",1,1,1,1,0")
	t[4].tag = get_free_id()
write_string("Tags", t[4].tag, "0,"..t[4].name.."1,"..t[4].id.."")

	t[5].id = get_free_id()
write_string("Events", t[5].id, "1,13,0,1")
write_string("Triggers", t[5].id, "Neutral,<none>,"..t[5].name..",1,1,1,1,0")
	t[5].tag = get_free_id()
write_string("Tags", t[5].tag, "2,"..t[5].name.."1,"..t[5].id.."")

	t[6].id = get_free_id()
write_string("Events", t[6].id, "1,13,0,1")
write_string("Triggers", t[6].id, "Neutral,<none>,"..t[6].name..",1,1,1,1,0")
	t[6].tag = get_free_id()
write_string("Tags", t[6].tag, "2,"..t[6].name.."1,"..t[6].id.."")

	t[7].id = get_free_id()
write_string("Events", t[7].id, "1,13,0,1")
write_string("Triggers", t[7].id, "Neutral,<none>,"..t[7].name..",1,1,1,1,0")
	t[7].tag = get_free_id()
write_string("Tags", t[7].tag, "2,"..t[7].name.."1,"..t[7].id.."")

	t[8].id = get_free_id()
write_string("Events", t[8].id, "1,13,0,1")
write_string("Triggers", t[8].id, "Neutral,<none>,"..t[8].name..",1,1,1,1,0")
	t[8].tag = get_free_id()
write_string("Tags", t[8].tag, "2,"..t[8].name.."1,"..t[8].id.."")

	t[9].id = get_free_id()
write_string("Events", t[9].id, "1,13,0,1")
write_string("Triggers", t[9].id, "Neutral,<none>,"..t[9].name..",1,1,1,1,0")
	t[9].tag = get_free_id()
write_string("Tags", t[9].tag, "2,"..t[9].name.."1,"..t[9].id.."")

	t[10].id = get_free_id()
write_string("Events", t[10].id, "1,13,0,0")
write_string("Triggers", t[10].id, "Neutral,<none>,"..t[10].name..",1,1,1,1,0")
	t[10].tag = get_free_id()
write_string("Tags", t[10].tag, "0,"..t[10].name.."1,"..t[10].id.."")

	t[11].id = get_free_id()
write_string("Events", t[11].id, "1,13,0,0")
write_string("Triggers", t[11].id, "Neutral,<none>,"..t[11].name..",1,1,1,1,0")
	t[11].tag = get_free_id()
write_string("Tags", t[11].tag, "0,"..t[11].name.."1,"..t[11].id.."")

	t[12].id = get_free_id()
write_string("Events", t[12].id, "1,13,0,0")
write_string("Triggers", t[12].id, "Neutral,<none>,"..t[12].name..",1,1,1,1,0")
	t[12].tag = get_free_id()
write_string("Tags", t[12].tag, "0,"..t[12].name.."1,"..t[12].id.."")

	t[13].id = get_free_id()
write_string("Events", t[13].id, "1,13,0,0")
write_string("Triggers", t[13].id, "Neutral,<none>,"..t[13].name..",1,1,1,1,0")
	t[13].tag = get_free_id()
write_string("Tags", t[13].tag, "0,"..t[13].name.."1,"..t[13].id.."")

	t[14].id = get_free_id()
write_string("Events", t[14].id, "1,13,0,0")
write_string("Triggers", t[14].id, "Neutral,<none>,"..t[14].name..",1,1,1,1,0")
	t[14].tag = get_free_id()
write_string("Tags", t[14].tag, "0,"..t[14].name.."1,"..t[14].id.."")

	t[15].id = get_free_id()
write_string("Events", t[15].id, "1,13,0,0")
write_string("Triggers", t[15].id, "Neutral,<none>,"..t[15].name..",1,1,1,1,0")
	t[15].tag = get_free_id()
write_string("Tags", t[15].tag, "0,"..t[15].name.."1,"..t[15].id.."")

	t[16].id = get_free_id()
write_string("Events", t[16].id, "1,13,0,0")
write_string("Triggers", t[16].id, "Neutral,<none>,"..t[16].name..",1,1,1,1,0")
	t[16].tag = get_free_id()
write_string("Tags", t[16].tag, "0,"..t[16].name.."1,"..t[16].id.."")

	t[17].id = get_free_id()
write_string("Events", t[17].id, "1,13,0,0")
write_string("Triggers", t[17].id, "Neutral,<none>,"..t[17].name..",1,1,1,1,0")
	t[17].tag = get_free_id()
write_string("Tags", t[17].tag, "0,"..t[17].name.."1,"..t[17].id.."")

--������Ϊ--

write_string("Actions", t[1].id, "2,53,2,"..t[3].id..",0,0,0,0,A,53,2,"..t[5].id..",0,0,0,0,A.")
write_string("Actions", t[2].id, "1,0,0,0,0,0,0,0,A.")
write_string("Actions", t[3].id, "4,23,0,0,0,0,0,0,A,27,0,1234,0,0,0,0,A,103,4,vox:ceva035,0,0,0,0,A,54,2,"..t[3].id..",0,0,0,0,A.")
write_string("Actions", t[4].id, "1,0,0,0,0,0,0,0,A.")
write_string("Actions", t[5].id, "1,25,0,1,0,0,0,0,A.")

write_string("Actions", t[6].id, "1,26,0,1,0,0,0,0,A.")
write_string("Actions", t[7].id, "1,26,0,2,0,0,0,0,A.")
write_string("Actions", t[8].id, "1,25,0,1,0,0,0,0,A.")
write_string("Actions", t[9].id, "1,25,0,2,0,0,0,0,A.")

write_string("Actions", t[10].id, "4,53,2,"..t[6].id..",0,0,0,0,A,54,2,"..t[7].id..",0,0,0,0,A,54,2,"..t[8].id..",0,0,0,0,A,54,2,"..t[9].id..",0,0,0,0,A.")
write_string("Actions", t[11].id, "4,54,2,"..t[6].id..",0,0,0,0,A,53,2,"..t[7].id..",0,0,0,0,A,54,2,"..t[8].id..",0,0,0,0,A,54,2,"..t[9].id..",0,0,0,0,A.")
write_string("Actions", t[12].id, "4,54,2,"..t[6].id..",0,0,0,0,A,54,2,"..t[7].id..",0,0,0,0,A,53,2,"..t[8].id..",0,0,0,0,A,54,2,"..t[9].id..",0,0,0,0,A.")
write_string("Actions", t[13].id, "4,54,2,"..t[6].id..",0,0,0,0,A,54,2,"..t[7].id..",0,0,0,0,A,54,2,"..t[8].id..",0,0,0,0,A,53,2,"..t[9].id..",0,0,0,0,A.")
write_string("Actions", t[14].id, "4,54,2,"..t[6].id..",0,0,0,0,A,54,2,"..t[7].id..",0,0,0,0,A,54,2,"..t[8].id..",0,0,0,0,A,54,2,"..t[9].id..",0,0,0,0,A.")

write_string("Actions", t[15].id, "6,24,0,0,0,0,0,0,A,54,2,"..t[5].id..",0,0,0,0,A,54,2,"..t[6].id..",0,0,0,0,A,54,2,"..t[7].id..",0,0,0,0,A,54,2,"..t[8].id..",0,0,0,0,A,54,2,"..t[9].id..",0,0,0,0,A.")

write_string("Actions", t[16].id, "4,53,2,"..t[6].id..",0,0,0,0,A,53,2,"..t[7].id..",0,0,0,0,A,54,2,"..t[8].id..",0,0,0,0,A,54,2,"..t[9].id..",0,0,0,0,A.")
write_string("Actions", t[17].id, "4,54,2,"..t[6].id..",0,0,0,0,A,54,2,"..t[7].id..",0,0,0,0,A,53,2,"..t[8].id..",0,0,0,0,A,53,2,"..t[9].id..",0,0,0,0,A.")

update_trigger()
message_box("�ѳɹ�ִ�нű�", "ִ�гɹ�", 1)
