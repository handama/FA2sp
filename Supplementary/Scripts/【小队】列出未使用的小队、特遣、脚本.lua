-- 功能：查找并输出地图中未被使用的特遣部队、动作脚本、作战小队
-- 遍历所有作战小队，检查特遣部队(TaskForce)和动作脚本(ScriptType)是否被引用
-- 遍历所有触发的行为/事件和AI触发，检查作战小队(TeamType)是否被引用

-----------------------------------------------------------
-- 1. 收集所有已定义的作战小队、特遣部队、动作脚本
-----------------------------------------------------------
local allTeams = get_teams()         -- 数组: id列表
local allTaskForces = get_task_forces()  -- 数组: id列表
local allScripts = get_scripts()     -- 数组: id列表

print(string.format("共发现 %d 个作战小队, %d 个特遣部队, %d 个动作脚本",
    #allTeams, #allTaskForces, #allScripts))

-----------------------------------------------------------
-- 2. 找出被小队引用的特遣部队和动作脚本
-----------------------------------------------------------
local usedTaskForces = {}
local usedScripts = {}

for _, id in ipairs(allTeams) do
    local t = get_team(id)
    if t then
        if t.task_force and t.task_force ~= "" then
            usedTaskForces[t.task_force] = true
        end
        if t.script and t.script ~= "" then
            usedScripts[t.script] = true
        end
    end
end

-- 输出未使用的特遣部队
print("")
print("========== 未使用的特遣部队 (TaskForces) ==========")
print(string.format("总计 %d 个", #allTaskForces))
local unusedTFCount = 0
for _, id in ipairs(allTaskForces) do
    if not usedTaskForces[id] then
        local tf = get_task_force(id)
        local name = tf and tf.name or "(无法读取)"
        print(string.format("  ID=%s  Name=%s", id, name))
        unusedTFCount = unusedTFCount + 1
    end
end
print(string.format("未使用: %d 个", unusedTFCount))

-- 输出未使用的动作脚本
print("")
print("========== 未使用的动作脚本 (ScriptTypes) ==========")
print(string.format("总计 %d 个", #allScripts))
local unusedScriptCount = 0
for _, id in ipairs(allScripts) do
    if not usedScripts[id] then
        local s = get_script(id)
        local name = s and s.name or "(无法读取)"
        print(string.format("  ID=%s  Name=%s", id, name))
        unusedScriptCount = unusedScriptCount + 1
    end
end
print(string.format("未使用: %d 个", unusedScriptCount))

-----------------------------------------------------------
-- 3. 找出被触发和AI触发引用的作战小队
-----------------------------------------------------------
local usedTeams = {}

-- 3a. 遍历所有触发 (Triggers)
local allTriggers = get_triggers()
for _, trigId in ipairs(allTriggers) do
    local trig = get_trigger(trigId)
    if trig then
        -- 3a-i. 遍历每个事件 (Events)
        -- 事件格式: 事件ID, P1, P2, ...
        -- 事件23(作战小队离开地图) -> 23,1,<作战小队id>  -> P3 = 小队ID
        for _, eventStr in ipairs(trig.events) do
            local parts = split_string(eventStr, ",")
            if parts[1] and parts[3] then
                local eventType = parts[1]
                local teamId = parts[3]
                if eventType == "23" and teamId ~= "" then
                    usedTeams[teamId] = true
                end
            end
        end

        -- 3a-ii. 遍历每个行为 (Actions)
        -- 行为格式: 行为ID, P2, P3, P4, P5, P6, P7, P8
        -- parts[1] = 行为类型, parts[3] = P3(小队ID)
        for _, actionStr in ipairs(trig.actions) do
            local parts = split_string(actionStr, ",")
            if parts[1] and parts[3] then
                local actionType = parts[1]
                local teamId = parts[3]
                -- 涉及小队引用的行为类型及对应参数位置:
                -- 类型4(建立作战小队)    -> P3 = 小队ID
                -- 类型5(解散作战小队)    -> P3 = 小队ID
                -- 类型7(援军)            -> P3 = 小队ID
                -- 类型80(援军在路径点)   -> P3 = 小队ID
                -- 类型104(闪烁小队)      -> P3 = 小队ID
                -- 类型105(对话气泡)      -> P3 = 小队ID
                -- 类型107(超时空传送)    -> P3 = 小队ID
                if actionType == "4" or actionType == "5" or actionType == "7"
                    or actionType == "80" or actionType == "104"
                    or actionType == "105" or actionType == "107" then
                    if teamId ~= "" then
                        usedTeams[teamId] = true
                    end
                end
            end
        end
    end
end

-- 3b. 遍历所有AI触发 (AITriggerTypes)
local allAiTriggers = get_ai_triggers()
for _, aiId in ipairs(allAiTriggers) do
    local ai = get_ai_trigger(aiId)
    if ai then
        if ai.team1 and ai.team1 ~= "" then
            usedTeams[ai.team1] = true
        end
        if ai.team2 and ai.team2 ~= "" then
            usedTeams[ai.team2] = true
        end
    end
end

-- 输出未使用的作战小队
print("")
print("========== 未使用的作战小队 (TeamTypes) ==========")
print(string.format("总计 %d 个", #allTeams))
local unusedTeamCount = 0
for _, id in ipairs(allTeams) do
    if not usedTeams[id] then
        local t = get_team(id)
        local name = t and t.name or "(无法读取)"
        print(string.format("  ID=%s  Name=%s", id, name))
        unusedTeamCount = unusedTeamCount + 1
    end
end
print(string.format("未使用: %d 个", unusedTeamCount))

print("")
print("=== 扫描完毕 ===")
