-- 判断是否为矿石类覆盖物（参考已有的矿石判断逻辑）
local function is_ore(ovrl)
    if 27 <= ovrl and ovrl <= 38 then return true end
    if 102 <= ovrl and ovrl <= 121 then return true end
    if 127 <= ovrl and ovrl <= 146 then return true end
    if 147 <= ovrl and ovrl <= 166 then return true end
    return false
end

save_undo()

local total = 0       -- 遍历的格子总数
local modified = 0    -- 被修改的格子数

print("正在遍历全图单元格，请稍候...")

-- 遍历所有单元格
for x = 0, iso_size() - 1 do
    for y = 0, iso_size() - 1 do
        local cell = get_cell(x, y)
		if (cell:is_multi_selected()) then	
			total = total + 1
			
			-- 检查是否为矿石覆盖物
			if is_ore(cell.overlay) then
				cell.overlay = 30
				cell:apply()
				modified = modified + 1
			end     
		end	
    end
end

-- 刷新覆盖物显示
update_overlay()
redraw_window()
update_minimap()

print(string.format("处理完成！共遍历 %d 个单元格，将 %d 个矿石替换为宝石。", total, modified))
