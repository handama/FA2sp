-- 生成开场结束和任务目标播报模板.lua

-- ========== 自动创建快照 ==========

create_snapshot()

-- ========== 输入对话框 ==========

local dlg = LuaDialog:new("开场/任务目标/胜利播报生成器", false, 225, 120)
dlg:add_label("lbl_num", "任务目标数量（1~9）", 10, 26, 150, 20)
dlg:add_edit("obj_number", "", "1", 140, 8, 40, 22)

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

local obj_number = tonumber((result.obj_number or ""):match("^%s*(.-)%s*$") or "")
if not obj_number or obj_number < 1 or obj_number > 9 then
    print("任务目标数量无效，使用默认值 1")
    obj_number = 1
end

print(string.format("=======================\n任务目标数量: %d", obj_number))

-- ========== 第一阶段：创建所有触发 ==========

-- 难度对应的 (easy, medium, hard)
local diff_flags = {
    {easy = false, medium = false, hard = true},  -- 困难
    {easy = false, medium = true,  hard = false}, -- 正常
    {easy = true,  medium = false, hard = false}, -- 简单
}

-- [1]~[6] 开场流程
local TR = {}
local trigger_names = {
    "[任务目标]0.00开场",           -- 1
    "[任务目标]0.01难度困难",       -- 2
    "[任务目标]0.02难度正常",       -- 3
    "[任务目标]0.03难度简单",       -- 4
    "[任务目标]0.04文本",           -- 5
    "[任务目标]0.05允许操作",       -- 6
}

-- 创建 1~6 (开场)
local trigger_events = {"8,0,0", "13,0,0", "13,0,0", "13,0,0", "13,0,10", "13,0,10"}
for i = 1, 6 do
    local t = trigger:new()
    t.name = trigger_names[i]
    t.country = "Neutral"
    t.easy = true
    t.medium = true
    t.hard = true
    t.disabled = true
    -- 难度选择器使用对应的难度标记
    if i >= 2 and i <= 4 then
        t.easy = diff_flags[i-1].easy
        t.medium = diff_flags[i-1].medium
        t.hard = diff_flags[i-1].hard
    end
    -- 开场(1)和测试(18)的 disabled=false 已在旧版参数中体现
    if i == 1 then t.easy = true; t.medium = true; t.hard = true; end

    t:add_event(trigger_events[i])
    t:add_tag("", trigger_names[i] .. "1", 0)
    t:apply()
    TR[i] = t
    print(string.format("  √ [%s] %s", t.id, t.name))
end

-- 创建 N 组任务目标 (每组 3 个触发: 目标/重复/完成 + 1个变量)
local OBJ = {}
for i = 1, obj_number do
    OBJ[i] = {}
    local names = {
        "[任务目标]" .. i .. ".00任务目标" .. i,
        "[任务目标]" .. i .. ".01任务目标" .. i .. "重复",
        "[任务目标]" .. i .. ".02任务目标" .. i .. "完成",
    }
    local events = {"13,0,10", "13,0,200", "13,0,10"}
    local repeats = {0, 2, 0}

    for j = 0, 2 do -- 0=目标, 1=重复, 2=完成
        local t = trigger:new()
        t.name = names[j+1]
        t.country = "Neutral"
        t.easy = true
        t.medium = true
        t.hard = true
        t.disabled = true
        t:add_event(events[j+1])
        t:add_tag("", names[j+1] .. "1", repeats[j+1])
        t:apply()
        OBJ[i][j] = t
    end

    -- 创建变量
    local var_name = "Mission0" .. i .. "_Acomplished"
    local var_index = tonumber(get_free_key("VariableNames"))
    write_string("VariableNames", tostring(var_index), var_name .. ",0")
    OBJ[i].var_index = var_index
    OBJ[i].var_name = var_name

    print(string.format("  √ [%s/%s/%s] 任务目标%d + 变量[%d]%s",
        OBJ[i][0].id, OBJ[i][1].id, OBJ[i][2].id, i, var_index, var_name))
end

-- [16]~[18] 胜利 + 测试
TR[16] = trigger:new()
TR[16].name = "[胜利]所有任务目标完成允许胜利"
TR[16].country = "Neutral"; TR[16].easy = true; TR[16].medium = true; TR[16].hard = true; TR[16].disabled = true
TR[16]:add_event("13,0,0")

for i = 1, obj_number do
    TR[16]:add_event("36,0," .. OBJ[i].var_index)   -- 局部变量被设置
end

TR[16]:add_tag("", "[胜利]所有任务目标完成允许胜利1", 0)
TR[16]:apply()

TR[17] = trigger:new()
TR[17].name = "[胜利]延迟10秒后胜利"
TR[17].country = "Neutral"; TR[17].easy = true; TR[17].medium = true; TR[17].hard = true; TR[17].disabled = true
TR[17]:add_event("13,0,10")
TR[17]:add_tag("", "[胜利]延迟10秒后胜利1", 0)
TR[17]:apply()

TR[18] = trigger:new()
TR[18].name = "[测试]全图"
TR[18].country = "Neutral"; TR[18].easy = true; TR[18].medium = true; TR[18].hard = true; TR[18].disabled = true
TR[18]:add_event("13,0,0")
TR[18]:add_tag("", "[测试]全图1", 0)
TR[18]:apply()

print(string.format("  √ [%s/%s/%s] 胜利+测试", TR[16].id, TR[17].id, TR[18].id))

-- ========== 第二阶段：添加 Actions ==========

local T = TR

-- 开场 → 启用三个难度选择器
T[1]:add_action("21,6,EVA_EstablishBattlefieldControl,0,0,0,0,A")
T[1]:add_action("11,4,name:testersmap,0,0,0,0,A")
T[1]:add_action("53,2," .. T[2].id .. ",0,0,0,0,A")
T[1]:add_action("53,2," .. T[3].id .. ",0,0,0,0,A")
T[1]:add_action("53,2," .. T[4].id .. ",0,0,0,0,A")
T[1]:add_action("46,0,0,0,0,0,0,A")
T[1]:apply()

-- 难度选择 → 显示对应文本 + 启用文本触发
T[2]:add_action("11,4,txt_hard,0,0,0,0,A")
T[2]:add_action("53,2," .. T[5].id .. ",0,0,0,0,A")
T[2]:apply()

T[3]:add_action("11,4,txt_normal,0,0,0,0,A")
T[3]:add_action("53,2," .. T[5].id .. ",0,0,0,0,A")
T[3]:apply()

T[4]:add_action("11,4,txt_easy,0,0,0,0,A")
T[4]:add_action("53,2," .. T[5].id .. ",0,0,0,0,A")
T[4]:apply()

-- 文本 → 启用允许操作
T[5]:add_action("11,4,gui:sidebartext,0,0,0,0,A")
T[5]:add_action("53,2," .. T[6].id .. ",0,0,0,0,A")
T[5]:apply()

-- 允许操作
T[6]:add_action("11,4,gui:sidebartext,0,0,0,0,A")
T[6]:add_action("47,0,0,0,0,0,0,A")
T[6]:add_action("21,6,EVA_BattlefieldControlOnline,0,0,0,0,A")
T[6]:apply()

-- 任务目标 1~N
for i = 1, obj_number do
    local o = OBJ[i]
    local o_prev = (i > 1) and OBJ[i-1] or nil

    -- i>1: 前一个目标完成 → 启用当前目标
    if o_prev then
        o_prev[2]:add_action("11,4,mission:obj" .. (i-1) .. "comp,0,0,0,0,A")
        o_prev[2]:add_action("54,2," .. o_prev[1].id .. ",0,0,0,0,A")
        o_prev[2]:add_action("53,2," .. o[0].id .. ",0,0,0,0,A")
        o_prev[2]:add_action("21,6,EVA_ObjectiveComplete,0,0,0,0,A")
        o_prev[2]:add_action("56,0," .. o_prev.var_index .. ",0,0,0,0,A")
        o_prev[2]:apply()
    end

    -- 任务目标 N → 启用重复 + 启用完成
    o[0]:add_action("11,4,gui:sidebartext,0,0,0,0,A")
    o[0]:add_action("53,2," .. o[1].id .. ",0,0,0,0,A")
    o[0]:add_action("53,2," .. o[2].id .. ",0,0,0,0,A")
    o[0]:add_action("19,7,BeaconPlaced,0,0,0,0,A")
    o[0]:apply()

    -- 重复显示
    o[1]:add_action("11,4,gui:sidebartext,0,0,0,0,A")
    o[1]:add_action("19,7,BeaconPlaced,0,0,0,0,A")
    o[1]:apply()

    -- 完成 → 禁用重复 + 启用全部完成
    o[2]:add_action("11,4,mission:obj" .. i .. "comp,0,0,0,0,A")
    o[2]:add_action("54,2," .. o[1].id .. ",0,0,0,0,A")
    o[2]:add_action("53,2," .. T[16].id .. ",0,0,0,0,A")
    o[2]:add_action("21,6,EVA_ObjectiveComplete,0,0,0,0,A")
    o[2]:add_action("56,0," .. o.var_index .. ",0,0,0,0,A")
    o[2]:apply()
end

-- 全部完成 → 启用胜利延迟
T[16]:add_action("11,4,gui:sidebartext,0,0,0,0,A")
T[16]:add_action("21,6,EVA_ObjectiveComplete,0,0,0,0,A")
T[16]:add_action("53,2," .. T[17].id .. ",0,0,0,0,A")
T[16]:add_action("19,7,Cheer,0,0,0,0,A")
T[16]:apply()

-- 延迟10秒 → 胜利 (69=胜利)
T[17]:add_action("69,0,0,0,0,0,0,A")
T[17]:apply()

-- 测试全图 (16=全图)
T[18]:add_action("16,0,0,0,0,0,0,A")
T[18]:apply()

-- ========== 输出结果 ==========

print("=======================")
print(string.format("播报系统已生成：共 %d 个触发", 6 + 3 * obj_number + 3))
print("-----------------------")
print("开场流程：")
print("  [0.00] 开场 → 启用难度选择")
print("  [0.01~0.03] 难度选择 → 显示文本 + 启用操作")
print("  [0.04~0.05] 文本 + 允许操作")
print("-----------------------")
for i = 1, obj_number do
    print(string.format("任务目标 %d:", i))
    print(string.format("  [%s] 目标%d → 启用重复/完成", OBJ[i][0].id, i))
    print(string.format("  [%s] 重复显示 (每200帧)", OBJ[i][1].id))
    print(string.format("  [%s] 完成 → 启用下一个/全部完成", OBJ[i][2].id))
end
print("-----------------------")
print("胜利流程：")
print(string.format("  [%s] 全部完成 → 启用胜利延迟", T[16].id))
print(string.format("  [%s] 延迟10s → 胜利", T[17].id))
print(string.format("  [%s] 测试全图", T[18].id))
print("-----------------------")
print("注意：需手动修改文本内容和任务完成条件")
print("=======================")

message_box(string.format("播报系统已成功生成！\n共 %d 个触发\n任务目标: %d 个\n\n需手动修改:\n1. 文本内容 (txt_hard/normal/easy)\n2. 地图名称 (name:testersmap)\n3. 任务完成条件",
    6 + 3 * obj_number + 3, obj_number), "执行成功", 1)