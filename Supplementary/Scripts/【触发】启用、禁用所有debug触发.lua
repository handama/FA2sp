mode = message_box("������ǡ���������debug������������񡱽�������debug�����������а���debug�Ĵ�����", "ģʽѡ��", 2)
for i,id in pairs(get_keys("Triggers")) do
	local name = get_param("Triggers", id, 3)
	if string.find(name:lower(), "debug") then
		trigger = get_trigger(id)	
		if mode == 1 then
			trigger.disabled = false
		else
			trigger.disabled = true
		end
		trigger:apply()
	end
end