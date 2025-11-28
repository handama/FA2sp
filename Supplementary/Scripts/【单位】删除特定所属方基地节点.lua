if is_multiplay() == false then
	if message_box("本脚本将删除选定所属方的所有基地节点，要继续吗？", "删除基地节点", 2) == 1 then
		box = select_box:new("选择所属方")
		for i,house in pairs(get_values("Houses", "map")) do
			box:add_option(house, translate_house(house))
		end
		house = box:do_modal()
		for i=0,999 do
			key = string.format("%03d", i)
			delete_key(house, key)
		end
		write_string(house, "NodeCount", "0")
		update_node()
	end
end