--[[
脚本名称：精确替换Action中的作战小队引用（按比例精确数量）
功能：在选定的触发范围内，将所有Action中对源作战小队ID的引用，
      按用户指定的比例（例如30%）精确替换为目标作战小队ID。
      替换采用随机抽样，确保替换数量与比例严格一致。
]]

-- 辅助函数：获取所有作战小队ID列表（从[TeamTypes]注册表中）
function get_all_team_ids()
    local teams = {}
    local reg = get_key_value_pairs("TeamTypes")
    for idx, team_id in pairs(reg) do
        table.insert(teams, team_id)
    end
    return teams
end

-- 辅助函数：获取作战小队的显示名称（Name字段或ID）
function get_team_display(team_id)
    local name = get_string(team_id, "Name")
    if name and name ~= "" then
        return team_id .. " (" .. name .. ")"
    else
        return team_id
    end
end

-- 辅助函数：获取所有触发的ID列表
function get_all_trigger_ids()
    local triggers = {}
    for id, _ in pairs(get_key_value_pairs("Triggers")) do
        table.insert(triggers, id)
    end
    return triggers
end

-- 在Action字符串中，将指定的源ID替换为目标ID
function replace_team_id_in_action(action_str, src_id, dst_id)
    return string.gsub(action_str, "(" .. src_id .. ")", dst_id)
end

-- 精确替换函数：按比例精确数量替换
function precise_replace_team_in_actions(trigger_ids, src_id, dst_id, ratio)
    -- 收集所有包含源ID的Action
    local candidates = {}
    for _, trig_id in ipairs(trigger_ids) do
        local action_str = get_string("Actions", trig_id)
        if action_str and action_str ~= "" and string.find(action_str, src_id) then
            table.insert(candidates, {trig_id = trig_id, action_str = action_str})
        end
    end
    
    local total = #candidates
    if total == 0 then
        return 0, 0
    end
    
    -- 计算需要替换的数量（四舍五入，至少1个）
    local replace_num = math.floor(total * ratio + 0.5)
    if replace_num == 0 and ratio > 0 then
        replace_num = 1
    elseif replace_num > total then
        replace_num = total
    end
    
    -- 随机选择 replace_num 个不同索引（洗牌算法）
    local indices = {}
    for i = 1, total do indices[i] = i end
    for i = 1, replace_num do
        local j = math.random(i, total)
        indices[i], indices[j] = indices[j], indices[i]
    end
    local selected = {}
    for i = 1, replace_num do
        selected[indices[i]] = true
    end
    
    -- 执行替换
    local replaced = 0
    for i, cand in ipairs(candidates) do
        if selected[i] then
            local new_action = replace_team_id_in_action(cand.action_str, src_id, dst_id)
            write_string("Actions", cand.trig_id, new_action)
            replaced = replaced + 1
            print("触发 " .. cand.trig_id .. " 中的源小队 " .. src_id .. " 已替换为 " .. dst_id)
        end
    end
    
    return replaced, total
end

-- 主流程
print("========== 精确比例替换Action中的作战小队引用开始 ==========")

-- 1. 获取所有作战小队列表
local all_teams = get_all_team_ids()
if #all_teams == 0 then
    message_box("地图中没有找到任何作战小队（[TeamTypes]为空）", "错误", 0)
    return
end

-- 2. 选择源作战小队
local src_box = select_box:new("请选择源作战小队（要被替换的）")
for _, team_id in ipairs(all_teams) do
    src_box:add_option(team_id, get_team_display(team_id))
end
src_box:sort_options(true)
local src_team = src_box:do_modal()
if not src_team or src_team == "" then
    print("未选择源作战小队，退出")
    return
end

-- 3. 选择目标作战小队
local dst_box = select_box:new("请选择目标作战小队（替换为）")
for _, team_id in ipairs(all_teams) do
    dst_box:add_option(team_id, get_team_display(team_id))
end
dst_box:sort_options(true)
local dst_team = dst_box:do_modal()
if not dst_team or dst_team == "" then
    print("未选择目标作战小队，退出")
    return
end

if src_team == dst_team then
    message_box("源小队和目标小队相同，无需替换", "警告", 0)
    return
end

-- 4. 获取替换比例
local ratio_str = input_box("请输入替换比例（0-1之间的小数，例如 0.3 表示30%）：")
local ratio = tonumber(ratio_str)
if not ratio or ratio < 0 or ratio > 1 then
    message_box("输入无效，将使用默认比例 0.3 (30%)", "警告", 0)
    ratio = 0.3
end
print("请求替换比例: " .. ratio * 100 .. "%")

-- 5. 选择触发范围
local scope_choice = select_box:new("选择要处理的触发范围")
scope_choice:add_option("all", "所有触发")
scope_choice:add_option("selected", "手动选择触发")
local scope = scope_choice:do_modal()

local target_triggers = {}
if scope == "all" then
    target_triggers = get_all_trigger_ids()
else
    local multi = multi_select_box:new("请选择要处理的Triggers（可多选）")
    for _, id in ipairs(get_all_trigger_ids()) do
        local name = get_param("Triggers", id, 3)
        multi:add_option(id, name .. " (" .. id .. ")")
    end
    multi:sort_options(true)
    target_triggers = multi:do_modal()
    if #target_triggers == 0 then
        print("未选择任何触发，退出")
        return
    end
end

-- 6. 执行精确替换
math.randomseed(os.time())
local replaced, total = precise_replace_team_in_actions(target_triggers, src_team, dst_team, ratio)

local actual_ratio = (total > 0) and (replaced / total) * 100 or 0
print(string.format("处理完成：共找到 %d 个包含源小队的Action，实际替换了 %d 个（比例 %.1f%%）", total, replaced, actual_ratio))
message_box(string.format("替换完成！\n源小队：%s\n目标小队：%s\n包含源小队的Action总数：%d\n实际替换数量：%d\n实际替换比例：%.1f%%", src_team, dst_team, total, replaced, actual_ratio), "完成", 0)