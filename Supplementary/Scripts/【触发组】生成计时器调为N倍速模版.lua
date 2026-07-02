-- 生成计时器调为N倍速模版.lua

-- ========== 自动创建快照 ==========

create_snapshot()

-- ========== 定义触发名称和属性 ==========

local trigger_defs = {
    {name = "[计时器]0.01显示开启",              event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.02占位1(用于对齐帧不可删除)", event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.03计时器生成",              event = "13,0,0",  repeat_type = 2},
    {name = "[计时器]0.04占位2(用于对齐帧不可删除)", event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.05计时器帧对齐",            event = "13,0,1",  repeat_type = 2},
    {name = "[计时器]0.11计时器1s减少1s",          event = "13,0,1",  repeat_type = 2},
    {name = "[计时器]0.12计时器1s减少2s",          event = "13,0,1",  repeat_type = 2},
    {name = "[计时器]0.21计时器1s增加1s",          event = "13,0,1",  repeat_type = 2},
    {name = "[计时器]0.22计时器1s增加2s",          event = "13,0,1",  repeat_type = 2},
    {name = "[计时器]0.31调整计时器为：流逝正向1倍速",  event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.32调整计时器为：流逝正向2倍速",  event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.41调整计时器为：流逝反向1倍速",  event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.42调整计时器为：流逝反向2倍速",  event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.50调整计时器为：流逝暂停",     event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.60显示关闭",               event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.33调整计时器为：流逝正向3倍速",  event = "13,0,0",  repeat_type = 0},
    {name = "[计时器]0.43调整计时器为：流逝反向3倍速",  event = "13,0,0",  repeat_type = 0},
}

-- ========== 第一阶段：创建所有触发并获取 ID ==========

local trig = {}
for i, def in ipairs(trigger_defs) do
    local t = trigger:new()
    t.name = def.name
    t.country = "Neutral"
    t.easy = true
    t.medium = true
    t.hard = true
    t.disabled = true

    t:add_event(def.event)
    t:add_tag("", def.name .. "1", def.repeat_type)
    t:apply()

    trig[i] = t
    print(string.format("  √ [%02d] %s (ID: %s)", i, def.name, t.id))
end

-- 名称速记，方便后续引用
local T = trig

-- ========== 第二阶段：添加 Actions（引用已创建的触发 ID）==========

-- 注意：以下 Actions 格式与旧版 write_string 保持一致，
-- 但去掉了数量前缀（旧版 "2,53,..." → "53,..."）
-- 多个 Action 拆分为独立 add_action 调用

-- [1] 显示开启：启用 计时器生成 + 计时器帧对齐
T[1]:add_action("53,2," .. T[3].id .. ",0,0,0,0,A")
T[1]:add_action("53,2," .. T[5].id .. ",0,0,0,0,A")
T[1]:apply()

-- [2] 占位1：无操作
T[2]:add_action("0,0,0,0,0,0,0,A")
T[2]:apply()

-- [3] 计时器生成：公告 + 语音 + 文字 + 禁用自身
T[3]:add_action("23,0,0,0,0,0,0,A")
T[3]:add_action("27,0,1234,0,0,0,0,A")
T[3]:add_action("103,4,vox:ceva035,0,0,0,0,A")
T[3]:add_action("54,2," .. T[3].id .. ",0,0,0,0,A")
T[3]:apply()

-- [4] 占位2：无操作
T[4]:add_action("0,0,0,0,0,0,0,A")
T[4]:apply()

-- [5] 计时器帧对齐：计时器增加 1
T[5]:add_action("25,0,1,0,0,0,0,A")
T[5]:apply()

-- [6] 计时器1s减少1s
T[6]:add_action("26,0,1,0,0,0,0,A")
T[6]:apply()

-- [7] 计时器1s减少2s
T[7]:add_action("26,0,2,0,0,0,0,A")
T[7]:apply()

-- [8] 计时器1s增加1s
T[8]:add_action("25,0,1,0,0,0,0,A")
T[8]:apply()

-- [9] 计时器1s增加2s
T[9]:add_action("25,0,2,0,0,0,0,A")
T[9]:apply()

-- [10] 正向1倍速：启用6/禁用7/禁用8/禁用9
T[10]:add_action("53,2," .. T[6].id .. ",0,0,0,0,A")  -- 启用减1s
T[10]:add_action("54,2," .. T[7].id .. ",0,0,0,0,A")  -- 禁用减2s
T[10]:add_action("54,2," .. T[8].id .. ",0,0,0,0,A")  -- 禁用加1s
T[10]:add_action("54,2," .. T[9].id .. ",0,0,0,0,A")  -- 禁用加2s
T[10]:apply()

-- [11] 正向2倍速：禁用6/启用7/禁用8/禁用9
T[11]:add_action("54,2," .. T[6].id .. ",0,0,0,0,A")
T[11]:add_action("53,2," .. T[7].id .. ",0,0,0,0,A")
T[11]:add_action("54,2," .. T[8].id .. ",0,0,0,0,A")
T[11]:add_action("54,2," .. T[9].id .. ",0,0,0,0,A")
T[11]:apply()

-- [12] 反向1倍速：禁用6/禁用7/启用8/禁用9
T[12]:add_action("54,2," .. T[6].id .. ",0,0,0,0,A")
T[12]:add_action("54,2," .. T[7].id .. ",0,0,0,0,A")
T[12]:add_action("53,2," .. T[8].id .. ",0,0,0,0,A")
T[12]:add_action("54,2," .. T[9].id .. ",0,0,0,0,A")
T[12]:apply()

-- [13] 反向2倍速：禁用6/禁用7/禁用8/启用9
T[13]:add_action("54,2," .. T[6].id .. ",0,0,0,0,A")
T[13]:add_action("54,2," .. T[7].id .. ",0,0,0,0,A")
T[13]:add_action("54,2," .. T[8].id .. ",0,0,0,0,A")
T[13]:add_action("53,2," .. T[9].id .. ",0,0,0,0,A")
T[13]:apply()

-- [16] 正向3倍速：启用6/启用7/禁用8/禁用9
T[16]:add_action("53,2," .. T[6].id .. ",0,0,0,0,A")
T[16]:add_action("53,2," .. T[7].id .. ",0,0,0,0,A")
T[16]:add_action("54,2," .. T[8].id .. ",0,0,0,0,A")
T[16]:add_action("54,2," .. T[9].id .. ",0,0,0,0,A")
T[16]:apply()

-- [17] 反向3倍速：禁用6/禁用7/启用8/启用9
T[17]:add_action("54,2," .. T[6].id .. ",0,0,0,0,A")
T[17]:add_action("54,2," .. T[7].id .. ",0,0,0,0,A")
T[17]:add_action("53,2," .. T[8].id .. ",0,0,0,0,A")
T[17]:add_action("53,2," .. T[9].id .. ",0,0,0,0,A")
T[17]:apply()

-- [14] 暂停：禁用6/禁用7/禁用8/禁用9
T[14]:add_action("54,2," .. T[6].id .. ",0,0,0,0,A")
T[14]:add_action("54,2," .. T[7].id .. ",0,0,0,0,A")
T[14]:add_action("54,2," .. T[8].id .. ",0,0,0,0,A")
T[14]:add_action("54,2," .. T[9].id .. ",0,0,0,0,A")
T[14]:apply()

-- [15] 显示关闭：禁用5/禁用6/禁用7/禁用8/禁用9 + 禁用自身
T[15]:add_action("24,0,0,0,0,0,0,A")
T[15]:add_action("54,2," .. T[5].id .. ",0,0,0,0,A")
T[15]:add_action("54,2," .. T[6].id .. ",0,0,0,0,A")
T[15]:add_action("54,2," .. T[7].id .. ",0,0,0,0,A")
T[15]:add_action("54,2," .. T[8].id .. ",0,0,0,0,A")
T[15]:add_action("54,2," .. T[9].id .. ",0,0,0,0,A")
T[15]:apply()

-- ========== 输出结果 ==========

print("=======================")
print("计时器系统已生成：共 17 个触发")
print(string.format("  ID区间: %s ~ %s", T[1].id, T[17].id))
print("-----------------------")
print("触发列表：")
for i, def in ipairs(trigger_defs) do
    print(string.format("  [%s] %s", T[i].id, def.name))
end
print("-----------------------")
print("速度档位说明：")
print("  启用 [1]显示开启  → 启动计时器")
print("  启用 [10~12]正向1~3倍速 / [13~15]反向1~3倍速")
print("  启用 [16]暂停 → 暂停计时")
print("  启用 [17]显示关闭 → 隐藏计时器")
print("=======================")

message_box(string.format("计时器系统已成功生成！\n共 17 个触发\nID区间: %s ~ %s\n\n启用 [1]显示开启 启动计时器", T[1].id, T[17].id), "执行成功", 1)
