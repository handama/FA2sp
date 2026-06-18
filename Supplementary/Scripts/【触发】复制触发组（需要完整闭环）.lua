--[[
终极版：触发组完整复制工具
- 支持多次复制（独立模式，始终基于原始触发集）
- 触发名称数字递增（如 000→001→002，忽略后缀）
- 行动中路径点递增（80/107）步长 = 复制次数
- 可独立更改触发国家和作战小队国家
- 可选择是否复制作战小队（包含特遣和脚本）
- 可选择局部变量复制方式
- 全局唯一ID分配器，避免任何ID冲突
]]

-- ==================== 通用辅助函数 ====================
function id_to_num(id) return tonumber(id) or 0 end
function num_to_id(num) return string.format("%08d", num) end

function string_split(str, delim)
    local result = {}
    for part in string.gmatch(str, "([^" .. delim .. "]+)") do
        table.insert(result, part)
    end
    return result
end

function get_all_existing_ids()
    local existing = {}
    local sections = {"Triggers", "Tags", "TeamTypes", "TaskForces", "ScriptTypes"}
    for _, section in ipairs(sections) do
        for id, _ in pairs(get_key_value_pairs(section)) do
            existing[id] = true
        end
    end
    return existing
end

function create_global_id_allocator()
    local existing = get_all_existing_ids()
    local max_num = 0
    for id, _ in pairs(existing) do
        local num = id_to_num(id)
        if num > max_num then max_num = num end
    end
    local next_num = max_num + 1
    return function()
        local new_id = num_to_id(next_num)
        next_num = next_num + 1
        return new_id
    end
end

function replace_ids_by_mapping(str, mapping)
    if not str then return str end
    local result = str
    for old_id, new_id in pairs(mapping) do
        result = string.gsub(result, "(" .. old_id .. ")", new_id)
    end
    return result
end

function replace_local_var_index(str, mapping)
    if not str or not mapping then return str, false end
    local parts = {}
    for part in string.gmatch(str, "([^,]+)") do
        table.insert(parts, part)
    end
    local changed = false
    for i = 1, #parts - 2 do
        local op = parts[i]
        if op == "36" or op == "37" or op == "56" or op == "57" then
            local idx_str = parts[i+2]
            local idx_num = tonumber(idx_str)
            if idx_num and mapping[idx_num] then
                parts[i+2] = tostring(mapping[idx_num])
                changed = true
            end
        end
    end
    if changed then
        return table.concat(parts, ","), true
    end
    return str, false
end

function waypoint_str_to_num(str)
    local num = 0
    for i = 1, #str do
        local c = string.sub(str, i, i)
        local digit = string.byte(c) - string.byte('A')
        num = num * 26 + digit
    end
    return num
end

function waypoint_num_to_str(num)
    if num < 0 then return "" end
    local str = ""
    repeat
        local remainder = num % 26
        str = string.char(remainder + string.byte('A')) .. str
        num = math.floor(num / 26) - 1
    until num < 0
    return str
end

function process_waypoint_increment(action_str, step)
    if not step then step = 1 end
    local parts = {}
    for part in string.gmatch(action_str, "([^,]+)") do
        table.insert(parts, part)
    end
    if #parts < 2 then return action_str end
    local num_actions = tonumber(parts[1])
    if not num_actions then return action_str end
    local idx = 2
    for i = 1, num_actions do
        if idx + 7 <= #parts then
            local op = parts[idx]
            if op == "80" or op == "107" then
                local wp_str = parts[idx+7]
                local wp_num = waypoint_str_to_num(wp_str)
                wp_num = wp_num + step
                parts[idx+7] = waypoint_num_to_str(wp_num)
                print("    路径点递增: " .. wp_str .. " +" .. step .. " -> " .. parts[idx+7])
            end
        end
        idx = idx + 8
    end
    return table.concat(parts, ",")
end

function get_country_list()
    local countries = {}
    local keys = get_values("Countries", "rules+map")
    for _, id in ipairs(keys) do
        local display = translate_house(id)
        if display == "" then display = id end
        table.insert(countries, {id = id, name = display})
    end
    local seen = {}
    local unique = {}
    for _, c in ipairs(countries) do
        if not seen[c.id] then
            seen[c.id] = true
            table.insert(unique, c)
        end
    end
    return unique
end

function select_country(prompt)
    local country_list = get_country_list()
    if #country_list == 0 then
        message_box("未找到任何国家定义，将保留原国家", "警告", 0)
        return nil
    end
    local sel = select_box:new(prompt)
    for _, c in ipairs(country_list) do
        sel:add_option(c.id, c.name .. " (" .. c.id .. ")")
    end
    sel:sort_options(true)
    local chosen = sel:do_modal()
    if chosen == "" then return nil end
    return chosen
end

function increment_trigger_name(name, offset)
    if offset == 0 then return name end
    local new_name = string.gsub(name, "(%d+)", function(num_str)
        local num = tonumber(num_str)
        if num then
            local new_num = num + offset
            local fmt = "%0" .. #num_str .. "d"
            return string.format(fmt, new_num)
        end
        return num_str
    end, 1)
    return new_name
end

function allocate_global_ids(old_set, allocator)
    local mapping = {}
    local order = {}
    for id in pairs(old_set) do
        table.insert(order, id)
    end
    table.sort(order, function(a,b) return id_to_num(a) < id_to_num(b) end)
    for _, old_id in ipairs(order) do
        local new_id = allocator()
        mapping[old_id] = new_id
        print("  分配全局唯一ID: " .. old_id .. " -> " .. new_id)
    end
    return mapping
end

-- ==================== 一次复制执行函数 ====================
function perform_copy(source_triggers, config, existing_reg_idx, global_allocator, name_offset, skip_suffix, waypoint_step)
    -- 读取当前地图数据
    local all = {
        Triggers = {},
        Tags = {},
        TeamTypes = {},
        TaskForces = {},
        ScriptTypes = {},
        VariableNames = {}
    }
    local registry = {
        TeamTypes = {},
        TaskForces = {},
        ScriptTypes = {}
    }

    for id, line in pairs(get_key_value_pairs("Triggers")) do
        local parts = string_split(line, ",")
        all.Triggers[id] = {
            line = line,
            house = parts[1] or "Neutral",
            linked = parts[2] or "<none>",
            name = parts[3] or "",
            disabled = parts[4] or "0",
            easy = parts[5] or "1",
            medium = parts[6] or "1",
            hard = parts[7] or "1"
        }
    end

    for id, line in pairs(get_key_value_pairs("Tags")) do
        local parts = string_split(line, ",")
        if #parts >= 3 then
            all.Tags[id] = {
                line = line,
                repeat_type = parts[1],
                name = parts[2],
                trigger = parts[3]
            }
        end
    end

    local var_data = get_key_value_pairs("VariableNames")
    for idx, line in pairs(var_data) do
        local parts = string_split(line, ",")
        if #parts >= 2 then
            local idx_num = tonumber(idx)
            if idx_num then
                all.VariableNames[idx_num] = { name = parts[1], value = parts[2] or "0" }
            end
        end
    end

    local function load_section_with_registry(section_name, all_table, registry_table)
        local reg = get_key_value_pairs(section_name)
        local ids = {}
        for idx, id in pairs(reg) do
            ids[id] = idx
        end
        registry_table[section_name] = {}
        for idx, id in pairs(reg) do
            registry_table[section_name][idx] = id
        end
        for id, _ in pairs(ids) do
            local content = get_key_value_pairs(id)
            if content and next(content) then
                all_table[id] = content
            end
        end
    end

    load_section_with_registry("TeamTypes", all.TeamTypes, registry)
    load_section_with_registry("TaskForces", all.TaskForces, registry)
    load_section_with_registry("ScriptTypes", all.ScriptTypes, registry)

    -- 收集依赖
    local to_copy = {
        Triggers = {},
        Tags = {},
        TeamTypes = {},
        TaskForces = {},
        ScriptTypes = {}
    }

    local function add_trigger(id)
        if to_copy.Triggers[id] or not all.Triggers[id] then return end
        to_copy.Triggers[id] = all.Triggers[id]
        local linked = all.Triggers[id].linked
        if linked and linked ~= "<none>" and linked ~= "" then
            add_trigger(linked)
        end
    end

    for _, id in ipairs(source_triggers) do
        add_trigger(id)
    end

    -- 应用触发名称递增
    if name_offset > 0 then
        for trig_id, info in pairs(to_copy.Triggers) do
            info.name = increment_trigger_name(info.name, name_offset)
        end
    end

    -- 收集标签
    for tag_id, info in pairs(all.Tags) do
        if to_copy.Triggers[info.trigger] then
            to_copy.Tags[tag_id] = info
        end
    end

    -- 扫描 Events/Actions 中的引用
    local referenced_ids = {}
    local referenced_vars = {}
    local function extract_ids_and_vars(str)
        if not str then return end
        for match in string.gmatch(str, "%d%d%d%d%d%d%d%d") do
            referenced_ids[match] = true
        end
        local parts = {}
        for part in string.gmatch(str, "([^,]+)") do
            table.insert(parts, part)
        end
        for i = 1, #parts - 2 do
            local op = parts[i]
            if op == "36" or op == "37" or op == "56" or op == "57" then
                local idx = tonumber(parts[i+2])
                if idx then referenced_vars[idx] = true end
            end
        end
    end

    for trig_id, _ in pairs(to_copy.Triggers) do
        extract_ids_and_vars(get_string("Events", trig_id))
        extract_ids_and_vars(get_string("Actions", trig_id))
    end

    for id, _ in pairs(referenced_ids) do
        if all.TeamTypes[id] then
            if config.copy_teams then
                to_copy.TeamTypes[id] = all.TeamTypes[id]
                local tf = all.TeamTypes[id].TaskForce
                local sc = all.TeamTypes[id].Script
                if tf and all.TaskForces[tf] then to_copy.TaskForces[tf] = all.TaskForces[tf] end
                if sc and all.ScriptTypes[sc] then to_copy.ScriptTypes[sc] = all.ScriptTypes[sc] end
            end
        elseif all.Triggers[id] then
            if not to_copy.Triggers[id] then add_trigger(id) end
        elseif all.Tags[id] then
            if not to_copy.Tags[id] then to_copy.Tags[id] = all.Tags[id] end
        elseif all.TaskForces[id] then
            if config.copy_teams then to_copy.TaskForces[id] = all.TaskForces[id] end
        elseif all.ScriptTypes[id] then
            if config.copy_teams then to_copy.ScriptTypes[id] = all.ScriptTypes[id] end
        end
    end

    -- 二次标签收集
    for tag_id, info in pairs(all.Tags) do
        if to_copy.Triggers[info.trigger] and not to_copy.Tags[tag_id] then
            to_copy.Tags[tag_id] = info
        end
    end

    if config.copy_teams then
        for team_id, _ in pairs(to_copy.TeamTypes) do
            local tf = all.TeamTypes[team_id].TaskForce
            local sc = all.TeamTypes[team_id].Script
            if tf and not to_copy.TaskForces[tf] and all.TaskForces[tf] then
                to_copy.TaskForces[tf] = all.TaskForces[tf]
            end
            if sc and not to_copy.ScriptTypes[sc] and all.ScriptTypes[sc] then
                to_copy.ScriptTypes[sc] = all.ScriptTypes[sc]
            end
        end
    end

    -- 分配新ID
    local trig_mapping = allocate_global_ids(to_copy.Triggers, global_allocator)
    local tag_mapping = allocate_global_ids(to_copy.Tags, global_allocator)
    local team_mapping = {}
    local tf_mapping = {}
    local sc_mapping = {}
    if config.copy_teams then
        team_mapping = allocate_global_ids(to_copy.TeamTypes, global_allocator)
        tf_mapping = allocate_global_ids(to_copy.TaskForces, global_allocator)
        sc_mapping = allocate_global_ids(to_copy.ScriptTypes, global_allocator)
    else
        for id,_ in pairs(to_copy.TeamTypes) do team_mapping[id]=id end
        for id,_ in pairs(to_copy.TaskForces) do tf_mapping[id]=id end
        for id,_ in pairs(to_copy.ScriptTypes) do sc_mapping[id]=id end
    end

    -- 局部变量处理
    local var_mapping = {}
    if config.copy_vars and next(referenced_vars) then
        local max_var_idx = 0
        for idx in pairs(all.VariableNames) do if idx > max_var_idx then max_var_idx = idx end end
        local next_idx = max_var_idx + 1
        local existing_var = {}
        for idx in pairs(all.VariableNames) do existing_var[tostring(idx)] = true end
        local sorted_old = {}
        for old in pairs(referenced_vars) do table.insert(sorted_old, old) end
        table.sort(sorted_old)
        for _, old_idx in ipairs(sorted_old) do
            while existing_var[tostring(next_idx)] do next_idx = next_idx + 1 end
            var_mapping[old_idx] = next_idx
            local old_var = all.VariableNames[old_idx]
            local new_name = (old_var and old_var.name or "Var"..old_idx)
            if not skip_suffix then
                new_name = new_name .. (config.use_auto and "-copy" or (" " .. config.fixed_suffix))
            end
            write_string("VariableNames", tostring(next_idx), new_name .. "," .. (old_var and old_var.value or "0"))
            existing_var[tostring(next_idx)] = true
            next_idx = next_idx + 1
        end
    end

    local function get_suffix(old_id, section)
        if skip_suffix then return "" end
        if config.use_auto then return "-copy" else return " " .. config.fixed_suffix end
    end

    local function next_reg_idx(reg_name)
        local idx = 1
        while existing_reg_idx[reg_name][tostring(idx)] do idx = idx + 1 end
        existing_reg_idx[reg_name][tostring(idx)] = true
        return idx
    end

    -- 复制 TaskForces / ScriptTypes / TeamTypes
    if config.copy_teams then
        for old_id, content in pairs(to_copy.TaskForces) do
            local new_id = tf_mapping[old_id]
            local reg_idx = next_reg_idx("TaskForces")
            write_string("TaskForces", tostring(reg_idx), new_id)
            for key, value in pairs(content) do
                local v = replace_ids_by_mapping(value, tf_mapping)
                v = replace_ids_by_mapping(v, sc_mapping)
                v = replace_ids_by_mapping(v, team_mapping)
                if key == "Name" then v = v .. get_suffix(old_id, "TaskForces") end
                write_string(new_id, key, v)
            end
        end
        for old_id, content in pairs(to_copy.ScriptTypes) do
            local new_id = sc_mapping[old_id]
            local reg_idx = next_reg_idx("ScriptTypes")
            write_string("ScriptTypes", tostring(reg_idx), new_id)
            for key, value in pairs(content) do
                local v = replace_ids_by_mapping(value, tf_mapping)
                v = replace_ids_by_mapping(v, sc_mapping)
                if key == "Name" then v = v .. get_suffix(old_id, "ScriptTypes") end
                write_string(new_id, key, v)
            end
        end
        for old_id, content in pairs(to_copy.TeamTypes) do
            local new_id = team_mapping[old_id]
            local reg_idx = next_reg_idx("TeamTypes")
            write_string("TeamTypes", tostring(reg_idx), new_id)
            local new_tf = nil
            local new_sc = nil
            if content.TaskForce then
                local old_tf = content.TaskForce
                if config.mode == "both_new" or config.mode == "new_tf_keep_sc" then
                    new_tf = tf_mapping[old_tf]
                else
                    new_tf = old_tf
                end
            end
            if content.Script then
                local old_sc = content.Script
                if config.mode == "both_new" or config.mode == "keep_tf_new_sc" then
                    new_sc = sc_mapping[old_sc]
                else
                    new_sc = old_sc
                end
            end
            for key, value in pairs(content) do
                local v = replace_ids_by_mapping(value, tf_mapping)
                v = replace_ids_by_mapping(v, sc_mapping)
                v = replace_ids_by_mapping(v, team_mapping)
                if key == "Name" then
                    v = v .. get_suffix(old_id, "TeamTypes")
                elseif key == "TaskForce" and new_tf then
                    v = new_tf
                elseif key == "Script" and new_sc then
                    v = new_sc
                elseif key == "House" and config.change_team_country then
                    v = config.new_team_country
                end
                write_string(new_id, key, v)
            end
        end
    end

    -- 复制 Tags
    for old_id, info in pairs(to_copy.Tags) do
        local new_id = tag_mapping[old_id]
        local new_trigger = trig_mapping[info.trigger]
        local name_suffix = get_suffix(info.trigger, "Triggers")
        local new_line = string.format("%s,%s%s,%s", info.repeat_type, info.name, name_suffix, new_trigger)
        write_string("Tags", new_id, new_line)
    end

    -- 复制 Triggers
    for old_id, info in pairs(to_copy.Triggers) do
        local new_id = trig_mapping[old_id]
        local new_linked = info.linked
        if info.linked ~= "<none>" and info.linked ~= "" then
            new_linked = trig_mapping[info.linked]
        end
        local new_name = info.name .. get_suffix(old_id, "Triggers")
        local new_house = info.house
        if config.change_trigger_country then new_house = config.new_trigger_country end
        local new_line = string.format("%s,%s,%s,%s,%s,%s,%s,0",
            new_house, new_linked, new_name, info.disabled, info.easy, info.medium, info.hard)
        write_string("Triggers", new_id, new_line)
    end

    -- 复制 Events / Actions
    for old_id, _ in pairs(to_copy.Triggers) do
        local new_id = trig_mapping[old_id]
        local old_event = get_string("Events", old_id)
        if old_event and old_event ~= "" then
            local new_event = old_event
            if config.copy_vars then new_event, _ = replace_local_var_index(new_event, var_mapping) end
            new_event = replace_ids_by_mapping(new_event, trig_mapping)
            new_event = replace_ids_by_mapping(new_event, tag_mapping)
            new_event = replace_ids_by_mapping(new_event, team_mapping)
            write_string("Events", new_id, new_event)
        end
        local old_action = get_string("Actions", old_id)
        if old_action and old_action ~= "" then
            local new_action = old_action
            if config.copy_vars then new_action, _ = replace_local_var_index(new_action, var_mapping) end
            new_action = replace_ids_by_mapping(new_action, trig_mapping)
            new_action = replace_ids_by_mapping(new_action, tag_mapping)
            new_action = replace_ids_by_mapping(new_action, team_mapping)
            if config.increase_waypoint then
                new_action = process_waypoint_increment(new_action, waypoint_step)
            end
            write_string("Actions", new_id, new_action)
        end
    end

    update_trigger()
    local new_trigger_ids = {}
    for old_id, new_id in pairs(trig_mapping) do
        table.insert(new_trigger_ids, new_id)
    end
    return new_trigger_ids
end

-- ==================== 主流程 ====================
print("========== 终极触发组复制工具 ==========")

local trigger_keys = get_keys("Triggers")
if #trigger_keys == 0 then
    message_box("地图中没有Triggers，无法复制", "错误", 0)
    return
end

local box = multi_select_box:new("请选择要复制的Triggers（可多选）")
for _, id in ipairs(trigger_keys) do
    local name = get_param("Triggers", id, 3)
    box:add_option(id, name .. " (" .. id .. ")")
end
box:sort_options(true)
local original_triggers = box:do_modal()
if #original_triggers == 0 then
    print("未选择任何Trigger，退出")
    return
end

-- 配置收集
local config = {}

-- 后缀方式（仅在数字递增未启用时有效）
local suffix_type = select_box:new("名称后缀方式（当不使用数字递增时生效）")
suffix_type:add_option("auto", "添加后缀 -copy")
suffix_type:add_option("fixed", "固定字符串（手动输入）")
local choice = suffix_type:do_modal()
config.use_auto = (choice == "auto")
config.fixed_suffix = ""
if not config.use_auto then
    config.fixed_suffix = input_box("请输入后缀字符串（例如 _copy）")
    if config.fixed_suffix == "" then config.fixed_suffix = "_copy" end
end

-- 数字递增
local inc_choice = select_box:new("是否启用触发名称数字递增（如 000→001→002）？\n启用后将忽略上述后缀")
inc_choice:add_option("yes", "是")
inc_choice:add_option("no", "否")
local enable_name_increment = (inc_choice:do_modal() == "yes")

-- 触发国家
local tc_choice = select_box:new("是否更改触发的国家？")
tc_choice:add_option("keep", "保留原国家")
tc_choice:add_option("change", "统一改为指定国家")
config.change_trigger_country = false
config.new_trigger_country = ""
if tc_choice:do_modal() == "change" then
    local chosen = select_country("请选择触发的新国家")
    if chosen then
        config.change_trigger_country = true
        config.new_trigger_country = chosen
    end
end

-- 是否复制作战小队
local team_copy_choice = select_box:new("是否复制作战小队（即新增作战小队副本）？")
team_copy_choice:add_option("yes", "是（复制并新增小队）")
team_copy_choice:add_option("no", "否（保留原小队引用，不复制）")
config.copy_teams = (team_copy_choice:do_modal() == "yes")

config.change_team_country = false
config.new_team_country = ""
config.mode = "both_new"
if config.copy_teams then
    local tcm_choice = select_box:new("是否更改作战小队的国家？")
    tcm_choice:add_option("keep", "保留原国家")
    tcm_choice:add_option("change", "统一改为指定国家")
    if tcm_choice:do_modal() == "change" then
        local chosen = select_country("请选择作战小队的新国家")
        if chosen then
            config.change_team_country = true
            config.new_team_country = chosen
        end
    end
    local ref_mode = select_box:new("选择作战小队引用处理方式")
    ref_mode:add_option("both_new", "全部新增（复制特遣和脚本）")
    ref_mode:add_option("keep_tf_new_sc", "保留原特遣小队，新增脚本")
    ref_mode:add_option("new_tf_keep_sc", "新增特遣小队，保留原脚本")
    ref_mode:add_option("both_keep", "全部保留（引用原特遣和脚本）")
    config.mode = ref_mode:do_modal()
end

-- 局部变量
local var_mode = select_box:new("选择局部变量处理方式")
var_mode:add_option("new", "新增局部变量（复制一份，名称加后缀）")
var_mode:add_option("keep", "保留原局部变量索引")
config.copy_vars = (var_mode:do_modal() == "new")

-- 路径点递增
local wp_choice = select_box:new("是否将刷出作战小队的路径点顺序增大（80/107行为）？")
wp_choice:add_option("yes", "是")
wp_choice:add_option("no", "否")
config.increase_waypoint = (wp_choice:do_modal() == "yes")

-- 复制次数
local times_str = input_box("请输入复制次数（例如 2）：")
local times = tonumber(times_str)
if not times or times < 1 then
    message_box("输入无效，将复制1次", "警告", 0)
    times = 1
end

-- 全局资源
local global_allocator = create_global_id_allocator()
local existing_reg_idx = {
    TeamTypes = {}, TaskForces = {}, ScriptTypes = {}
}
local temp_reg = {
    TeamTypes = get_key_value_pairs("TeamTypes"),
    TaskForces = get_key_value_pairs("TaskForces"),
    ScriptTypes = get_key_value_pairs("ScriptTypes")
}
for reg_name, reg in pairs(temp_reg) do
    for idx, _ in pairs(reg) do
        existing_reg_idx[reg_name][tostring(idx)] = true
    end
end

-- 执行多次复制
for i = 1, times do
    print(string.format("\n========== 第 %d 次复制 ==========", i))
    local name_offset = 0
    local skip_suffix = false
    if enable_name_increment then
        name_offset = i
        skip_suffix = true
    end
    local waypoint_step = i   -- 第i次复制，路径点增加i
    local new_ids = perform_copy(original_triggers, config, existing_reg_idx, global_allocator, name_offset, skip_suffix, waypoint_step)
    print(string.format("第 %d 次复制完成，生成了 %d 个新触发", i, #new_ids))
end

print("========== 全部复制完成 ==========")
message_box(string.format("已成功完成 %d 次复制。\n所有新ID均为全局唯一。\n触发名称数字递增已按设置处理（后缀已忽略）。\n路径点递增步长等于复制次数。", times), "完成", 0)