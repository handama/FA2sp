-- 生成_行为4建立小队_刷兵模板_三种难度副本

-- ========== 自动创建快照 ==========
create_snapshot()

-- ========== 获取所有国家列表 ==========

local countries = {}
for i, house in pairs(get_values("Countries", "rules+map")) do
    table.insert(countries, {key = house, value = translate_house(house) .. " (" .. house .. ")"})
end
if is_multiplay() then
    for i = 0, 7 do
        local letter = string.char(string.byte("A") + i)
        table.insert(countries, {key = "<Player @ " .. letter .. ">", value = "<Player @ " .. letter .. ">"})
    end
end

-- ========== 获取所有单位列表 ==========

local all_units = {}
for k, v in pairs(get_values("InfantryTypes", "rules+map")) do
    if v ~= "" then table.insert(all_units, {id = v, name = get_uiname(v), cat = "步兵"}) end
end
for k, v in pairs(get_values("VehicleTypes", "rules+map")) do
    if v ~= "" then table.insert(all_units, {id = v, name = get_uiname(v), cat = "车辆"}) end
end
for k, v in pairs(get_values("AircraftTypes", "rules+map")) do
    if v ~= "" then table.insert(all_units, {id = v, name = get_uiname(v), cat = "飞行器"}) end
end
table.sort(all_units, function(a, b)
    if a.cat ~= b.cat then return a.cat < b.cat end
    return a.name < b.name
end)

local unit_items = {}
for _, u in ipairs(all_units) do
    table.insert(unit_items, "[" .. u.cat .. "] " .. u.name .. " (" .. u.id .. ")")
end

-- ========== 创建对话框 ==========

local dlg = LuaDialog:new("三种难度刷兵生成器", false, 680, 380)
local y = 10

-- 所属方选择
dlg:add_label("lbl_house", "所属方", 10, y + 18, 50, 20)
local country_items = {}
for _, c in ipairs(countries) do
    table.insert(country_items, c.value)
end
dlg:add_combobox("house", "", country_items, country_items[1] or "", true, 65, y, 590, 22)

-- 三种难度分区（横向三列）
local difficulty_names = {"困难 (H)", "普通 (M)", "简单 (E)"}
local col_width = 205
local col_gap = 15
local col_start_x = 15

for diff_idx = 1, 3 do
    local col_x = col_start_x + (diff_idx - 1) * (col_width + col_gap)
    local row_y = y + 32
    
    -- 难度标题
    dlg:add_label("title_" .. diff_idx, "── " .. difficulty_names[diff_idx] .. " ──", col_x, row_y + 18, col_width, 20)
    
    row_y = row_y + 24
    
    -- 特遣部队标题
    dlg:add_label("tfh_" .. diff_idx, "#  数量 单位类型", col_x, row_y + 18, col_width, 16)
    
    -- 6个单位输入行（每行：序号+数量+单位）
    for row = 1, 6 do
        row_y = row_y + 24
        local prefix = "d" .. diff_idx .. "_r" .. row
        dlg:add_label(prefix .. "_idx", tostring(row) .. ".", col_x, row_y + 18, 15, 22)
        dlg:add_edit(prefix .. "_num", "", "", col_x + 18, row_y, 32, 22)
        dlg:add_combobox(prefix .. "_unit", "", unit_items, "", false, col_x + 55, row_y, col_width - 55, 22)
    end
    
    -- 触发间隔
    row_y = row_y + 28
    dlg:add_label("delay_lbl_" .. diff_idx, "间隔(秒)", col_x, row_y + 18, 60, 22)
    dlg:add_edit("delay_" .. diff_idx, "", "220", col_x + 60, row_y, 50, 22)
end

-- 重复类型（底部居中）
local y_bottom = y + 32 + 24 + (24 + 24*6 + 28) + 5
dlg:add_label("lbl_repeat", "重复类型", 10, y_bottom + 18, 60, 22)
local repeat_items = {"0 - 任一满足，单次", "1 - 所有满足，单次", "2 - 任一满足，重复"}
dlg:add_combobox("repeat_type", "", repeat_items, repeat_items[1], true, 75, y_bottom, 180, 22)

-- ========== 显示对话框 ==========

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- ========== 解析输入 ==========

-- 解析所属方
local selected_value = result.house
local selected_house = ""
for _, c in ipairs(countries) do
    if c.value == selected_value then
        selected_house = c.key
        break
    end
end
if selected_house == "" then
    print("未选择有效的所属方，脚本已中止")
    return
end

-- 解析重复类型
local repeat_type_str = result.repeat_type or ""
local repeat_value = 0
if repeat_type_str:find("1") then
    repeat_value = 1
elseif repeat_type_str:find("2") then
    repeat_value = 2
end

-- 解析三组数据
local groups = {} -- {units={...}, delay=number}
local has_any = false

for diff_idx = 1, 3 do
    local parsed_units = {}
    for row = 1, 6 do
        local prefix = "d" .. diff_idx .. "_r" .. row
        local num_str = (result[prefix .. "_num"] or ""):match("^%s*(.-)%s*$") or ""
        local unit_str = (result[prefix .. "_unit"] or ""):match("^%s*(.-)%s*$") or ""
        if num_str ~= "" and unit_str ~= "" then
            local num = tonumber(num_str)
            local id = unit_str:match("%((.+)%)$")
            if num and num > 0 and id then
                table.insert(parsed_units, {number = num, id = id})
                print(string.format("  [难度%d] 第%d行) 数量: %d | 单位: %s", diff_idx, row, num, id))
            end
        end
    end
    
    -- 解析间隔
    local delay_str = (result["delay_" .. diff_idx] or ""):match("^%s*(.-)%s*$") or ""
    local delay = tonumber(delay_str)
    if not delay or delay < 0 then
        print(string.format("  [难度%d] 间隔无效，使用默认值 220", diff_idx))
        delay = 220
    end
    
    groups[diff_idx] = {units = parsed_units, delay = delay}
    if #parsed_units > 0 then
        has_any = true
    end
end

if not has_any then
    print("未输入任何有效的特遣部队，脚本已中止")
    return
end

-- ========== 创建三组特遣部队、脚本、小队、触发 ==========

local difficulty_suffix = {"H", "M", "E"}
-- 难度对应的触发参数(easy,medium,hard)
local difficulty_flags = {
    {easy = false, medium = false, hard = true},   -- 困难
    {easy = false, medium = true, hard = false},    -- 普通
    {easy = true, medium = false, hard = false},    -- 简单
}

local created_items = {}

for diff_idx = 1, 3 do
    local units = groups[diff_idx].units
    local delay = groups[diff_idx].delay

    -- 跳过空组（允许只填1~2组）
    if #units == 0 then
        print(string.format("  [难度%d] 未填写单位，跳过创建", diff_idx))
        table.insert(created_items, nil)
        goto continue
    end

    -- 生成名称
    local name_parts = {}
    for _, pu in ipairs(units) do
        table.insert(name_parts, pu.number .. pu.id)
    end
    local name_suffix = table.concat(name_parts, "")
    local base_name = selected_house .. "_" .. name_suffix .. "_" .. difficulty_suffix[diff_idx]
    local trigger_name = "[刷兵]" .. base_name
    if repeat_value == 2 then
        trigger_name = trigger_name .. "_重复"
    end
    
    -- 创建特遣部队
    local task = task_force:new()
    task.name = base_name
    for _, pu in ipairs(units) do
        task:add_number(pu.number, pu.id)
    end
    task:apply()
    
    -- 创建脚本（0-0：攻击移动）
    local s = script:new()
    s.name = base_name
    s:add_action(0, 0)
    s:apply()
    
    -- 创建小队
    local t = team:new()
    t.name = base_name
    t.country = selected_house
    t.task_force = task.id
    t.script = s.id
    t.recruiter = true
    t:apply()
    
    -- 创建触发
    local trig = trigger:new()
    trig.name = trigger_name
    trig.country = "Neutral"
    trig.easy = difficulty_flags[diff_idx].easy
    trig.medium = difficulty_flags[diff_idx].medium
    trig.hard = difficulty_flags[diff_idx].hard
    trig.disabled = false
    
    trig:add_event("13,0," .. delay)
    trig:add_action("4,1," .. t.id .. ",0,0,0,0,A")
    trig:add_tag("", trigger_name, repeat_value)
    trig:apply()
    
    table.insert(created_items, {
        name = base_name,
        trigger_name = trigger_name,
        trigger_id = trig.id,
        difficulty = difficulty_names[diff_idx],
        delay = delay
    })
    
    print(string.format("  √ [%s] 小队: %s | 触发: %s (ID: %s, 间隔: %d秒)",
        difficulty_names[diff_idx], base_name, trigger_name, trig.id, delay))
    
    ::continue::
end

-- ========== 输出结果 ==========

print("=======================")
local count = 0
for _, item in ipairs(created_items) do
    if item then count = count + 1 end
end
print(string.format("三种难度刷兵已生成：共 %d 组", count))
for _, item in ipairs(created_items) do
    if item then
        print(string.format("  [%s] 小队: %s | 间隔: %d秒", item.difficulty, item.name, item.delay))
    end
end
print("脚本行为：0-0（攻击移动）")
print("注意：触发默认禁用，请手动允许触发")
print("=======================")

local msg_parts = {}
for _, item in ipairs(created_items) do
    if item then
        table.insert(msg_parts, item.difficulty .. ": " .. item.trigger_name)
    end
end
message_box("三种难度刷兵已成功生成！\n" .. table.concat(msg_parts, "\n") .. "\n\n触发默认禁用，请手动允许触发", "执行成功", 1)