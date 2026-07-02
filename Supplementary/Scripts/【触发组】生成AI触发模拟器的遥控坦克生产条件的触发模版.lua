-- 生成AI触发模拟器的遥控坦克生产条件触发模版.lua

-- ========== 自动创建快照 ==========

create_snapshot()

-- ========== 选择所属方 ==========

local dlg = LuaDialog:new("遥控坦克生产条件生成器", false, 380, 110)

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

local country_items = {}
for _, c in ipairs(countries) do
    table.insert(country_items, c.value)
end

dlg:add_label("lbl_house", "所属方", 10, 24, 50, 20)
dlg:add_combobox("house", "", country_items, country_items[1] or "", true, 65, 6, 280, 22)

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

local selected_house = ""
for _, c in ipairs(countries) do
    if c.value == result.house then
        selected_house = c.key
        break
    end
end
if selected_house == "" then
    print("未选择有效的所属方，脚本已中止")
    return
end

-- ========== 获取国家在 Countries 注册表中的索引（0基）==========

local house_idx = nil
local ordered_values = get_ordered_values("Countries", "rules+map")
for idx, val in pairs(ordered_values) do
    if val == selected_house then
        house_idx = idx  -- 0基索引
        break
    end
end

if house_idx == nil then
    print("未找到所属方的注册索引，脚本已中止")
    return
end

print(string.format("所属方: %s (注册索引: %d)", selected_house, house_idx))
print("注意：建筑索引基于原版尤里的复仇，请仔细核对！")

-- ========== 创建变量 ==========

local var_name = "[AIS]" .. selected_house .. "_ROBO"
local variable_index = tonumber(get_free_key("VariableNames"))
write_string("VariableNames", tostring(variable_index), var_name .. ",0")

-- ========== 创建 3 个触发 ==========

-- on 触发：控制中心(356)存在 + 条件满足 + 所属方匹配 → 设置变量=1
local on_trig = trigger:new()
on_trig.name = "[AI触发模拟]" .. selected_house .. "_遥控坦克_on"
on_trig.country = selected_house
on_trig.easy = true; on_trig.medium = true; on_trig.hard = true; on_trig.disabled = false
on_trig:add_event("32,0,356")           -- 32: 建筑(控制中心)存在
on_trig:add_event("37,0," .. variable_index)  -- 37: 变量条件
on_trig:add_event("58,0," .. house_idx)      -- 58: 所属方条件
on_trig:add_action("56,0," .. variable_index .. ",0,0,0,0,A")  -- 56: 设置变量=1
on_trig:add_tag("", on_trig.name .. " 1", 2)
on_trig:apply()

-- off1 触发：控制中心被摧毁 → 设置变量=0
local off1 = trigger:new()
off1.name = "[AI触发模拟]" .. selected_house .. "_遥控坦克_off_1"
off1.country = selected_house
off1.easy = true; off1.medium = true; off1.hard = true; off1.disabled = false
off1:add_event("57,0,356")              -- 57: 建筑不存在(被摧毁)
off1:add_event("36,0," .. variable_index)  -- 36: 变量条件
off1:add_action("57,0," .. variable_index .. ",0,0,0,0,A")  -- 57: 设置变量=0
off1:add_tag("", off1.name .. " 1", 2)
off1:apply()

-- off2 触发：所属方失去此建筑/归属变更 → 设置变量=0
local off2 = trigger:new()
off2.name = "[AI触发模拟]" .. selected_house .. "_遥控坦克_off_2"
off2.country = selected_house
off2.easy = true; off2.medium = true; off2.hard = true; off2.disabled = false
off2:add_event("30,0," .. house_idx)         -- 30: 所属方条件
off2:add_event("36,0," .. variable_index)     -- 36: 变量条件
off2:add_action("57,0," .. variable_index .. ",0,0,0,0,A")  -- 57: 设置变量=0
off2:add_tag("", off2.name .. " 1", 2)
off2:apply()

-- ========== 输出结果 ==========

print("=======================")
print("遥控坦克生产条件已生成：")
print(string.format("  变量 [%d] %s", variable_index, var_name))
print(string.format("  on:   [%s] 控制中心存在 → 变量=1", on_trig.id))
print(string.format("  off1: [%s] 控制中心被摧毁 → 变量=0", off1.id))
print(string.format("  off2: [%s] 归属变更 → 变量=0", off2.id))
print("-----------------------")
print("使用说明：")
print("  在 AI触发 的条件中引用此变量")
print("  变量=1 时表示可以生产遥控坦克")
print("-----------------------")
print("注意：建筑索引 356 (控制中心) 基于原版尤里的复仇")
print("若使用 mod 请自行核对！")
print("=======================")

message_box(string.format("遥控坦克生产条件已成功生成！\n变量 [%d] %s\n\non: 控制中心存在 → 变量=1\noff1: 控制中心被摧毁 → 变量=0\noff2: 归属变更 → 变量=0",
    variable_index, var_name), "执行成功", 1)