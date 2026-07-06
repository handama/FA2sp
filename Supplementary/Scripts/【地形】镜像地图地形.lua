--[[
地形镜像工具 - 将地图地形沿指定轴线镜像
支持：左右镜像、上下镜像、X轴镜像、Y轴镜像、中心对称
对称轴均穿过地图中心

坐标系说明（等轴网格坐标 ix, iy）：
  左右镜像: 对称轴 Y=X (world) → 等轴网格中 ix=iy，公式 (ix,iy)→(iy,ix)
  上下镜像: 对称轴 Y=C-X (world) → 等轴网格中 ix+iy=isoSize
  X轴镜像: 对称轴平行于世界X轴 → 等轴网格中 iy=中心，公式 (ix,iy)→(ix,isoSize-iy)
  Y轴镜像: 对称轴平行于世界Y轴 → 等轴网格中 ix=中心，公式 (ix,iy)→(isoSize-ix,iy)
  中心对称: 点反射 (ix,iy)→(isoSize-ix,isoSize-iy)
]]
local ret = message_box("本脚本只是严格按照几何关系镜像地形，无法保证镜像后的悬崖、水岸等地形正确，要继续吗？", "提示", 2)
if ret == 1 then
	local isoSize = iso_size()

	print("isoSize=" .. isoSize)
	print("")

	-- 创建对话框
	local dlg = LuaDialog:new("地形镜像工具", true)

	local mirrorModes = {"左右镜像", "上下镜像", "X轴镜像", "Y轴镜像", "中心对称"}
	dlg:add_combobox("mode", "镜像方式", mirrorModes, "左右镜像", true)

	local directionOptions = {}
	directionOptions["左右镜像"] = {"从左到右", "从右到左"}
	directionOptions["上下镜像"] = {"从上到下", "从下到上"}
	directionOptions["X轴镜像"] = {"左上→右下", "右下→左上"}
	directionOptions["Y轴镜像"] = {"右上→左下", "左下→右上"}
	directionOptions["中心对称"] = {"左上→右下", "右下→左上", "右上→左下", "左下→右上"}

	dlg:add_combobox("direction", "镜像方向", directionOptions["左右镜像"], "从左到右", true)

	dlg:on_event("mode", "selchange", function(key)
		local mode = dlg:get_string("mode")
		dlg:set_combo_items("direction", directionOptions[mode])
	end)

	local result = dlg:do_modal()
	if not result then
		print("用户取消了操作")
		return
	end

	local mode = result.mode
	local direction = result.direction

	print("镜像方式: " .. mode .. " | 方向: " .. direction)
	save_undo()

	------------------------------------------------------------
	-- 判断是否在对称轴上（轴上点不参与镜像）
	------------------------------------------------------------
	function is_on_axis(ix, iy, mode, isoSize)
		if mode == "左右镜像" then
			return ix == iy
		elseif mode == "上下镜像" then
			return ix + iy == isoSize
		elseif mode == "X轴镜像" then
			return 2 * iy == isoSize
		elseif mode == "Y轴镜像" then
			return 2 * ix == isoSize
		elseif mode == "中心对称" then
			return 2 * ix == isoSize and 2 * iy == isoSize
		end
		return false
	end

	------------------------------------------------------------
	-- 判断源侧（源侧的单元格会被镜像到目标侧）
	-- 中心对称说明：
	--   左上(左半区)=ix<isoSize/2, 右下(右半区)=ix>isoSize/2
	--   右上(上半区)=iy<isoSize/2, 左下(下半区)=iy>isoSize/2
	--   每个源区域对应半个地图
	------------------------------------------------------------
	function is_source_side(ix, iy, mode, direction, isoSize)
		local half = isoSize / 2
		if mode == "左右镜像" then
			if direction == "从左到右" then
				return ix < iy
			else
				return ix > iy
			end
		elseif mode == "上下镜像" then
			if direction == "从上到下" then
				return ix + iy < isoSize
			else
				return ix + iy > isoSize
			end
		elseif mode == "X轴镜像" then
			if direction == "左上→右下" then
				return 2 * iy < isoSize
			else
				return 2 * iy > isoSize
			end
		elseif mode == "Y轴镜像" then
			if direction == "右上→左下" then
				return 2 * ix > isoSize
			else
				return 2 * ix < isoSize
			end
		elseif mode == "中心对称" then
			-- 取半张地图作为源区域
			if direction == "左上→右下" then
				return 2 * ix < isoSize  -- 左半区
			elseif direction == "右下→左上" then
				return 2 * ix > isoSize  -- 右半区
			elseif direction == "右上→左下" then
				return 2 * iy < isoSize  -- 上半区
			elseif direction == "左下→右上" then
				return 2 * iy > isoSize  -- 下半区
			end
		end
		return false
	end

	------------------------------------------------------------
	-- 计算镜像位置
	------------------------------------------------------------
	function get_mirror_pos(ix, iy, mode, isoSize)
		if mode == "左右镜像" then
			return iy, ix
		elseif mode == "上下镜像" then
			return isoSize - iy, isoSize - ix
		elseif mode == "X轴镜像" then
			return ix, isoSize - iy
		elseif mode == "Y轴镜像" then
			return isoSize - ix, iy
		elseif mode == "中心对称" then
			return isoSize - ix, isoSize - iy
		end
		return ix, iy
	end

	------------------------------------------------------------
	-- 执行镜像
	------------------------------------------------------------

    local mirrored = 0
	local skipped = 0
	local outside = 0

	for ix = 0, isoSize - 1 do
		for iy = 0, isoSize - 1 do
			if in_map(ix, iy) then
				if is_on_axis(ix, iy, mode, isoSize) then
					skipped = skipped + 1
				elseif is_source_side(ix, iy, mode, direction, isoSize) then
					local tx, ty = get_mirror_pos(ix, iy, mode, isoSize)
					if in_map(tx, ty) then
						local src = get_cell(ix, iy)
						local dst = get_cell(tx, ty)
						dst.tile = src.tile
						dst.subtile = src.subtile
						dst.height = src.height
						dst.overlay = src.overlay
						dst.overlay_data = src.overlay_data
						dst.hidden = src.hidden
						dst:apply()
						mirrored = mirrored + 1
					else
						outside = outside + 1
					end
				end
			end
		end
		if ix % 10 == 0 then
			avoid_time_out()
		end
	end

	redraw_window()
	update_minimap()

	print("")
	print("镜像完成！")
	print("  已镜像: " .. mirrored .. " 个单元格")
	print("  轴上跳过: " .. skipped .. " 个")
	if outside > 0 then
		print("  超出地图: " .. outside .. " 个")
	end
end
