house = "none"
if message_box("需要将修改限制到制定所属方吗？", "限制所属方", 2) == 1 then
	box = select_box:new("选择所属方")
	house_section = "Houses"
	load_from = "map"
	if is_multiplay() then
		house_section = "Countries"
		load_from = "rules+map"
	end
	for i,house in pairs(get_values(house_section, load_from)) do
		box:add_option(house, translate_house(house))
	end
	house = box:do_modal()
end

rnd_start = 0
rnd_end = 0
input = input_box('请输入随机血量的上下限，使用空格隔开，例如:\n1 127')
rnd_start = tonumber(split_string(input, " ")[1])
rnd_end = tonumber(split_string(input, " ")[2])
if rnd_start < 0 then rnd_start = 0 end
if rnd_end > 256 then rnd_end = 256 end

buildings = get_buildings()
for i=#buildings,1,-1 do
    local b = buildings[i]
	if house ~= "none" and b.house ~= house then
		goto continue
	end
    remove_building(i-1)
    b.health = tostring(math.random(rnd_start, rnd_end))
    b:place()
	::continue::
end