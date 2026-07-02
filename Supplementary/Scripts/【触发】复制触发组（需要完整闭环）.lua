-- 终极触发组复制工具（全窗口合并版 - 集成简称输入，已修正重复后缀）
-- 所有配置集成在单一对话框，支持国家替换、名称修改、路径点递增、多次复制等
-- 触发名称仅处理一次：在 perform_copy 开头的循环中统一修改，后续直接使用 info.name

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

function process_waypoint_increment(action_str, step, keep_waypoints)
    if not keep_waypoints or next(keep_waypoints) == nil then return action_str end
    local parts = {}
    for part in string.gmatch(action_str, "([^,]+)") do
        table.insert(parts, part)
    end
    if #parts < 2 then return action_str end
    local num_actions = tonumber(parts[1])
    if not num_actions then return action_str end
    local idx = 2
    local action_index = 1
    for i = 1, num_actions do
        if idx + 7 <= #parts then
            local op = parts[idx]
            if op == "17" or op == "41" or op == "42" or op == "80" or op == "107" or op == "108" then
                if keep_waypoints[action_index] then
                    local wp_str = parts[idx+7]
                    local wp_num = string_to_waypoint(wp_str)
                    wp_num = wp_num + step
                    parts[idx+7] = waypoint_to_string(wp_num)
                    print("    路径点递增: " .. wp_str .. " +" .. step .. " -> " .. parts[idx+7])
                end
            end
            action_index = action_index + 1
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

-- ==================== 国家映射表与替换辅助 ====================
function get_country_map()
    return {
        ["Americans"]   = {cn = "美", abbr = "Am"},
        ["Alliance"]    = {cn = "韩", abbr = "Al"},
        ["French"]      = {cn = "法", abbr = "Fr"},
        ["Germans"]     = {cn = "德", abbr = "Ge"},
        ["British"]     = {cn = "英", abbr = "Br"},
        ["Africans"]    = {cn = "利比", abbr = "Af"},
        ["Arabs"]       = {cn = "伊拉", abbr = "Ar"},
        ["Confederation"] = {cn = "古", abbr = "Co"},
        ["Russians"]    = {cn = "苏", abbr = "Ru"},
        ["YuriCountry"] = {cn = "尤", abbr = "Yr"},
        ["Neutral"]     = {cn = "平", abbr = "Ne"},
        ["Special"]     = {cn = "特", abbr = "Sp"},
    }
end

function build_replacement_map_from_input(src_id, dst_id, src_cn, src_abbr, dst_cn, dst_abbr)
    local map = get_country_map()
    local src_info = map[src_id]
    local dst_info = map[dst_id]
    local src_cn_final = src_info and src_info.cn or src_cn
    local src_abbr_final = src_info and src_info.abbr or src_abbr
    local dst_cn_final = dst_info and dst_info.cn or dst_cn
    local dst_abbr_final = dst_info and dst_info.abbr or dst_abbr
    return {
        {src = src_cn_final, dst = dst_cn_final},
        {src = src_abbr_final, dst = dst_abbr_final}
    }
end

function apply_replacements(str, replacements)
    local result = str
    for _, rep in ipairs(replacements) do
        result = string.gsub(result, rep.src, rep.dst)
    end
    return result
end

function increment_trigger_name(name, offset, digit_info)
    if offset == 0 then return name end
    -- 查找所有数字串及其位置
    local digits = {}
    local pos = 1
    while true do
        local s, e = string.find(name, "%d+", pos)
        if not s then break end
        local str = name:sub(s, e)
        table.insert(digits, {start = s, finish = e, str = str})
        pos = e + 1
    end
    if #digits == 0 then return name end
    -- 取最后一个数字串
    local last = digits[#digits]
    local num = tonumber(last.str)
    if not num then return name end
    local new_num = num + offset
    local width = digit_info and digit_info.width or #last.str
    local fmt = "%0" .. width .. "d"
    local new_str = string.format(fmt, new_num)
    -- 替换原字符串中的该数字串
    return name:sub(1, last.start - 1) .. new_str .. name:sub(last.finish + 1)
end

-- ==================== 一次复制执行函数 ====================
function perform_copy(source_triggers, config, existing_reg_idx, global_allocator, waypoint_step, keep_waypoints, name_offset)
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

    -- 建立现有对象名称->ID映射（用于查找复用）
    local existing_team_by_name = {}
    local existing_var_by_name = {}
    if config.use_existing_team then
        for id, content in pairs(all.TeamTypes) do
            if content.Name then
                existing_team_by_name[content.Name] = id
            end
        end
    end
    if config.use_existing_var then
        for idx, var in pairs(all.VariableNames) do
            existing_var_by_name[var.name] = idx
        end
    end

    -- 收集依赖
    local to_copy = {
        Triggers = {},
        Tags = {},
        TeamTypes = {},
        TaskForces = {},
        ScriptTypes = {}
    }

    local existing_team_mapping = {}
    local existing_var_mapping = {}

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

    -- ===== 统一处理触发名称（国家替换 / 数字递增 / 后缀） - 仅一次 =====
    for trig_id, info in pairs(to_copy.Triggers) do
        if config.replace_country then
            local old_name = info.name
            info.name = apply_replacements(info.name, config.replacements)
            if old_name ~= info.name then
                print("  触发名称替换: " .. old_name .. " -> " .. info.name)
            end
        end
        if config.digit_info and name_offset > 0 then
            info.name = increment_trigger_name(info.name, name_offset, config.digit_info)
            print("  触发名称递增: " .. info.name)
        end
        if not config.replace_country and not config.digit_info then
            if config.use_auto then
                info.name = info.name .. "-copy"
            elseif config.fixed_suffix and config.fixed_suffix ~= "" then
                info.name = info.name .. " " .. config.fixed_suffix
            end
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

    -- 处理引用
    for id, _ in pairs(referenced_ids) do
        if all.TeamTypes[id] then
            if config.copy_teams then
                if config.use_existing_team and config.replace_country then
                    local old_name = all.TeamTypes[id].Name
                    if old_name then
                        local new_name = apply_replacements(old_name, config.replacements)
                        local existing_id = existing_team_by_name[new_name]
                        if existing_id then
                            existing_team_mapping[id] = existing_id
                            print("  使用现有小队: " .. id .. " -> " .. existing_id .. " (名称匹配: " .. new_name .. ")")
                            goto continue_team
                        else
                            print("  未找到匹配现有小队，将复制: " .. id .. " (查找名: " .. new_name .. ")")
                        end
                    else
                        print("  原小队无Name字段，将复制: " .. id)
                    end
                end
                to_copy.TeamTypes[id] = all.TeamTypes[id]
                local tf = all.TeamTypes[id].TaskForce
                local sc = all.TeamTypes[id].Script
                if tf and all.TaskForces[tf] then to_copy.TaskForces[tf] = all.TaskForces[tf] end
                if sc and all.ScriptTypes[sc] then to_copy.ScriptTypes[sc] = all.ScriptTypes[sc] end
            end
            ::continue_team::
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

    -- 处理 Variables 的查找复用
    if config.use_existing_var and config.replace_country then
        for old_idx, _ in pairs(referenced_vars) do
            local old_var = all.VariableNames[old_idx]
            if old_var then
                local new_name = apply_replacements(old_var.name, config.replacements)
                local existing_idx = existing_var_by_name[new_name]
                if existing_idx then
                    existing_var_mapping[old_idx] = existing_idx
                    print("  使用现有变量: " .. old_idx .. " -> " .. existing_idx .. " (名称匹配: " .. new_name .. ")")
                else
                    print("  未找到匹配现有变量，将复制: " .. old_idx .. " (查找名: " .. new_name .. ")")
                end
            else
                print("  原变量不存在，将复制: " .. old_idx)
            end
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

    -- 合并现有小队映射
    for old_id, existing_id in pairs(existing_team_mapping) do
        team_mapping[old_id] = existing_id
    end

    -- 局部变量处理
    local var_mapping = {}
    if config.copy_vars and next(referenced_vars) then
        local max_var_idx = 0
        for idx in pairs(all.VariableNames) do if idx > max_var_idx then max_var_idx = idx end end
        local next_idx = max_var_idx + 1
        local existing_var = {}
        for idx in pairs(all.VariableNames) do existing_var[tostring(idx)] = true end
        for _, existing_idx in pairs(existing_var_mapping) do
            existing_var[tostring(existing_idx)] = true
        end
        local sorted_old = {}
        for old in pairs(referenced_vars) do
            if not existing_var_mapping[old] then
                table.insert(sorted_old, old)
            end
        end
        table.sort(sorted_old)
        for _, old_idx in ipairs(sorted_old) do
            while existing_var[tostring(next_idx)] do next_idx = next_idx + 1 end
            var_mapping[old_idx] = next_idx
            local old_var = all.VariableNames[old_idx]
            local new_name = old_var and old_var.name or "Var"..old_idx
            if config.replace_country then
                new_name = apply_replacements(new_name, config.replacements)
            end
            if config.digit_info and name_offset > 0 then
                new_name = increment_trigger_name(new_name, name_offset, config.digit_info)
            end
            if not config.replace_country and not config.digit_info then
                if config.use_auto then
                    new_name = new_name .. "-copy"
                elseif config.fixed_suffix and config.fixed_suffix ~= "" then
                    new_name = new_name .. " " .. config.fixed_suffix
                end
            end
            write_string("VariableNames", tostring(next_idx), new_name .. "," .. (old_var and old_var.value or "0"))
            existing_var[tostring(next_idx)] = true
            next_idx = next_idx + 1
        end
    end

    -- 合并现有变量映射
    for old_idx, existing_idx in pairs(existing_var_mapping) do
        var_mapping[old_idx] = existing_idx
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
                if key == "Name" then
                    if config.replace_country then
                        v = apply_replacements(v, config.replacements)
                    end
                    if config.digit_info and name_offset > 0 then
                        v = increment_trigger_name(v, name_offset, config.digit_info)
                    end
                    if not config.replace_country and not config.digit_info then
                        if config.use_auto then
                            v = v .. "-copy"
                        elseif config.fixed_suffix and config.fixed_suffix ~= "" then
                            v = v .. " " .. config.fixed_suffix
                        end
                    end
                end
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
                if key == "Name" then
                    if config.replace_country then
                        v = apply_replacements(v, config.replacements)
                    end
                    if config.digit_info and name_offset > 0 then
                        v = increment_trigger_name(v, name_offset, config.digit_info)
                    end
                    if not config.replace_country and not config.digit_info then
                        if config.use_auto then
                            v = v .. "-copy"
                        elseif config.fixed_suffix and config.fixed_suffix ~= "" then
                            v = v .. " " .. config.fixed_suffix
                        end
                    end
                end
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
                    if config.replace_country then
                        v = apply_replacements(v, config.replacements)
                    end
                    if config.digit_info and name_offset > 0 then
                        v = increment_trigger_name(v, name_offset, config.digit_info)
                    end
                    if not config.replace_country and not config.digit_info then
                        if config.use_auto then
                            v = v .. "-copy"
                        elseif config.fixed_suffix and config.fixed_suffix ~= "" then
                            v = v .. " " .. config.fixed_suffix
                        end
                    end
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
        local new_name = info.name
        if config.replace_country then
            new_name = apply_replacements(new_name, config.replacements)
        end
        if config.digit_info and name_offset > 0 then
            new_name = increment_trigger_name(new_name, name_offset, config.digit_info)
        end
        if not config.replace_country and not config.digit_info then
            if config.use_auto then
                new_name = new_name .. "-copy"
            elseif config.fixed_suffix and config.fixed_suffix ~= "" then
                new_name = new_name .. " " .. config.fixed_suffix
            end
        end
        local new_line = string.format("%s,%s,%s", info.repeat_type, new_name, new_trigger)
        write_string("Tags", new_id, new_line)
    end

    -- 复制 Triggers（直接使用已处理好的 info.name）
    for old_id, info in pairs(to_copy.Triggers) do
        local new_id = trig_mapping[old_id]
        local new_linked = info.linked
        if info.linked ~= "<none>" and info.linked ~= "" then
            new_linked = trig_mapping[info.linked]
        end
        local new_name = info.name  -- 已在上方统一处理，直接使用
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
                local trig_keep = keep_waypoints[old_id] or {}
                new_action = process_waypoint_increment(new_action, waypoint_step, trig_keep)
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

-- ==================== 主流程（对话框） ====================
print("========== 终极触发组复制工具（最终版 - 无重复后缀） ==========")

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

-- 预扫描路径点候选
local waypoint_candidates = {}
for _, trig_id in ipairs(original_triggers) do
    local action_str = get_string("Actions", trig_id)
    if action_str and action_str ~= "" then
        local parts = string_split(action_str, ",")
        local num_actions = tonumber(parts[1])
        if num_actions then
            local idx = 2
            for i = 1, num_actions do
                if idx + 7 <= #parts then
                    local op = parts[idx]
                    if op == "17" or op == "41" or op == "42" or op == "80" or op == "107" or op == "108" then
                        local trig_name = get_param("Triggers", trig_id, 3)
                        table.insert(waypoint_candidates, {
                            display = string.format("触发 %s  行为%d (op=%s)  路径点=%s", trig_name, i, op, parts[idx+7]),
                            trig_id = trig_id,
                            action_index = i
                        })
                    end
                    idx = idx + 8
                end
            end
        end
    end
end

-- 创建对话框
local dlg = LuaDialog:new("复制触发组设置", false, 580, 820)

-- 国家列表
local country_items = {}
local country_list = get_country_list()
for _, c in ipairs(country_list) do
    table.insert(country_items, c.id .. " (" .. c.name .. ")")
end

-- 1. 国家替换
dlg:add_checkbox("replace_country", "启用国家替换（替换触发、作战小队、局部变量名称里的中/英文缩写，并更改触发的国家）", false, 10, 10, 540, 20)

-- 2. 源国家
dlg:add_combobox("src_country", "原触发的国家", country_items, "", false, 10, 35, 540, 22)
-- 3. 目标国家
dlg:add_combobox("dst_country", "复制后触发的国家", country_items, "", false, 10, 71, 540, 22)

-- 4. 源国家中文简称
dlg:add_edit("src_cn", "原触发名中的国家中文简称", "", 10, 107, 250, 20)
-- 5. 源国家英文缩写
dlg:add_edit("src_abbr", "原触发名中的国家英文缩写", "", 300, 107, 250, 20)
-- 6. 目标国家中文简称
dlg:add_edit("dst_cn", "复制后触发的国家中文简称", "", 10, 143, 250, 20)
-- 7. 目标国家英文缩写
dlg:add_edit("dst_abbr", "复制后触发的国家英文缩写", "", 300, 143, 250, 20)

-- 初始时禁用简称输入
dlg:set_enabled("src_cn", false)
dlg:set_enabled("src_abbr", false)
dlg:set_enabled("dst_cn", false)
dlg:set_enabled("dst_abbr", false)

-- 8. 名称修改方式
local name_modes = {"添加后缀 -copy", "手动输入后缀", "触发名中的数字递增"}
dlg:add_combobox("name_mode", "名称修改方式", name_modes, "添加后缀 -copy", true, 10, 186, 540, 22)

-- 9. 自定义后缀输入
dlg:add_edit("suffix_input", "自定义后缀", "_copy", 10, 223, 540, 20)
dlg:set_enabled("suffix_input", false)

-- 10. 起始数字输入
dlg:add_edit("digit_input", "起始数字（如007或07或7）", "0", 10, 260, 540, 20)
dlg:set_enabled("digit_input", false)

-- 11. 从已有作战小队中寻找
dlg:add_checkbox("use_team", "先从已有作战小队中寻找，若找不到则复制新增", false, 10, 305, 540, 20)
dlg:set_visible("use_team", false)

-- 12. 从已有局部变量中寻找
dlg:add_checkbox("use_var", "先从已有局部变量中寻找，若找不到则复制新增", false, 10, 325, 540, 20)
dlg:set_visible("use_var", false)

-- 13. 复制作战小队
dlg:add_checkbox("copy_teams", "新增作战小队", true, 10, 355, 540, 20)

-- 14. 小队引用方式
local ref_modes = {"新增特遣，新增脚本", "保留原特遣，新增脚本", "新增特遣，保留原脚本", "保留原特遣，保留原脚本"}
dlg:add_combobox("team_ref_mode", "默认作战小队新增方式", ref_modes, "新增特遣，新增脚本", true, 10, 385, 540, 22)

-- 15. 局部变量处理
local var_modes = {"新增局部变量", "保留原局部变量"}
dlg:add_combobox("var_mode", "默认局部变量处理方式", var_modes, "新增局部变量", true, 10, 425, 540, 22)

-- 16. 路径点递增
dlg:add_checkbox("wp_enable", "是否对路径点行为进行递增", false, 10, 465, 200, 20)
dlg:add_checkbox("wp_select_all", "全选", false, 220, 465, 100, 20)
dlg:set_enabled("wp_select_all", false)

local wp_display_items = {}
for _, cand in ipairs(waypoint_candidates) do
    table.insert(wp_display_items, cand.display)
end
dlg:add_multilistbox("wp_list", "选择需要递增路径点的行为", wp_display_items, 10, 495, 540, 120)
dlg:set_enabled("wp_list", false)

-- 17. 复制次数
dlg:add_edit("copy_times", "复制次数", "1", 10, 635, 200, 20)

-- 18. 配置摘要
dlg:add_label("summary", "配置摘要：", 10, 680, 540, 80)

-- ==================== 事件绑定 ====================
local function update_country_inputs(dlg)
    if not dlg:get_bool("replace_country") then
        dlg:set_enabled("src_cn", false)
        dlg:set_enabled("src_abbr", false)
        dlg:set_enabled("dst_cn", false)
        dlg:set_enabled("dst_abbr", false)
        dlg:set_text("src_cn", "")
        dlg:set_text("src_abbr", "")
        dlg:set_text("dst_cn", "")
        dlg:set_text("dst_abbr", "")
        return
    end

    local src_text = dlg:get_string("src_country") or ""
    local dst_text = dlg:get_string("dst_country") or ""
    local src_id = string.match(src_text, "^([^ ]+)") or ""
    local dst_id = string.match(dst_text, "^([^ ]+)") or ""
    local map = get_country_map()
    local src_info = map[src_id]
    local dst_info = map[dst_id]

    if src_info then
        dlg:set_text("src_cn", src_info.cn)
        dlg:set_text("src_abbr", src_info.abbr)
        dlg:set_enabled("src_cn", false)
        dlg:set_enabled("src_abbr", false)
    else
        dlg:set_text("src_cn", "")
        dlg:set_text("src_abbr", "")
        dlg:set_enabled("src_cn", true)
        dlg:set_enabled("src_abbr", true)
    end

    if dst_info then
        dlg:set_text("dst_cn", dst_info.cn)
        dlg:set_text("dst_abbr", dst_info.abbr)
        dlg:set_enabled("dst_cn", false)
        dlg:set_enabled("dst_abbr", false)
    else
        dlg:set_text("dst_cn", "")
        dlg:set_text("dst_abbr", "")
        dlg:set_enabled("dst_cn", true)
        dlg:set_enabled("dst_abbr", true)
    end
end

dlg:on_event("src_country", "editchange", function(key) update_country_inputs(dlg) end)
dlg:on_event("src_country", "selchange", function(key) update_country_inputs(dlg) end)
dlg:on_event("dst_country", "editchange", function(key) update_country_inputs(dlg) end)
dlg:on_event("dst_country", "selchange", function(key) update_country_inputs(dlg) end)

dlg:on_event("replace_country", "changed", function(key)
    local is_replace = dlg:get_bool("replace_country")
    dlg:set_enabled("src_country", is_replace)
    dlg:set_enabled("dst_country", is_replace)
    dlg:set_visible("use_team", is_replace)
    dlg:set_visible("use_var", is_replace)
    dlg:set_enabled("name_mode", not is_replace)
    if is_replace then
        dlg:set_enabled("suffix_input", false)
        dlg:set_enabled("digit_input", false)
    else
        local mode = dlg:get_string("name_mode")
        dlg:set_enabled("suffix_input", mode == "手动输入后缀")
        dlg:set_enabled("digit_input", mode == "触发名中的数字递增")
    end
    if is_replace then
        update_country_inputs(dlg)
        dlg:set_enabled("src_cn", true)
        dlg:set_enabled("src_abbr", true)
        dlg:set_enabled("dst_cn", true)
        dlg:set_enabled("dst_abbr", true)
        update_country_inputs(dlg)
    else
        dlg:set_enabled("src_cn", false)
        dlg:set_enabled("src_abbr", false)
        dlg:set_enabled("dst_cn", false)
        dlg:set_enabled("dst_abbr", false)
        dlg:set_text("src_cn", "")
        dlg:set_text("src_abbr", "")
        dlg:set_text("dst_cn", "")
        dlg:set_text("dst_abbr", "")
    end
    update_summary(dlg)
end)

dlg:on_event("name_mode", "selchange", function(key)
    local mode = dlg:get_string("name_mode")
    dlg:set_enabled("suffix_input", mode == "手动输入后缀")
    dlg:set_enabled("digit_input", mode == "触发名中的数字递增")
    update_summary(dlg)
end)

dlg:on_event("wp_enable", "changed", function(key)
    local enabled = dlg:get_bool("wp_enable")
    dlg:set_enabled("wp_select_all", enabled)
    dlg:set_enabled("wp_list", enabled)
    if not enabled then dlg:set_check("wp_select_all", false) end
    update_summary(dlg)
end)

dlg:on_event("wp_select_all", "changed", function(key) update_summary(dlg) end)
dlg:on_event("use_team", "changed", function(key) update_summary(dlg) end)
dlg:on_event("use_var", "changed", function(key) update_summary(dlg) end)
dlg:on_event("copy_teams", "changed", function(key)
    dlg:set_enabled("team_ref_mode", dlg:get_bool("copy_teams"))
    update_summary(dlg)
end)
dlg:on_event("team_ref_mode", "selchange", function(key) update_summary(dlg) end)
dlg:on_event("var_mode", "selchange", function(key) update_summary(dlg) end)
dlg:on_event("suffix_input", "change", function(key) update_summary(dlg) end)
dlg:on_event("digit_input", "change", function(key) update_summary(dlg) end)
dlg:on_event("copy_times", "change", function(key) update_summary(dlg) end)

function update_summary(dlg)
    local lines = {}
    local replace = dlg:get_bool("replace_country")
    if replace then
        local src = dlg:get_string("src_country")
        local dst = dlg:get_string("dst_country")
        local src_cn = dlg:get_string("src_cn")
        local src_abbr = dlg:get_string("src_abbr")
        local dst_cn = dlg:get_string("dst_cn")
        local dst_abbr = dlg:get_string("dst_abbr")
        lines[#lines+1] = "国家替换: 是 | 源: " .. src .. " | 目标: " .. dst
        lines[#lines+1] = "中文简称: " .. src_cn .. " → " .. dst_cn
        lines[#lines+1] = "英文缩写: " .. src_abbr .. " → " .. dst_abbr
        lines[#lines+1] = "从已有小队查找: " .. (dlg:get_bool("use_team") and "是" or "否")
        lines[#lines+1] = "从已有变量查找: " .. (dlg:get_bool("use_var") and "是" or "否")
    else
        local mode = dlg:get_string("name_mode")
        lines[#lines+1] = "名称修改: " .. mode
        if mode == "手动输入后缀" then lines[#lines+1] = "  后缀: " .. dlg:get_string("suffix_input") end
        if mode == "触发名中的数字递增" then lines[#lines+1] = "  起始数字: " .. dlg:get_string("digit_input") end
    end
    lines[#lines+1] = "复制作战小队: " .. (dlg:get_bool("copy_teams") and "是" or "否")
    if dlg:get_bool("copy_teams") then
        lines[#lines+1] = "  引用方式: " .. dlg:get_string("team_ref_mode")
    end
    lines[#lines+1] = "局部变量: " .. dlg:get_string("var_mode")
    local wp_enabled = dlg:get_bool("wp_enable")
    lines[#lines+1] = "路径点递增: " .. (wp_enabled and "是" or "否")
    if wp_enabled then
        lines[#lines+1] = "  全选: " .. (dlg:get_bool("wp_select_all") and "是" or "否")
    end
    lines[#lines+1] = "复制次数: " .. dlg:get_string("copy_times")
    dlg:set_text("summary", table.concat(lines, "\n"))
end

-- 初始状态
dlg:set_enabled("src_country", false)
dlg:set_enabled("dst_country", false)
dlg:set_enabled("team_ref_mode", true)
dlg:set_enabled("suffix_input", false)
dlg:set_enabled("digit_input", false)
dlg:set_visible("use_team", false)
dlg:set_visible("use_var", false)
dlg:set_enabled("wp_select_all", false)
dlg:set_enabled("wp_list", false)
update_summary(dlg)

-- 显示对话框
local result = dlg:do_modal()
if result == nil then
    print("用户取消了配置，退出")
    return
end

-- ==================== 解析配置 ====================
local config = {}

function extract_country_id(text)
    if not text or text == "" then return nil end
    return string.match(text, "^([^ ]+)")
end

local replace_enabled = result.replace_country
config.replace_country = replace_enabled

if replace_enabled then
    local src_id = extract_country_id(result.src_country)
    local dst_id = extract_country_id(result.dst_country)
    if not src_id or not dst_id then
        message_box("源国家和目标国家必须选择或输入有效ID", "错误", 0)
        return
    end
    local country_ids = {}
    for _, c in ipairs(country_list) do table.insert(country_ids, c.id) end
    local function is_valid(id)
        for _, cid in ipairs(country_ids) do if cid == id then return true end end
        return false
    end
    if not is_valid(src_id) or not is_valid(dst_id) then
        message_box("输入的国家ID无效", "错误", 0)
        return
    end

    local src_cn = result.src_cn or ""
    local src_abbr = result.src_abbr or ""
    local dst_cn = result.dst_cn or ""
    local dst_abbr = result.dst_abbr or ""

    config.replacements = build_replacement_map_from_input(src_id, dst_id, src_cn, src_abbr, dst_cn, dst_abbr)
    config.new_trigger_country = dst_id
    config.change_trigger_country = true
    config.new_team_country = dst_id
    config.change_team_country = true
    config.use_auto = false
    config.fixed_suffix = ""
    config.digit_info = nil

    config.use_existing_team = result.use_team
    config.use_existing_var = result.use_var
    if config.use_existing_team then
        config.copy_teams = true
        config.mode = "both_new"
    else
        config.copy_teams = result.copy_teams
        config.mode = "both_new"
        if config.copy_teams then
            local mode_str = result.team_ref_mode
            if mode_str == "新增特遣，新增脚本" then config.mode = "both_new"
            elseif mode_str == "保留原特遣，新增脚本" then config.mode = "keep_tf_new_sc"
            elseif mode_str == "新增特遣，保留原脚本" then config.mode = "new_tf_keep_sc"
            elseif mode_str == "保留原特遣，保留原脚本" then config.mode = "both_keep"
            end
        end
    end
    if config.use_existing_var then
        config.copy_vars = true
    else
        config.copy_vars = (result.var_mode == "新增局部变量（复制一份）")
    end
else
    config.change_trigger_country = false
    config.change_team_country = false
    config.use_existing_team = false
    config.use_existing_var = false
    local mode = result.name_mode
    if mode == "添加后缀 -copy" then
        config.use_auto = true
        config.fixed_suffix = ""
        config.digit_info = nil
    elseif mode == "手动输入后缀" then
        config.use_auto = false
        config.fixed_suffix = result.suffix_input
        if config.fixed_suffix == "" then config.fixed_suffix = "_copy" end
        config.digit_info = nil
    else
        config.use_auto = false
        config.fixed_suffix = ""
        local start_str = result.digit_input
        local start = tonumber(start_str)
        if not start then start = 0 end
        local width = #start_str
        if width == 0 then width = 1 end
        config.digit_info = {start = start, width = width}
    end
    config.copy_teams = result.copy_teams
    config.mode = "both_new"
    if config.copy_teams then
        local mode_str = result.team_ref_mode
        if mode_str == "新增特遣，新增脚本" then config.mode = "both_new"
        elseif mode_str == "保留原特遣，新增脚本" then config.mode = "keep_tf_new_sc"
        elseif mode_str == "新增特遣，保留原脚本" then config.mode = "new_tf_keep_sc"
        elseif mode_str == "保留原特遣，保留原脚本" then config.mode = "both_keep"
        end
    end
    config.copy_vars = (result.var_mode == "新增局部变量（复制一份）")
end

-- 路径点递增
config.increase_waypoint = result.wp_enable
local keep_waypoints = {}
if config.increase_waypoint then
    local selected_items = result.wp_list or {}
    if result.wp_select_all then
        selected_items = {}
        for _, cand in ipairs(waypoint_candidates) do
            table.insert(selected_items, cand.display)
        end
    end
    for _, display in ipairs(selected_items) do
        for _, cand in ipairs(waypoint_candidates) do
            if cand.display == display then
                if not keep_waypoints[cand.trig_id] then keep_waypoints[cand.trig_id] = {} end
                keep_waypoints[cand.trig_id][cand.action_index] = true
                break
            end
        end
    end
    if next(keep_waypoints) == nil and #waypoint_candidates > 0 then
        message_box("未选择任何路径点行为，路径点递增将跳过", "提示", 0)
        config.increase_waypoint = false
    end
end

-- 复制次数
local times = tonumber(result.copy_times)
if not times or times < 1 then
    message_box("复制次数必须为大于0的数字", "错误", 0)
    return
end

-- ==================== 全局资源与执行 ====================
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

for i = 1, times do
    print(string.format("\n========== 第 %d 次复制 ==========", i))
    local waypoint_step = i
    local name_offset = i
    if not config.digit_info then name_offset = 0 end
    local new_ids = perform_copy(original_triggers, config, existing_reg_idx, global_allocator, waypoint_step, keep_waypoints, name_offset)
    print(string.format("第 %d 次复制完成，生成了 %d 个新触发", i, #new_ids))
end

print("========== 全部复制完成 ==========")
message_box(string.format("已成功完成 %d 次复制。", times), "完成", 0)