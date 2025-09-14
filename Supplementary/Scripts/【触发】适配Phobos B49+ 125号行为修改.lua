local function clone(tbl)
    local new = {}
    for k, v in pairs(tbl) do
        new[k] = v
    end
    return new
end

mode = message_box([[Phobos B49+ �޸��˴�����Ϊ[125 ����������...]��Ĭ��ֵ��
ini�еĵ�����������Phobos 0.3�б�ʾ��\�񲥷Ž��춯����
����Phobos B49+���Ƿ������ġ�
�����ĵ�ͼ����ָ�������ֵ����Phobos B49+����Ҫ�޸ġ�

������ǡ����ò���ȫ����Ϊ0�����ţ�
������񡱽��ò���ȫ����Ϊ1�������ţ�
�����ȡ�������ò���ȡ��]], "ѡ��ģʽ", 3)
create_snapshot()
for i,id in pairs(get_keys("Triggers")) do
	local trigger = get_trigger(id)
	local actions = clone(trigger.actions)
	for i,action in pairs (actions) do
		local params = split_string(action)
		if params[1] == "125" then
			print("���޸Ĵ���: "..id..", ��Ϊ: "..i-1)
			if mode == 1 then
				params[4] = "0"
			elseif mode == 2 then
				params[4] = "1"
			else
				if params[4] == "0" then
					params[4] = "1"
				else
					params[4] = "0"
				end
			end
		end
		actions[i] = table.concat(params, ",")
	end
	for i = #actions,1,-1 do
		trigger:delete_action(i)
	end
	for i = 1,#actions do
		trigger:add_action(actions[i])
	end
	trigger:apply()
end
update_trigger()