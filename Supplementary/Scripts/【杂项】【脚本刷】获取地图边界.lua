function calculate_mp_values(x1, y1, x2, y2, mapwidth, mapheight)
    local mpL = (mapwidth + y1 - x1 - 1) / 2
    local mpT = y1 - mpL + 2
    local sum_WH = y2 - mpT - mpL - 2
    local diff_HW = x2 - mapwidth - 1 - (mpT - mpL)
    local mpH = (sum_WH + diff_HW) / 2
    local mpW = sum_WH - mpH

    return math.ceil(mpL), math.floor(mpT), math.floor(mpW), math.floor(mpH)
end

if running_lua_brush() then
	if holding_click == false then
		if first_run then
			top_left_x = Y
			top_left_y = X
			print("提示：你刚刚选择了左上角，接下来请选择右下角")
		else
			bottom_right_x = Y
			bottom_right_y = X
			
			if top_left_y < bottom_right_y then
				local mpL, mpT, mpW, mpH = calculate_mp_values(top_left_x, top_left_y, bottom_right_x, bottom_right_y, width(), height())
				print("地图边界（左,上,宽,高）："..tostring(mpL)..","..tostring(mpT)..","..tostring(mpW)..","..tostring(mpH))
				print("提示：由于地图高度会影响显示，请尽量在打开平面显示的情况下进行选择。")
			else
				print("错误：必须按照左上角->右下角的顺序选择！")
			end
		end
	end
else
	message_box("本脚本仅能通过“脚本刷”运行！", "错误", 8)
end
