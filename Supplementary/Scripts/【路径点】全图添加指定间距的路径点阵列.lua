function get_digit_count(n)
	if n == 0 then return 1 end
	return math.floor(math.log(math.abs(n), 10)) + 1
end

if message_box("是否为地图创建快照？", "创建快照", 2) == 1 then create_snapshot() end
step = tonumber(input_box("请输入路径点距离"))
wps = {}
biggest_wp = -1
local ds = step * 2
local max_iso = iso_size()
local start_Y = math.floor(max_iso / ds) * ds
for Y=start_Y,0,-ds do
	local line = {}
	for y=Y,max_iso,step do
		x = y - Y
		if in_map(x, y) then
			local cell = get_cell(x, y)
			if cell.waypoint == -1 then	
				local wpk = get_free_key("Waypoints")
				local value = tostring(y * 1000 + x)
				write_string("Waypoints", wpk, value)
				local wp = tonumber(wpk)
				if wp >= 0 then
					table.insert(line, wp)
				end
				if wp > biggest_wp then
					biggest_wp = wp
				end
			else
				print("Location: "..tostring(x)..", "..tostring(y).." already has waypoint, skip...")
			end
		end
	end
	if #line > 0 then
		table.insert(wps, line)
	end
end

for X=ds,max_iso,ds do
	local line = {}
	for x=X,max_iso,step do
		y = x - X
		if in_map(x, y) then
			local cell = get_cell(x, y)
			if cell.waypoint == -1 then	
				local wpk = get_free_key("Waypoints")
				local value = tostring(y * 1000 + x)
				write_string("Waypoints", wpk, value)
				local wp = tonumber(wpk)
				if wp >= 0 then
					table.insert(line, wp)
				end
				if wp > biggest_wp then
					biggest_wp = wp
				end
			else
				print("Location: "..tostring(x)..", "..tostring(y).." already has waypoint, skip...")
			end
		end
	end
	if #line > 0 then
		table.insert(wps, line)
	end
end

update_waypoint()
local width = #wps
local height = #wps[1]
print("Waypoint Matrix:")
if #wps == 0 then
    print("未生成任何路径点！")
    return
end

local max_col = 0
for _, row in ipairs(wps) do
    if #row > max_col then
        max_col = #row
    end
end

local digit = get_digit_count(biggest_wp)
local format_str = "%0" .. digit .. "d"

for col = 1, max_col do
    local line = ""
    for row = 1, #wps do
        local val = wps[row][col] or -1
        line = line .. " " .. string.format(format_str, val)
    end
    print(line)
end