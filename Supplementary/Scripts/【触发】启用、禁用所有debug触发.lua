mode = message_box("点击“是”启动所有debug触发，点击“否”禁用所有debug触发（名称中包含debug的触发）", "模式选择", 2)
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