-- 生成文本列(定时暂停逐帧播放)模版.lua

-- ========== 自动创建快照 ==========

create_snapshot()

-- ========== 输入对话框 ==========

local dlg = LuaDialog:new("文本列(时停)生成器", false, 380, 130)

dlg:add_label("lbl_num", "文本触发数量", 10, 32, 100, 20)
dlg:add_edit("text_number", "", "5", 110, 14, 50, 22)

dlg:add_label("lbl_time", "时停间隔(秒)", 200, 32, 100, 20)
dlg:add_edit("time_interval", "", "4", 290, 14, 50, 22)

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- 解析输入
local text_number = tonumber((result.text_number or ""):match("^%s*(.-)%s*$") or "")
if not text_number or text_number < 1 or text_number >= 100 then
    print("文本数量无效，使用默认值 5")
    text_number = 5
end

local time_interval = tonumber((result.time_interval or ""):match("^%s*(.-)%s*$") or "")
if not time_interval or time_interval <= 0 then
    print("时停间隔无效，使用默认值 4 秒")
    time_interval = 4
end

print(string.format("=======================\n文本数量: %d | 时停间隔: %d 秒", text_number, time_interval))

-- ========== 第一阶段：创建所有触发（逆序命名）==========

-- 触发名称为逆序：t[1]="[文本(时停)] - NN" → t[N]="[文本(时停)] - 01"
-- 链式方向：t[N]→t[N-1]→...→t[2]→t[1]

local trigs = {}
for i = 1, text_number do
    local display_idx = text_number - i + 1
    local name = "[文本(时停)] - " .. string.format("%02d", display_idx)
    local t = trigger:new()
    t.name = name
    t.country = "Neutral"
    t.easy = true
    t.medium = true
    t.hard = true
    t.disabled = true

    t:add_event("13,0,0")
    t:add_tag("", name .. "1", 0)
    t:apply()

    trigs[i] = t
    print(string.format("  √ [%s] %s", t.id, name))
end

-- ========== 第二阶段：添加 Actions（逆序链）==========

for i = 2, text_number do
    trigs[i]:add_action("11,4,gui:sidebartext,0,0,0,0,A")
    trigs[i]:add_action("110,0," .. time_interval .. ",0,0,0,0,A")
    trigs[i]:add_action("19,7,BeaconPlaced,0,0,0,0,A")
    trigs[i]:add_action("53,2," .. trigs[i-1].id .. ",0,0,0,0,A")
    trigs[i]:apply()
end

-- t[1]（显示名=NN）：显示文本 + 时停 + 信标（无启用链）
trigs[1]:add_action("11,4,gui:sidebartext,0,0,0,0,A")
trigs[1]:add_action("110,0," .. time_interval .. ",0,0,0,0,A")
trigs[1]:add_action("19,7,BeaconPlaced,0,0,0,0,A")
trigs[1]:apply()

-- ========== 输出结果 ==========

print("=======================")
print(string.format("文本列(时停)已生成：共 %d 个触发", text_number))
print("触发列表：")
for i = 1, text_number do
    print(string.format("  [%s] %s", trigs[i].id, trigs[i].name))
end
print("-----------------------")
print("播放流程（逆序链）：")
for i = text_number, 2, -1 do
    print(string.format("  [%s] 显示+时停%ds+信标 → 启用下一个", trigs[i].name, time_interval))
end
print(string.format("  [%s] 显示+时停%ds+信标 (最后一个)", trigs[1].name, time_interval))
print("-----------------------")
print("注意：需手动修改每段文本内容 (gui:sidebartext)")
print("=======================")

message_box(string.format("文本列(时停)已成功生成！\n共 %d 个触发\n时停间隔: %d 秒\n\n需手动修改每段文本内容", text_number, time_interval), "执行成功", 1)