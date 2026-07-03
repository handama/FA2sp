-- 生成字幕队列模板.lua

-- ========== 自动创建快照 ==========

create_snapshot()

-- ========== 获取已有变量 ==========

local var_items = {}
local var_keys = get_keys("VariableNames", "map")
for _, key in ipairs(var_keys) do
    local val = get_string("VariableNames", key, "", "map")
    local name = val:match("^([^,]+)")
    table.insert(var_items, {index = tonumber(key), display = "[" .. key .. "] " .. (name or "")})
end
table.sort(var_items, function(a, b) return a.index < b.index end)

if #var_items == 0 then
    print("地图中暂无局部变量，请先创建变量")
    message_box("地图中暂无局部变量，请先创建变量", "错误", 1)
    return
end

local var_display_items = {}
for _, v in ipairs(var_items) do
    table.insert(var_display_items, v.display)
end

-- ========== 输入对话框 ==========

local dlg = LuaDialog:new("字幕队列生成器", false, 420, 150)

dlg:add_label("lbl_var", "文本锁变量", 10, 27, 80, 20)
dlg:add_combobox("variable", "", var_display_items, var_display_items[1], true, 90, 10, 300, 22)

dlg:add_label("lbl_num", "文本数量", 10, 59, 80, 20)
dlg:add_edit("text_number", "", "3", 90, 40, 50, 22)

dlg:add_label("lbl_time", "间隔(秒)", 200, 59, 70, 20)
dlg:add_edit("time_interval", "", "10", 270, 40, 50, 22)

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- 解析变量
local var_display = result.variable or ""
local variable_index = nil
for _, v in ipairs(var_items) do
    if v.display == var_display then
        variable_index = v.index
        break
    end
end
if not variable_index then
    print("未选择有效的变量，脚本已中止")
    return
end

-- 解析其他输入
local text_number = tonumber((result.text_number or ""):match("^%s*(.-)%s*$") or "")
if not text_number or text_number < 1 or text_number >= 100 then
    print("文本数量无效，使用默认值 3")
    text_number = 3
end

local time_interval = tonumber((result.time_interval or ""):match("^%s*(.-)%s*$") or "")
if not time_interval or time_interval <= 0 then
    print("间隔无效，使用默认值 10 秒")
    time_interval = 10
end

print(string.format("=======================\n文本锁变量: [%d] | 文本数量: %d | 间隔: %d 秒",
    variable_index, text_number, time_interval))

-- ========== 第一阶段：创建文本链触发 ==========

local trigs = {}
for i = 1, text_number do
    local name = "[字幕] - " .. string.format("%02d", i)
    local t = trigger:new()
    t.name = name
    t.country = "Neutral"
    t.easy = true; t.medium = true; t.hard = true; t.disabled = false

    if i == 1 then
        -- 第一个触发：经过时间 + 变量=0 时触发（等待其他文本播放完毕）
        t:add_event("13,0," .. time_interval)
        t:add_event("37,1," .. variable_index)  -- 37,1 = 变量==0 时触发
    else
        t:add_event("13,0," .. time_interval)
    end

    t:add_tag("", name .. "1", 0)
    t:apply()
    trigs[i] = t
    print(string.format("  √ [%s] %s", t.id, name))
end

-- ========== 第二阶段：添加 Actions ==========

for i = 1, text_number - 1 do
    trigs[i]:add_action("11,4,gui:sidebartext,0,0,0,0,A")
    trigs[i]:add_action("53,2," .. trigs[i+1].id .. ",0,0,0,0,A")
    trigs[i]:apply()
end

-- A[1]：额外设置变量=1（锁定，禁止其他文本干扰）
trigs[1]:add_action("56,0," .. variable_index .. ",0,0,0,0,A")
trigs[1]:apply()

-- A[N]：额外设置变量=0（解锁，允许其他文本播放）
trigs[text_number]:add_action("11,4,gui:sidebartext,0,0,0,0,A")
trigs[text_number]:add_action("57,0," .. variable_index .. ",0,0,0,0,A")
trigs[text_number]:apply()

-- ========== 输出结果 ==========

print("=======================")
print(string.format("字幕队列已生成：共 %d 个触发", text_number))
print("触发列表：")
for i = 1, text_number do
    print(string.format("  [%s] %s", trigs[i].id, trigs[i].name))
end
print("-----------------------")
print("播放流程：")
print(string.format("  变量 [%d] = 0 时 → A[1] 触发", variable_index))
print(string.format("  A[1] 设变量=1 (锁定) → A[2] → ... → A[%d]", text_number))
print(string.format("  A[%d] 设变量=0 (解锁)", text_number))
print("-----------------------")
print("注意：需手动修改每段文本内容 (gui:sidebartext)")
print("=======================")

message_box(string.format("字幕队列已成功生成！\n共 %d 个触发\n文本锁: [%d]\nA[1]锁定 → A[%d]解锁",
    text_number, variable_index, text_number), "执行成功", 1)