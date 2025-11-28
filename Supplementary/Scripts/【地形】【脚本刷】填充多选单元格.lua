if running_lua_brush() then
	save_undo()
	local cell = get_cell(X, Y)
	local tile_index = cell.tile
	local subtile_index = cell.subtile
	for i,cell in pairs(get_multi_selected_cells()) do
		cell.tile = tile_index
		cell.subtile = subtile_index
		cell:apply()
	end
	save_redo()
else
	message_box("本脚本仅能通过“脚本刷”运行！", "错误", 8)
end
