if running_lua_brush() then
	if holding_click == false then
		local cell = get_cell(X, Y)
		if first_run then
			tile_index = cell.tile
			subtile_index = cell.subtile
			message_box("你刚刚选择了源地形，接下来点击目标地形，实现替换", "提示", 0)
		else
			save_undo()
			local tile_index2 = cell.tile
			local subtile_index2 = cell.subtile
			local loop_cells = get_cells()
			for i,cell in pairs(loop_cells) do
				if cell.tile == tile_index2 and cell.subtile == subtile_index2 then
					cell.tile = tile_index
					cell.subtile = subtile_index
					cell:apply()
				end
			end
			save_redo()
		end
	end
else
	message_box("本脚本仅能通过“脚本刷”运行！", "错误", 8)
end
