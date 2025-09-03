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
			local wp = place_waypoint(x, y)
			if wp >= 0 then
				table.insert(line, wp)
			end
			if wp > biggest_wp then
				biggest_wp = wp
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
			local wp = place_waypoint(x, y)
			if wp >= 0 then
				table.insert(line, wp)
			end
			if wp > biggest_wp then
				biggest_wp = wp
			end
		end
	end
	if #line > 0 then
		table.insert(wps, line)
	end
end

local width = #wps
local height = #wps[1]
print("Waypoint Matrix:")
for col = 1, height do
	local line = ""
	local format = "%0"..tostring(get_digit_count(biggest_wp)).."d"
	for row = 1, width do
		line = line.." "..string.format(format, wps[row][col])
	end
	print(line)
end