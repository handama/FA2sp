function get_random_facing()
	facings = {0,32,64,96,128,160,192,224}
	return facings[math.random(1,8)]
end

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


objs = get_infantries()
for i=#objs,1,-1 do
    local o = objs[i]
	if house ~= "none" and o.house ~= house then
		goto continue
	end
    remove_infantry(i-1)
    o.facing = tostring(get_random_facing())
    o:place()
	::continue::
end