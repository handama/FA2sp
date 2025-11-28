function get_random_facing()
	facings = {0,32,64,96,128,160,192,224}
	return facings[math.random(1,8)]
end

if running_lua_brush() then
	local cell = get_cell(X, Y)
	if cell.building > -1 then
		local obj = get_building(cell.building)
		remove_building(cell.building)
		obj.facing = tostring(get_random_facing())
		obj:place()
	end
else
	message_box("本脚本仅能通过“脚本刷”运行！", "错误", 8)
end