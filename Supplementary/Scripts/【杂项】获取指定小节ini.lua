local box = select_box:new("选择读取文件")
box:add_option("fadata")
box:add_option("rules")
box:add_option("rules+map")
box:add_option("map")
box:add_option("art")
box:add_option("sound")
box:add_option("theme")
box:add_option("ai")
box:add_option("ai+map")
box:add_option("eva")
box:add_option("theater")
box:add_option("其他（手动输入文件名）")

local load_from = box:do_modal()
if load_from == "其他（手动输入文件名）" then
	load_from = input_box("请输入文件名（相对于游戏目录，包含拓展名）")
end
local sections = get_sections(load_from)
local box2 = select_box:new("选择小节")
for i,sec in pairs(sections) do
	if sec ~= "" then
		box2:add_option(sec)
	end
end
local section = box2:do_modal()
local kvps = get_ordered_key_value_pairs(section, load_from)
local output = "输出：\n["..section.."]\n"
for _, kv in ipairs(kvps) do
    local k = kv[1]
    local v = kv[2]
    output = output .. k .. "=" .. v .. "\n"
end
print(output)
