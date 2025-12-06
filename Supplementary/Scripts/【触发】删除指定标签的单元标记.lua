if message_box("本脚本将删除选定标签的所有单元标记，要继续吗？", "删除单元标记", 2) == 1 then
	box = select_box:new("选择标签")
	for i,k in pairs(get_keys("Tags")) do
		name = get_param("Tags", k, 2)
		box:add_option(k, name)
	end
	box:sort_options(true)
	remove_celltags(box:do_modal())
end