function is_ore(ovrl)
	if 27 <= ovrl and ovrl <= 38 then return true end
	if 102 <= ovrl and ovrl <= 121 then return true end
	if 127 <= ovrl and ovrl <= 146 then return true end
	if 147 <= ovrl and ovrl <= 166 then return true end
	return false
end

save_undo()
for i,cell in pairs(get_cells()) do
	if is_ore(cell.overlay) then
		if get_tile_block(cell.tile, cell.subtile).ramp_type ~= 0 then
			cell.overlay = 65535
			cell:apply()
		end
	end
end

if message_box("是否要平滑矿石过渡？\n（会破坏非自然过渡的矿石）", "平滑矿石", 2) == 0 then 
	smooth_ore()
end
save_redo()