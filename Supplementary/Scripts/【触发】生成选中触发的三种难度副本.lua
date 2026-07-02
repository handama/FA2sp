-- 生成选中触发的三种难度副本.lua

-- ========== 自动创建快照 ==========
create_snapshot()

-- ========== 选择触发 ==========

-- 获取所有触发列表
local all_triggers = {}
for i, id in ipairs(get_keys("Triggers")) do
    local name = get_param("Triggers", id, 3)
    table.insert(all_triggers, {id = id, name = name, display = id .. " - " .. name})
end
table.sort(all_triggers, function(a, b) return a.id < b.id end)

local trigger_items = {}
for _, t in ipairs(all_triggers) do
    table.insert(trigger_items, t.display)
end

-- 对话框
local dlg = LuaDialog:new("选择要创建难度副本的触发", false, 620, 440)
dlg:add_label("lbl_tip", "选中触发后将自动创建 Easy/Medium/Hard 三种难度版本", 10, 28, 500, 20)
dlg:add_multilistbox("triggers", "", trigger_items, 10, 40, 580, 300)

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- 解析选中的触发
local selected_values = result.triggers or {}
local selected_ids = {}
local function extract_id(str)
    if type(str) == "string" then
        return str:match("^(%S+)")
    end
    return nil
end
if type(selected_values) == "table" then
    for _, val in ipairs(selected_values) do
        local id = extract_id(val)
        if id then table.insert(selected_ids, id) end
    end
elseif type(selected_values) == "string" and selected_values ~= "" then
    local id = extract_id(selected_values)
    if id then table.insert(selected_ids, id) end
end

if #selected_ids == 0 then
    print("未选择任何触发，脚本已中止")
    return
end

-- ========== 创建三种难度副本 ==========

print(string.format("=======================\n开始处理 %d 个触发...", #selected_ids))

local total_created = 0

for _, id in ipairs(selected_ids) do
    local trigger = get_trigger(id)
    if not trigger then
        print(string.format("  ！ 触发 %s 不存在，跳过", id))
        goto continue
    end

    local ori_name = trigger.name
    -- 获取重复类型（从第一个标签）
    local repeat_type = "0"
    if trigger.tags and #trigger.tags > 0 then
        repeat_type = tostring(trigger.tags[1].type)
    end

    print(string.format("\n[%s] %s", id, ori_name))

    -- 1) 原触发 → Easy
    trigger.easy = true
    trigger.medium = false
    trigger.hard = false
    trigger.name = ori_name .. " - Easy"
    if trigger.tags and #trigger.tags > 0 then
        trigger.tags[1].name = ori_name .. " - Easy 1"
    end
    trigger:apply()
    print(string.format("  √ Easy: %s (ID: %s)", trigger.name, trigger.id))
    total_created = total_created + 1

    -- 2) 创建 Medium 副本
    trigger:change_id(get_free_id())
    trigger.easy = false
    trigger.medium = true
    trigger.hard = false
    trigger.name = ori_name .. " - Medium"
    trigger:delete_tags(false)
    trigger:add_tag("", "", tonumber(repeat_type))
    trigger:apply()
    print(string.format("  √ Medium: %s (ID: %s)", trigger.name, trigger.id))
    total_created = total_created + 1

    -- 3) 创建 Hard 副本
    trigger:change_id(get_free_id())
    trigger.easy = false
    trigger.medium = false
    trigger.hard = true
    trigger.name = ori_name .. " - Hard"
    trigger:delete_tags(false)
    trigger:add_tag("", "", tonumber(repeat_type))
    trigger:apply()
    print(string.format("  √ Hard: %s (ID: %s)", trigger.name, trigger.id))
    total_created = total_created + 1

    ::continue::
end

-- ========== 输出结果 ==========

print(string.format("\n=======================\n完成！共处理 %d 个触发，创建 %d 个副本", #selected_ids, total_created))
print("每个触发已拆分为：Easy(原) / Medium(新) / Hard(新)")
print("=======================")

message_box(string.format("处理完成！\n共处理 %d 个触发\n创建 %d 个难度副本\n\n原触发 → Easy\n新增 → Medium, Hard",
    #selected_ids, total_created), "执行成功", 1)