--【生成选中小队及其特遣脚本的三种难度副本.lua

-- ========== 自动创建快照 ==========

create_snapshot()

-- ========== 选择小队 + 选择是否复制脚本 ==========

-- 获取所有小队列表
local all_teams = {}
for i, id in ipairs(get_values("TeamTypes")) do
    local name = get_string(id, "Name")
    table.insert(all_teams, {id = id, name = name, display = id .. " - " .. name})
end
table.sort(all_teams, function(a, b) return a.id < b.id end)

local team_items = {}
for _, t in ipairs(all_teams) do
    table.insert(team_items, t.display)
end

local dlg = LuaDialog:new("选择要创建难度副本的小队", false, 640, 450)
local y = 10

-- 是否同时复制脚本
dlg:add_label("lbl_hint", "选中后将自动创建 Easy/Medium/Hard 三种难度", 10, y + 18, 500, 20)
y = y + 20
dlg:add_checkbox("copy_script", "同时为关联的脚本创建三种难度副本", true, 10, y + 18, 400, 22)

-- 小队多选
y = y + 28
dlg:add_multilistbox("teams", "", team_items, 10, y, 600, 300)

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- 解析选项
local copy_script = result.copy_script or false

-- 解析选中的小队
local selected_values = result.teams or {}
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
    print("未选择任何小队，脚本已中止")
    return
end

-- ========== 创建三种难度副本 ==========

print(string.format("=======================\n开始处理 %d 个小队...", #selected_ids))

local total_created = 0

for _, id in ipairs(selected_ids) do
    local team = get_team(id)
    if not team then
        print(string.format("  ⚠ 小队 %s 不存在，跳过", id))
        goto continue
    end

    local task = get_task_force(team.task_force)
    local script = get_script(team.script)
    local ori_team_name = team.name
    local ori_task_name = (task and task.name) or ""
    local ori_script_name = (script and script.name) or ""

    print(string.format("\n[%s] %s", id, ori_team_name))

    -- 1) 原小队 → Easy
    team.name = ori_team_name .. " - Easy"
    if task then
        task.name = ori_task_name .. " - Easy"
        task:apply()
    end
    if copy_script and script then
        script.name = ori_script_name .. " - Easy"
        script:apply()
    end
    team:apply()
    print(string.format("  √ Easy: %s (ID: %s)", team.name, team.id))
    total_created = total_created + 1

    -- 2) 创建 Medium 副本
    team.name = ori_team_name .. " - Medium"
    if task then
        task.name = ori_task_name .. " - Medium"
        task:change_id(get_free_id())
        task:apply()
        team.task_force = task.id
    end
    if copy_script and script then
        script.name = ori_script_name .. " - Medium"
        script:change_id(get_free_id())
        team.script = script.id
        script:apply()
    end
    team:change_id(get_free_id())
    team:apply()
    print(string.format("  √ Medium: %s (ID: %s)", team.name, team.id))
    total_created = total_created + 1

    -- 3) 创建 Hard 副本
    team.name = ori_team_name .. " - Hard"
    if task then
        task.name = ori_task_name .. " - Hard"
        task:change_id(get_free_id())
        task:apply()
        team.task_force = task.id
    end
    if copy_script and script then
        script.name = ori_script_name .. " - Hard"
        script:change_id(get_free_id())
        team.script = script.id
        script:apply()
    end
    team:change_id(get_free_id())
    team:apply()
    print(string.format("  √ Hard: %s (ID: %s)", team.name, team.id))
    total_created = total_created + 1

    ::continue::
end

-- ========== 输出结果 ==========

print(string.format("\n=======================\n完成！共处理 %d 个小队，创建 %d 个副本", #selected_ids, total_created))
print("每个小队已拆分为：Easy(原) / Medium(新) / Hard(新)")
if copy_script then
    print("关联脚本：已同步创建三种难度副本")
else
    print("关联脚本：保持不变（所有难度共用同一脚本）")
end
print("=======================")

message_box(string.format("处理完成！\n共处理 %d 个小队\n创建 %d 个难度副本\n原小队 → Easy\n新增 → Medium, Hard\n\n脚本同步: %s",
    #selected_ids, total_created, copy_script and "已开启" or "未开启"), "执行成功", 1)