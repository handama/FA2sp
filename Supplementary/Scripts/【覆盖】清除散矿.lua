-- 清除散矿：当一格内矿石（金矿/宝石）且相邻8格（含对角）都没有矿石时，清除这格的矿石
local function is_ore(ovrl)
	if 27 <= ovrl and ovrl <= 38 then return true end
	if 102 <= ovrl and ovrl <= 121 then return true end
	if 127 <= ovrl and ovrl <= 146 then return true end
	if 147 <= ovrl and ovrl <= 166 then return true end
	return false
end

-- 判断某坐标是否为矿石；越界视为没有矿石
local function has_ore(x, y, n)
	if x < 0 or x >= n or y < 0 or y >= n then return false end
	return is_ore(get_cell(x, y).overlay)
end

save_undo()

local n = iso_size()
local to_clear = {}

-- 第一遍：依据原有矿石分布，标记孤立矿石格子
for x = 0, n - 1 do
	for y = 0, n - 1 do
		local cell = get_cell(x, y)
		if is_ore(cell.overlay) then
			local isolated = true
			for dx = -1, 1 do
				for dy = -1, 1 do
					if (dx ~= 0 or dy ~= 0) and has_ore(x + dx, y + dy, n) then
						isolated = false
						break
					end
				end
				if not isolated then break end
			end
			if isolated then
				to_clear[#to_clear + 1] = { x = x, y = y }
			end
		end
	end
end

-- 第二遍：统一清除已标记的孤立矿石
for i, p in ipairs(to_clear) do
	local c = get_cell(p.x, p.y)
	c.overlay = 65535
	c:apply()
end

save_redo()

update_overlay()
redraw_window()
update_minimap()

print(string.format("清除散矿完成：共清除 %d 个孤立矿石格子。", #to_clear))
