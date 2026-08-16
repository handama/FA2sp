-- 脚本名称：搜索触发（按事件/行为类型编号）
-- 功能：弹出对话框，输入事件或行为类型编号（逗号分隔），选择"与/或"模式，
--       搜索所有匹配的触发并以 "ID - 名称" 格式输出。

-- 解析逗号分隔的数字字符串，返回数字集合（table，key为数字）
local function parse_number_list(str)
    local nums = {}
    if str == nil or str == "" then
        return nums
    end
    for token in string.gmatch(str, "%d+") do
        local n = tonumber(token)
        if n then
            nums[n] = true
        end
    end
    return nums
end

-- 从事件/行为表达式字符串中提取第一个数字（类型编号）
-- 例如 "4,0,0" -> 4, "7,1,0,0,0,0,0,A" -> 7
local function extract_type_id(expr)
    local first = string.match(expr, "^(%-?%d+)")
    if first then
        return tonumber(first)
    end
    return nil
end

-- 检查触发是否匹配
-- 参数：
--   trigger_obj - 触发对象
--   event_nums - 要匹配的事件类型编号集合
--   action_nums - 要匹配的行为类型编号集合
--   is_and - true=与模式，false=或模式
-- 返回：boolean
local function is_trigger_match(trigger_obj, event_nums, action_nums, is_and)
    local has_event_filter = false
    local has_action_filter = false
    for _, _ in pairs(event_nums) do has_event_filter = true; break end
    for _, _ in pairs(action_nums) do has_action_filter = true; break end
    
    -- 如果两个过滤条件都为空，则不匹配任何触发
    if not has_event_filter and not has_action_filter then
        return false
    end
    
    -- 收集触发中所有的事件类型和行为类型
    local trigger_event_types = {}
    for _, ev in ipairs(trigger_obj.events) do
        local etype = extract_type_id(ev)
        if etype then
            trigger_event_types[etype] = true
        end
    end
    
    local trigger_action_types = {}
    for _, ac in ipairs(trigger_obj.actions) do
        local atype = extract_type_id(ac)
        if atype then
            trigger_action_types[atype] = true
        end
    end
    
    -- 检查事件是否匹配
    local function check_events()
        if not has_event_filter then return true end
        if is_and then
            -- 与模式：所有指定的事件类型都必须存在
            for eid, _ in pairs(event_nums) do
                if not trigger_event_types[eid] then
                    return false
                end
            end
            return true
        else
            -- 或模式：任意一个指定的事件类型存在即可
            for eid, _ in pairs(event_nums) do
                if trigger_event_types[eid] then
                    return true
                end
            end
            return false
        end
    end
    
    -- 检查行为是否匹配
    local function check_actions()
        if not has_action_filter then return true end
        if is_and then
            -- 与模式：所有指定的行为类型都必须存在
            for aid, _ in pairs(action_nums) do
                if not trigger_action_types[aid] then
                    return false
                end
            end
            return true
        else
            -- 或模式：任意一个指定的行为类型存在即可
            for aid, _ in pairs(action_nums) do
                if trigger_action_types[aid] then
                    return true
                end
            end
            return false
        end
    end
    
    if is_and then
        -- 与模式：事件和行为都要满足各自的条件
        return check_events() and check_actions()
    else
        -- 或模式：事件或行为任意一个满足条件即可
        -- 但如果两个过滤条件都存在，则事件满足 或 行为满足
        local event_ok = check_events()
        local action_ok = check_actions()
        
        if has_event_filter and has_action_filter then
            return event_ok or action_ok
        elseif has_event_filter then
            return event_ok
        else
            return action_ok
        end
    end
end

-- ========== 主逻辑 ==========

-- 创建对话框（使用自动排布模式）
local dlg = LuaDialog:new("搜索触发", true)

dlg:add_edit("events", "事件类型编号（逗号分隔，如 1,4,7）", "")
dlg:add_edit("actions", "行为类型编号（逗号分隔，如 7,14,22）", "")
dlg:add_combobox("mode", "匹配模式", {"AND（与）- 须同时包含所有指定类型", "OR（或）- 包含任意一个指定类型即可"}, "AND（与）- 须同时包含所有指定类型", true)

-- 弹出对话框
local result = dlg:do_modal()

if result == nil then
    print("用户取消了搜索")
    return
end

-- 解析用户输入
local event_nums = parse_number_list(result.events)
local action_nums = parse_number_list(result.actions)
local is_and = (string.find(result.mode, "^AND") ~= nil)

-- 检查是否输入了任何内容
local has_events = false
local has_actions = false
for _, _ in pairs(event_nums) do has_events = true; break end
for _, _ in pairs(action_nums) do has_actions = true; break end

if not has_events and not has_actions then
    print("错误：请至少输入一个事件编号或行为编号！")
    return
end

-- 收集所有触发ID
local all_ids = get_triggers()
if #all_ids == 0 then
    print("地图中没有触发。")
    return
end

-- 遍历并匹配
local matched = {}
for i, id in ipairs(all_ids) do
    local t = get_trigger(id)
    if t then
        if is_trigger_match(t, event_nums, action_nums, is_and) then
            table.insert(matched, {id = id, name = t.name})
        end
    end
    -- 防止界面卡死
    if i % 50 == 0 then
        avoid_time_out()
    end
end

-- 输出结果
print("========================================")
if is_and then
    print("匹配模式：AND（与）")
else
    print("匹配模式：OR（或）")
end

if has_events then
    local ev_list = {}
    for k, _ in pairs(event_nums) do table.insert(ev_list, tostring(k)) end
    table.sort(ev_list)
    print("搜索事件类型：" .. table.concat(ev_list, ", "))
end
if has_actions then
    local ac_list = {}
    for k, _ in pairs(action_nums) do table.insert(ac_list, tostring(k)) end
    table.sort(ac_list)
    print("搜索行为类型：" .. table.concat(ac_list, ", "))
end
print("----------------------------------------")
print("共找到 " .. #matched .. " 个匹配的触发：")
print("----------------------------------------")

if #matched > 0 then
    -- 按ID排序输出
    table.sort(matched, function(a, b)
        -- 尝试按数字排序，否则按字符串排序
        local na = tonumber(a.id)
        local nb = tonumber(b.id)
        if na and nb then
            return na < nb
        else
            return a.id < b.id
        end
    end)
    
    for _, m in ipairs(matched) do
        print(m.id .. " - " .. m.name)
    end
end

print("========================================")
