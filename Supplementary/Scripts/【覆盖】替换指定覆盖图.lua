local select = message_box("是否将修改限制在多选单元格内？", "限制范围", 3)
if select == 3 then
	goto finish
end
local box = select_box:new("选择原始覆盖图")
for i,v in pairs(get_ordered_values("OverlayTypes", "rules+map")) do
	local uiname = get_uiname(v)
	local value = v
	if uiname ~= value then
		value = value.." - "..uiname
	end
	box:add_option(tostring(i), value)
end
local ori = tonumber(box:do_modal())
box.caption = "选择目标覆盖图"
local replace = tonumber(box:do_modal())
if ori < 0 then ori = 0 end
if ori > 255 then ori = 255 end
if replace < 0 then replace = 0 end
if replace > 256 then replace = 256 end

save_undo()
local loop_cells = {}
if select == 1 then
	loop_cells = get_multi_selected_cells()
elseif select == 2 then
	loop_cells = get_cells()
end
for i,cell in pairs(loop_cells) do
	if cell.overlay == ori then
		cell.overlay = replace
		cell:apply()
	end
end
save_redo()
::finish::