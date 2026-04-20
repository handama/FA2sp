if not _G.__RANDOM_SEEDED then
    math.randomseed(os.time())
    _G.__RANDOM_SEEDED = true
end
function randomNumber(t)
    return t[math.random(1, #t)]
end

if running_lua_brush() then
	health = health or { }
	if first_run then
		input = input_box("请输入血量范围，最大0-256\n如：80-120")
		local range_parts = split_string(input, "-")
		if #range_parts == 2 then
			local start_num, end_num = tonumber(range_parts[1]), tonumber(range_parts[2])
			if start_num and end_num then
				for i = start_num, end_num do
					table.insert(health, i)
				end
			end
		end
	end
	local cell = get_cell(X, Y)
	if cell.building > -1 then
		local obj = get_building(cell.building)
		remove_building(cell.building)
		obj.health = tostring(randomNumber(health))
		obj:place()
	end
else
	message_box("本脚本仅能通过“脚本刷”运行！", "错误", 8)
end