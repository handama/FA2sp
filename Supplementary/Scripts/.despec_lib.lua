-- ============================================================================
-- .despec_lib.lua - Reverse spec extraction library for FA2sp MCP
-- ----------------------------------------------------------------------------
-- Purpose: Analyze an existing map that has NO spec, and reconstruct a
--          standard mymap.spec.md (story -> screenplay -> implementation)
--          from the map code, so the map can be managed by the spec workflow
--          afterwards. All inference details (confidence, evidence,
--          discrepancies, orphans) go into a companion report mymap.despec.md;
--          the spec file itself stays clean.
--
-- Usage (via MCP 'despec' tool or manually):
--     local D = dofile([[...path.../.despec_lib.lua]])
--     print(D.dispatch('{"action":"inventory","map_path":"C:/maps/old.map"}'))
--
-- ENCODING NOTES (important):
--   * This source file must stay PURE ASCII. Strings that pass through the MCP
--     run_lua path are converted to the internal (ANSI) encoding; dofile does
--     NOT convert file content, so any Chinese literal here would not match
--     runtime strings.
--   * Map data read from the editor (get_* globals) is already in internal
--     ANSI encoding and passes through as-is. Generated summaries use ASCII
--     keywords only: the lib does mechanical extraction, full semantic
--     interpretation is the agent's job (authority: knowledge/trigger_and_
--     script/* docs). Unknown event/action ids are kept as E<n>/A<n>.
--   * dispatch() returns a JSON string; bytes >= 0x80 are kept raw and the
--     MCP layer converts them back to UTF-8 for the client.
--
-- DEPENDENCIES: loads .spec_lib.lua (single source of the JSON codec, the
--   spec file format, and the import action). The MCP despec tool handler
--   injects the global SPEC_LIB_PATH before dofile; when absent, the lib
--   falls back to GetScriptRoot() .. ".spec_lib.lua".
-- ============================================================================

-- ============================================================================
-- Load the spec library (codec + import)
-- ============================================================================
local SPEC = nil
do
    local path = _G.SPEC_LIB_PATH
    if not path or path == "" then
        local gsr = _G.GetScriptRoot
        if type(gsr) == "function" then
            local ok, root = pcall(gsr)
            if ok and type(root) == "string" then path = root .. ".spec_lib.lua" end
        end
    end
    if path and path ~= "" then
        local ok, lib = pcall(dofile, path)
        if ok and type(lib) == "table" and type(lib.dispatch) == "function" then
            SPEC = lib
        end
    end
end
local json = SPEC and SPEC.json

-- ============================================================================
-- File / path helpers (mirror .spec_lib.lua conventions)
-- ============================================================================

local function normalize_path(p) return (p:gsub("\\", "/")) end

-- mymap.map -> mymap.despec.md (same directory, same base name)
local function derive_despec_path(map_path)
    local p = normalize_path(map_path)
    local base = p:match("^(.*)%.[^%.%/]+$") or p
    return base .. ".despec.md"
end

local function read_file(path)
    local f = io.open(path, "rb")
    if not f then return nil end
    local content = f:read("*a")
    f:close()
    local conv = _G.to_ansi
    if type(conv) == "function" then
        local ok, r = pcall(conv, content)
        if ok then return r end
    end
    return content
end

local function write_file(path, content)
    local conv = _G.to_utf8
    if type(conv) == "function" then
        local ok, r = pcall(conv, content)
        if ok then content = r end
    end
    local tmp = path .. ".tmp"
    local f = io.open(tmp, "wb")
    if not f then error("cannot open temp file for write: " .. tmp) end
    f:write(content)
    f:close()
    local back = read_file(tmp)
    if back ~= content then
        os.remove(tmp)
        error("despec write verification failed")
    end
    os.remove(path)
    local ok2, err2 = os.rename(tmp, path)
    if not ok2 then error("despec rename failed: " .. tostring(err2)) end
end

-- ============================================================================
-- Reader: all editor access goes through here so tests can mock it.
-- Every call is guarded; a missing global degrades to empty data instead of
-- crashing the whole action.
-- ============================================================================

local Reader = {}

local function call_global(name, ...)
    local fn = _G[name]
    if type(fn) ~= "function" then return nil end
    local ok, res = pcall(fn, ...)
    if not ok then return nil end
    return res
end

Reader.get_keys = function(section)
    local v = call_global("get_keys", section)
    return type(v) == "table" and v or {}
end

Reader.get_param = function(section, key, index, delim)
    local v = call_global("get_param", section, key, index, delim)
    return type(v) == "string" and v or ""
end

Reader.get_string = function(section, key, def)
    local v = call_global("get_string", section, key)
    if type(v) ~= "string" or v == "" then return def or "" end
    return v
end

Reader.get_key_value_pairs = function(section)
    local v = call_global("get_key_value_pairs", section)
    return type(v) == "table" and v or {}
end

Reader.get_ordered_key_value_pairs = function(section)
    local v = call_global("get_ordered_key_value_pairs", section)
    return type(v) == "table" and v or {}
end

Reader.get_values = function(section)
    local v = call_global("get_values", section)
    return type(v) == "table" and v or {}
end

Reader.get_sections = function()
    local v = call_global("get_sections")
    return type(v) == "table" and v or {}
end

-- Waypoint letter <-> number conversion (runtime globals, with local fallback)
local function wp_to_num(str)
    if type(str) ~= "string" or str == "" then return nil end
    local g = _G.string_to_waypoint
    if type(g) == "function" then
        local ok, r = pcall(g, str)
        if ok and type(r) == "string" then
            local n = tonumber(r)
            if n then return n end
        end
    end
    -- local fallback: A=0 .. Z=25, AA=26, ...
    if not str:match("^[A-Za-z]+$") then return nil end
    local n = 0
    local L = #str
    for i = 1, L do
        local b = str:byte(i)
        local d = (b >= 97) and (b - 97) or (b - 65)
        if i == 1 and L > 1 then d = d + 1 end
        n = n * 26 + d
    end
    return n
end

-- ============================================================================
-- String helpers
-- ============================================================================

local function split(str, delim)
    local out = {}
    if str == nil or str == "" then return out end
    local d = delim or ","
    local start = 1
    while true do
        local pos = str:find(d, start, true)
        if not pos then
            out[#out + 1] = str:sub(start)
            break
        end
        out[#out + 1] = str:sub(start, pos - 1)
        start = pos + 1
    end
    return out
end

local function param_at(fields, index)
    return fields[index] or ""
end

-- ============================================================================
-- Minimal event / action decode tables (ASCII keywords).
-- Authority for full semantics: knowledge/trigger_and_script/* docs.
-- Unknown ids keep the numeric form (E<n> / A<n>).
-- ============================================================================

local EVENT_TABLE = {
    [0]="None", [1]="Entered", [4]="Discovered", [5]="DiscoveredBy",
    [6]="Attacked", [7]="Destroyed", [8]="Any", [9]="AllUnitsDestroyed",
    [10]="AllBuildingsDestroyed", [11]="AllObjectsDestroyed", [12]="MoneyMore",
    [13]="ElapsedTime", [14]="TimerExpired", [15]="BuildingsLost", [16]="UnitsLost",
    [17]="NoFactories", [19]="BuiltBuildingType", [20]="ProducedVehicleType",
    [21]="TrainedInfantryType", [22]="ProducedAircraftType", [23]="TeamLeftMap",
    [24]="EnteredArea", [25]="CrossedHorizLine", [26]="CrossedVertLine",
    [27]="GlobalVarSet", [28]="GlobalVarClear", [29]="DestroyedAny",
    [30]="LowPower", [31]="BridgeDestroyed", [32]="BuildingExists", [33]="Selected",
    [34]="ReachedWP", [35]="EnemyInSpotlight", [36]="LocalVarSet",
    [37]="LocalVarClear", [38]="FirstDamaged", [39]="HalfHP", [40]="RedHP",
    [41]="FirstDamagedAny", [42]="HalfHPAny", [43]="RedHPAny", [44]="AttackedBy",
    [45]="AmbientLE", [46]="AmbientGE", [47]="GameElapsedTime",
    [48]="DestroyedAny", [49]="PickedUpCrate", [50]="AnyCrate", [51]="RandomDelay",
    [52]="MoneyLess", [55]="NavyDestroyed", [56]="GroundDestroyed",
    [57]="BuildingGone", [58]="PowerOK", [59]="EnteredOrFlewOver",
    [60]="TechExists", [61]="TechNotExists", [62]="EmpHit", [63]="EmpHitBy",
    [64]="EmpReleased", [65]="EmpReleasedBy", [66]="EnemyInSpotlightRepeat",
    [75]="SWReleased", [76]="SWStopped", [77]="SWAtWP", [78]="TechReverseEngineered",
    [79]="ReverseEngineeredAny", [80]="ReverseEngineeredType", [81]="TechExistsForHouse",
    [82]="TechNotExistsForHouse", [83]="AttackedOrDestroyed", [84]="AttackedOrDestroyedBy",
    [85]="DestroyedBy", [86]="TechCountLE", [87]="KeepAliveDestroyed",
    [88]="KeepAliveBuildingsDestroyed",
}

-- event id -> { param = n, kind = "team"|"wp"|"gvar"|"lvar"|"var" } reference slot
local EVENT_REFS = {
    [23] = { 2, "team" },
    [34] = { 2, "wp" },
    [77] = { 2, "wp" },
    [27] = { 2, "gvar" }, [28] = { 2, "gvar" },
    [36] = { 2, "lvar" }, [37] = { 2, "lvar" },
}

local ACTION_TABLE = {
    [0]="None", [1]="Win", [2]="Lose", [3]="StartBuilding", [4]="CreateTeam",
    [5]="DisbandTeam", [6]="HuntAll", [7]="Reinforce", [8]="DropZoneFlash",
    [9]="SellAll", [10]="PlayMovie", [11]="ShowText", [12]="DestroyTrigger",
    [13]="AutoCreateStart", [14]="ChangeOwner", [15]="AllowWin", [16]="RevealAll",
    [17]="RevealWP", [18]="RevealCellArea", [19]="PlaySound", [20]="PlayMusic",
    [21]="PlaySpeech", [22]="ForceTrigger", [23]="TimerStart", [24]="TimerStop",
    [25]="TimerAdd", [26]="TimerSub", [27]="TimerSet", [28]="SetGlobalVar",
    [29]="ClearGlobalVar", [30]="BaseBuilding", [31]="ShroudExpand",
    [32]="DestroyAssociated", [33]="AddSWOnce", [34]="AddSWRepeat",
    [36]="ChangeAllOwner", [37]="Alliance", [38]="DeclareWar", [40]="ResizeView",
    [41]="AnimAtWP", [42]="ExplosionAtWP", [43]="VoxelAnimAtWP",
    [46]="DisableInput", [47]="EnableInput", [48]="MoveCameraToWP",
    [49]="ZoomIn", [50]="ZoomOut", [51]="ResetShroud", [53]="EnableTrigger",
    [54]="DisableTrigger", [55]="RadarEvent", [56]="SetLocalVar",
    [57]="ClearLocalVar", [58]="MeteorAtWP", [59]="DeleteOreAtWP",
    [60]="SellAssociated", [61]="CloseAssociated", [62]="OpenAssociated",
    [63]="DamageAtWP", [67]="AnnounceWin", [68]="AnnounceLose", [69]="EndMission",
    [70]="DestroyTag", [71]="LightingChangeRate", [72]="LightingChangeSpeed",
    [73]="SetLighting", [74]="AITriggerStart", [75]="AITriggerStop",
    [76]="AITeamChance", [80]="ReinforceAtWP", [81]="WakeUp",
    [82]="WakeAllSleeping", [83]="WakeAllHarmless", [84]="WakeGroup",
    [85]="OreGrowth", [86]="OreSpread", [88]="ParticleAtWP",
    [89]="RemoveParticleAtWP", [90]="LightningAtWP", [94]="IonCannonAtWP",
    [95]="NukeAtWP", [107]="ChronoReinforce",
}

-- action id -> param index of a trigger reference
local TRIGGER_REF_ACTIONS = { [12] = 3, [22] = 3, [53] = 3, [54] = 3 }
-- action id -> param index of a team reference
local TEAM_REF_ACTIONS = { [4] = 3, [5] = 3, [7] = 3, [80] = 3 }
local TAG_REF_ACTIONS = { [70] = 3 }
local VAR_REF_ACTIONS = { [28] = 3, [29] = 3, [56] = 3, [57] = 3 }
-- waypoint references: letter format in param 8, or numeric index in param 3
local WP_ACTIONS_P8 = { [8] = 8, [41] = 8, [42] = 8, [43] = 8, [48] = 8, [55] = 8,
                        [80] = 8, [88] = 8, [89] = 8, [90] = 8, [94] = 8, [95] = 8 }
local WP_ACTIONS_P3 = { [17] = 3, [18] = 3, [59] = 3, [63] = 3 }

local function add_ref(refs, kind, value)
    if value == nil or value == "" then return end
    refs[#refs + 1] = { kind = kind, value = value }
end

local function decode_event(fields)
    local id = tonumber(fields[1] or "") or 0
    local kw = EVENT_TABLE[id]
    if not kw then
        if id >= 500 and id <= 535 then kw = "VarCmp" else kw = "E" .. tostring(id) end
    end
    local refs = {}
    local slot = EVENT_REFS[id]
    if slot then add_ref(refs, slot[2], fields[slot[1]]) end
    if id >= 500 and id <= 535 then add_ref(refs, "var", fields[2]) end
    return { id = id, kw = kw, refs = refs, fields = fields }
end

local function decode_action(fields)
    local id = tonumber(fields[1] or "") or 0
    local kw = ACTION_TABLE[id] or ("A" .. tostring(id))
    local refs = {}
    local hard = false
    local term = false
    local p = TRIGGER_REF_ACTIONS[id]
    if p then
        local target = fields[p]
        if target and target ~= "" then
            add_ref(refs, "trigger", target)
            if id == 53 or id == 22 then hard = true end
            if id == 12 or id == 54 then term = true end
        end
    end
    p = TEAM_REF_ACTIONS[id]
    if p then add_ref(refs, "team", fields[p]) end
    p = TAG_REF_ACTIONS[id]
    if p then add_ref(refs, "tag", fields[p]) end
    p = VAR_REF_ACTIONS[id]
    if p then add_ref(refs, "var", fields[p]) end
    p = WP_ACTIONS_P8[id]
    if p then
        local w = fields[p]
        if w and w ~= "" and w ~= "A" then
            local num = wp_to_num(w)
            add_ref(refs, "wp", num and tostring(num) or w)
        end
    end
    p = WP_ACTIONS_P3[id]
    if p then
        local w = fields[p]
        if w and w ~= "" and w ~= "0" then add_ref(refs, "wp", w) end
    end
    return { id = id, kw = kw, refs = refs, fields = fields, hard = hard, term = term }
end

-- ============================================================================
-- Map section readers
-- ============================================================================

-- [Triggers] ID = House,Linked,Name,Disabled,Easy,Normal,Hard,Persistence,...
local function read_triggers()
    local out = {}
    local pairs = Reader.get_ordered_key_value_pairs("Triggers")
    for _, p in ipairs(pairs) do
        local id = tostring(p[1])
        local f = split(p[2])
        out[#out + 1] = {
            id = id,
            house = param_at(f, 1),
            linked = param_at(f, 2),
            name = param_at(f, 3),
            disabled = (param_at(f, 4) == "1"),
            easy = param_at(f, 5),
            normal = param_at(f, 6),
            hard = param_at(f, 7),
            persistence = param_at(f, 8),
        }
    end
    return out
end

local function map_trigger_ids(triggers)
    local out = {}
    for _, t in ipairs(triggers) do out[#out + 1] = t.id end
    return out
end

-- [Events] ID = count, EID,P2[,P3[,P4]], ...
local function read_events_map(trigger_ids)
    local out = {}
    for _, id in ipairs(trigger_ids) do
        local val = Reader.get_string("Events", id)
        local list = {}
        if val and val ~= "" then
            local f = split(val)
            local count = tonumber(f[1]) or 0
            local pos = 2
            for _ = 1, count do
                local eid = tonumber(f[pos])
                if eid == 60 or eid == 61 or eid == 78 or eid == 80
                    or eid == 81 or eid == 82 or eid == 86 or eid == 77
                    or (eid and eid >= 500 and eid <= 535) then
                    -- 4-value events (tech type / comparison events)
                    list[#list + 1] = decode_event({ f[pos], f[pos + 1], f[pos + 2], f[pos + 3] })
                    pos = pos + 4
                elseif eid then
                    list[#list + 1] = decode_event({ f[pos], f[pos + 1], f[pos + 2] })
                    pos = pos + 3
                else
                    break
                end
                if pos > #f then break end
            end
        end
        out[id] = list
    end
    return out
end

-- [Actions] ID = count, AID,P2,P3,P4,P5,P6,P7,P8, ...
local function read_actions_map(trigger_ids)
    local out = {}
    for _, id in ipairs(trigger_ids) do
        local val = Reader.get_string("Actions", id)
        local list = {}
        if val and val ~= "" then
            local f = split(val)
            local count = tonumber(f[1]) or 0
            local pos = 2
            for _ = 1, count do
                local aid = tonumber(f[pos])
                if aid then
                    list[#list + 1] = decode_action({ f[pos], f[pos + 1], f[pos + 2],
                        f[pos + 3], f[pos + 4], f[pos + 5], f[pos + 6], f[pos + 7] })
                    pos = pos + 8
                else
                    break
                end
                if pos > #f then break end
            end
        end
        out[id] = list
    end
    return out
end

-- [Tags] ID = Repeat,Name,Trigger
local function read_tags()
    local out = {}
    local pairs = Reader.get_ordered_key_value_pairs("Tags")
    for _, p in ipairs(pairs) do
        local id = tostring(p[1])
        local f = split(p[2])
        out[#out + 1] = {
            id = id,
            repeat_type = param_at(f, 1),
            name = param_at(f, 2),
            trigger = param_at(f, 3),
        }
    end
    return out
end

-- Registry sections whose values are object ids ([TeamTypes]/[TaskForces]/[ScriptTypes])
local function read_registry(section)
    local out = {}
    local pairs = Reader.get_ordered_key_value_pairs(section)
    for _, p in ipairs(pairs) do
        local id = tostring(p[2])
        if id ~= "" then out[#out + 1] = id end
    end
    return out
end

-- Named-key object sections ([<teamid>], [<tfid>], [<scriptid>])
local function read_named_section(id, keys)
    local out = {}
    for _, k in ipairs(keys) do out[k] = Reader.get_string(id, k) end
    return out
end

local function read_teams()
    local out = {}
    local ids = read_registry("TeamTypes")
    for _, id in ipairs(ids) do
        local s = read_named_section(id, { "Name", "House", "TaskForce", "Script", "Tag", "Waypoint" })
        s.id = id
        out[#out + 1] = s
    end
    return out
end

local function read_taskforces()
    local out = {}
    local ids = read_registry("TaskForces")
    for _, id in ipairs(ids) do
        local s = read_named_section(id, { "Name", "Group" })
        s.id = id
        out[#out + 1] = s
    end
    return out
end

local function read_scripts()
    local out = {}
    local ids = read_registry("ScriptTypes")
    for _, id in ipairs(ids) do
        local s = read_named_section(id, { "Name", "House" })
        s.id = id
        out[#out + 1] = s
    end
    return out
end

local function count_pairs(section)
    local n = 0
    for _ in pairs(Reader.get_key_value_pairs(section)) do n = n + 1 end
    return n
end

-- ============================================================================
-- Dependency graph
-- ============================================================================

local function build_graph(triggers, actions_map)
    local nodes = {}
    local edges = {}
    local refs = {}
    local id_index = {}
    for _, t in ipairs(triggers) do
        id_index[t.id] = true
        nodes[#nodes + 1] = { id = t.id, name = t.name }
        refs[t.id] = {}
    end
    for _, t in ipairs(triggers) do
        local acts = actions_map[t.id] or {}
        for _, a in ipairs(acts) do
            for _, r in ipairs(a.refs) do
                if r.kind == "trigger" then
                    local kind = a.hard and "hard" or (a.term and "terminate" or "other")
                    if id_index[r.value] then
                        edges[#edges + 1] = { from = t.id, to = r.value, kind = kind, via = a.kw }
                    else
                        refs[t.id][#refs[t.id] + 1] = { kind = "trigger", value = r.value, note = "not in [Triggers]" }
                    end
                else
                    refs[t.id][#refs[t.id] + 1] = r
                end
            end
        end
    end
    -- cycle detection over hard edges only
    local adj = {}
    for _, e in ipairs(edges) do
        if e.kind == "hard" then
            adj[e.from] = adj[e.from] or {}
            adj[e.from][#adj[e.from] + 1] = e.to
        end
    end
    local color = {}
    local stack = {}
    local cycles = {}
    local function visit(id)
        color[id] = 1
        stack[#stack + 1] = id
        for _, nxt in ipairs(adj[id] or {}) do
            if color[nxt] == 1 then
                local cyc = {}
                local on = false
                for _, s in ipairs(stack) do
                    if s == nxt then on = true end
                    if on then cyc[#cyc + 1] = s end
                end
                cycles[#cycles + 1] = cyc
            elseif not color[nxt] then
                visit(nxt)
            end
        end
        stack[#stack] = nil
        color[id] = 2
    end
    for _, t in ipairs(triggers) do
        if not color[t.id] then visit(t.id) end
    end
    return { nodes = nodes, edges = edges, refs = refs, cycles = cycles }
end

-- ============================================================================
-- Clustering (draft pipelines)
-- ============================================================================

local function union_find(n)
    local parent = {}
    for i = 1, n do parent[i] = i end
    local function find(x)
        while parent[x] ~= x do
            parent[x] = parent[parent[x]]
            x = parent[x]
        end
        return x
    end
    local function union(a, b)
        local ra, rb = find(a), find(b)
        if ra ~= rb then parent[ra] = rb end
    end
    return { find = find, union = union }
end

local function cluster_triggers(triggers, graph)
    local n = #triggers
    local uf = union_find(n)
    local idx = {}
    for i, t in ipairs(triggers) do idx[t.id] = i end
    -- hard edges always join
    for _, e in ipairs(graph.edges) do
        if e.kind == "hard" and idx[e.from] and idx[e.to] then
            uf.union(idx[e.from], idx[e.to])
        end
    end
    -- weak evidence joins: shared typed references (team/tag/var/wp), and
    -- the linked_trigger chain
    local by_ref = {}
    for _, t in ipairs(triggers) do
        for _, r in ipairs(graph.refs[t.id] or {}) do
            if r.kind ~= "trigger" then
                local key = r.kind .. ":" .. tostring(r.value)
                by_ref[key] = by_ref[key] or {}
                by_ref[key][#by_ref[key] + 1] = t.id
            end
        end
        if t.linked and t.linked ~= "" and idx[t.linked] then
            uf.union(idx[t.id], idx[t.linked])
        end
    end
    for _, list in pairs(by_ref) do
        if #list >= 2 then
            for i = 2, #list do
                if idx[list[1]] and idx[list[i]] then
                    uf.union(idx[list[1]], idx[list[i]])
                end
            end
        end
    end
    -- collect components
    local comps = {}
    local order = {}
    for i = 1, n do
        local r = uf.find(i)
        if not comps[r] then
            comps[r] = {}
            order[#order + 1] = r
        end
        comps[r][#comps[r] + 1] = i
    end
    local lines = {}
    for _, r in ipairs(order) do
        lines[#lines + 1] = comps[r]
    end
    return lines
end

-- ============================================================================
-- Summary / confidence
-- ============================================================================

local function refs_str(refs)
    if #refs == 0 then return "" end
    local parts = {}
    for _, r in ipairs(refs) do
        parts[#parts + 1] = r.kind .. ":" .. tostring(r.value)
    end
    return table.concat(parts, ",")
end

local function summary_of(trigger, events, actions)
    local parts = {}
    if trigger.name and trigger.name ~= "" then
        parts[#parts + 1] = trigger.name
    end
    local ev = {}
    for _, e in ipairs(events) do
        local r = refs_str(e.refs)
        ev[#ev + 1] = e.kw .. (r ~= "" and ("(" .. r .. ")") or "")
    end
    local ac = {}
    for _, a in ipairs(actions) do
        local r = refs_str(a.refs)
        ac[#ac + 1] = a.kw .. (r ~= "" and ("(" .. r .. ")") or "")
    end
    if #ev > 0 then parts[#parts + 1] = "Ev: " .. table.concat(ev, "; ") end
    if #ac > 0 then parts[#parts + 1] = "Ac: " .. table.concat(ac, "; ") end
    local s = table.concat(parts, " | ")
    if #s > 300 then s = s:sub(1, 300) .. "..." end
    return s
end

-- count events/actions with a real (non-zero) id; id 0 entries are placeholders
local function meaningful_count(list)
    local n = 0
    for _, x in ipairs(list) do
        if x.id and x.id > 0 then n = n + 1 end
    end
    return n
end

local function confidence_of(trigger, events, actions, graph)
    local hard = 0
    for _, e in ipairs(graph.edges) do
        if e.kind == "hard" and (e.from == trigger.id or e.to == trigger.id) then
            hard = hard + 1
        end
    end
    local known_actions = 0
    for _, a in ipairs(actions) do
        if a.id > 0 and ACTION_TABLE[a.id] then known_actions = known_actions + 1 end
    end
    local known_events = 0
    for _, e in ipairs(events) do
        if e.id > 0 and EVENT_TABLE[e.id] then known_events = known_events + 1 end
    end
    if hard > 0 or known_actions > 0 or known_events > 0 then return "high" end
    if trigger.name ~= "" and (meaningful_count(events) > 0 or meaningful_count(actions) > 0) then
        return "medium"
    end
    return "low"
end

-- ============================================================================
-- Story scaffold (mechanical evidence; the agent turns it into prose)
-- ============================================================================

local function build_story_scaffold(triggers, actions_map)
    local title = Reader.get_string("Basic", "Name")
    local author = Reader.get_string("Basic", "Author")
    local briefing = Reader.get_key_value_pairs("Briefing")
    local briefing_snippet = ""
    for k, v in pairs(briefing) do
        if type(v) == "string" and v ~= "" then
            briefing_snippet = v
            break
        end
    end
    if #briefing_snippet > 200 then briefing_snippet = briefing_snippet:sub(1, 200) .. "..." end
    local movies = {}
    local texts = {}
    local endings = {}
    for _, t in ipairs(triggers) do
        for _, a in ipairs(actions_map[t.id] or {}) do
            if a.id == 10 then movies[#movies + 1] = t.name end
            if a.id == 11 then texts[#texts + 1] = t.name end
            if a.id == 1 or a.id == 2 or a.id == 67 or a.id == 68 or a.id == 69 then
                endings[#endings + 1] = t.name .. ":" .. a.kw
            end
        end
    end
    local story = "Draft story (machine-extracted, needs human rewrite): "
    if title ~= "" then story = story .. "map '" .. title .. "'" end
    if author ~= "" then story = story .. " by " .. author end
    story = story .. ". "
    if briefing_snippet ~= "" then
        story = story .. "Briefing text found: \"" .. briefing_snippet .. "\". "
    end
    if #movies > 0 then story = story .. "Cinematic triggers: " .. table.concat(movies, ", ") .. ". " end
    if #endings > 0 then story = story .. "Ending markers: " .. table.concat(endings, "; ") .. ". " end
    if #texts > 0 then story = story .. "Text triggers (CSF labels, content not readable from map): "
        .. table.concat(texts, ", ") .. ". " end
    return story, { title = title, author = author, has_briefing = briefing_snippet ~= "",
                    briefing_snippet = briefing_snippet, movies = movies, texts = texts,
                    endings = endings }
end

-- ============================================================================
-- draft data builder
-- ============================================================================

local function fmt_line_id(n) return string.format("L%02d", n) end
local function fmt_entry_id(line_id, seq) return line_id .. string.format("S%02d", seq) end

local function build_draft(triggers, events_map, actions_map, graph, map_path)
    local groups = cluster_triggers(triggers, graph)
    local lines = {}
    local entries = {}
    local next_seq = {}
    local confidence = {}
    local unclassified = {}
    local questions = {}
    local trig_to_entry = {}
    for gi, group in ipairs(groups) do
        -- classify: does the group have any evidence at all?
        local has_evidence = #group > 1
        if not has_evidence then
            local t = triggers[group[1]]
            local refs = graph.refs[t.id] or {}
            local ev = events_map[t.id] or {}
            local ac = actions_map[t.id] or {}
            -- a linked_trigger counts as evidence only when it points at a
            -- trigger that actually exists in the map
            local linked_ok = false
            if t.linked ~= "" then
                for _, o in ipairs(triggers) do
                    if o.id == t.linked then linked_ok = true break end
                end
            end
            has_evidence = #refs > 0 or meaningful_count(ev) > 0
                or meaningful_count(ac) > 0 or linked_ok
        end
        local line_id = fmt_line_id(#lines + 1)
        if not has_evidence then
            unclassified[#unclassified + 1] = triggers[group[1]].id
        end
        local houses = {}
        for _, i in ipairs(group) do
            local h = triggers[i].house
            if h ~= "" then houses[h] = (houses[h] or 0) + 1 end
        end
        local house_str = ""
        for h, c in pairs(houses) do
            if house_str ~= "" then house_str = house_str .. "," end
            house_str = house_str .. h .. "x" .. tostring(c)
        end
        local name = "Pipeline-" .. tostring(#lines + 1)
            .. (house_str ~= "" and (" (house " .. house_str .. ")") or "")
        if not has_evidence then name = "Unclassified" end
        lines[#lines + 1] = { id = line_id, name = name }
        next_seq[line_id] = 1
        -- first pass: assign entry ids so cross-entry dependencies can be built
        local group_entries = {}
        for _, i in ipairs(group) do
            local t = triggers[i]
            local entry_id = fmt_entry_id(line_id, next_seq[line_id])
            next_seq[line_id] = next_seq[line_id] + 1
            trig_to_entry[t.id] = entry_id
            group_entries[#group_entries + 1] = { t = t, entry_id = entry_id }
        end
        for _, ge in ipairs(group_entries) do
            local t = ge.t
            local ev = events_map[t.id] or {}
            local ac = actions_map[t.id] or {}
            local deps = {}
            for _, e in ipairs(graph.edges) do
                if e.kind == "hard" and e.from == t.id then
                    local dep_entry = trig_to_entry[e.to]
                    if dep_entry then deps[#deps + 1] = dep_entry end
                end
            end
            entries[#entries + 1] = {
                id = ge.entry_id,
                line = line_id,
                summary = summary_of(t, ev, ac),
                depends_on = deps,
                status = "implemented",
                triggers = { { type = t.id, name = t.name } },
            }
            confidence[ge.entry_id] = confidence_of(t, ev, ac, graph)
        end
    end
    if #unclassified > 0 then
        questions[#questions + 1] = "unclassified triggers (no events/actions/refs found): "
            .. table.concat(unclassified, ", ")
            .. " - decide whether each belongs to a pipeline or stays out of the spec"
    end
    local story, evidence = build_story_scaffold(triggers, actions_map)
    local base = map_path:match("([^/]+)%.[^%.%/]+$") or map_path
    local data = {
        version = 1,
        map_path = map_path,
        title = base,
        updated = os.date("%Y-%m-%d %H:%M:%S"),
        story = story,
        lines = lines,
        entries = entries,
        next_seq = next_seq,
    }
    return data, confidence, unclassified, questions, evidence
end

-- ============================================================================
-- Report generation
-- ============================================================================

local function bool_yn(v) return v and "yes" or "no" end

local function render_report(map_path, triggers, events_map, actions_map, graph, tags, teams, user_flow, discrepancies)
    local out = {}
    local base = map_path:match("([^/]+)%.[^%.%/]+$") or map_path
    out[#out + 1] = "# " .. base .. " despec report"
    out[#out + 1] = "> map: " .. map_path .. "  |  generated: " .. os.date("%Y-%m-%d %H:%M:%S")
    out[#out + 1] = "> This report records the reverse-engineering evidence behind the spec. "
        .. "The spec itself is the design record; this file is reference only."
    out[#out + 1] = ""
    out[#out + 1] = "## User narrative (provided by user, unverified)"
    if user_flow and user_flow ~= "" then
        out[#out + 1] = user_flow
    else
        out[#out + 1] = "(none provided)"
    end
    out[#out + 1] = ""
    out[#out + 1] = "## Trigger evidence table"
    out[#out + 1] = "| ID | Name | House | Disabled | Linked | Events | Actions | Inferred | Confidence |"
    out[#out + 1] = "|---|---|---|---|---|---|---|---|---|"
    for _, t in ipairs(triggers) do
        local ev = events_map[t.id] or {}
        local ac = actions_map[t.id] or {}
        local evs = {}
        for _, e in ipairs(ev) do
            local r = refs_str(e.refs)
            evs[#evs + 1] = e.kw .. (r ~= "" and ("(" .. r .. ")") or "")
        end
        local acs = {}
        for _, a in ipairs(ac) do
            local r = refs_str(a.refs)
            acs[#acs + 1] = a.kw .. (r ~= "" and ("(" .. r .. ")") or "")
        end
        local summary = summary_of(t, ev, ac)
        out[#out + 1] = "| " .. t.id .. " | " .. t.name .. " | " .. t.house .. " | "
            .. bool_yn(t.disabled) .. " | " .. (t.linked ~= "" and t.linked or "-") .. " | "
            .. table.concat(evs, "<br>") .. " | " .. table.concat(acs, "<br>") .. " | "
            .. summary .. " | " .. confidence_of(t, ev, ac, graph) .. " |"
    end
    out[#out + 1] = ""
    out[#out + 1] = "## Dependency graph"
    if #graph.edges == 0 then
        out[#out + 1] = "No trigger-to-trigger references found."
    else
        for _, e in ipairs(graph.edges) do
            out[#out + 1] = "- [" .. e.kind .. "] " .. e.from .. " -> " .. e.to
                .. " (via " .. e.via .. ")"
        end
    end
    if #graph.cycles > 0 then
        out[#out + 1] = ""
        out[#out + 1] = "### Cycles (hard edges)"
        for _, c in ipairs(graph.cycles) do
            out[#out + 1] = "- " .. table.concat(c, " -> ")
        end
    end
    out[#out + 1] = ""
    out[#out + 1] = "## Tags and teams"
    out[#out + 1] = "Tags: " .. tostring(#tags)
    local tnames = {}
    for _, tm in ipairs(teams) do
        tnames[#tnames + 1] = tm.id .. (tm.Name ~= "" and ("(" .. tm.Name .. ")") or "")
    end
    out[#out + 1] = "Teams: " .. table.concat(tnames, ", ")
    out[#out + 1] = ""
    out[#out + 1] = "## Discrepancies (user narrative vs code evidence)"
    if discrepancies and #discrepancies > 0 then
        out[#out + 1] = "| Ref | User claim | Code evidence | Category | Status |"
        out[#out + 1] = "|---|---|---|---|---|"
        for _, d in ipairs(discrepancies) do
            out[#out + 1] = "| " .. tostring(d.ref or "-") .. " | " .. tostring(d.user_claim or "")
                .. " | " .. tostring(d.code_evidence or "") .. " | " .. tostring(d.category or "")
                .. " | " .. tostring(d.status or "open") .. " |"
        end
    else
        out[#out + 1] = "(none recorded)"
    end
    out[#out + 1] = ""
    out[#out + 1] = "## Orphans / not included"
    out[#out + 1] = "Trigger ids present in the map but not referenced by any spec entry will be listed here after import (see despec(report) after spec(import))."
    out[#out + 1] = ""
    out[#out + 1] = "## Stats"
    out[#out + 1] = "- triggers: " .. tostring(#triggers)
    out[#out + 1] = "- events decoded: " .. tostring(#events_map)
    out[#out + 1] = "- actions decoded: " .. tostring(#actions_map)
    out[#out + 1] = "- graph edges: " .. tostring(#graph.edges)
    out[#out + 1] = "- cycles: " .. tostring(#graph.cycles)
    return table.concat(out, "\r\n") .. "\r\n"
end

-- ============================================================================
-- Actions
-- ============================================================================

local function act_inventory(map_path)
    local triggers = read_triggers()
    local tags = read_tags()
    local teams = read_teams()
    local taskforces = read_taskforces()
    local scripts = read_scripts()
    local events_map = read_events_map(map_trigger_ids(triggers))
    local actions_map = read_actions_map(map_trigger_ids(triggers))
    -- trigger -> tags
    local tag_by_trigger = {}
    for _, tg in ipairs(tags) do
        if tg.trigger ~= "" then
            tag_by_trigger[tg.trigger] = tag_by_trigger[tg.trigger] or {}
            tag_by_trigger[tg.trigger][#tag_by_trigger[tg.trigger] + 1] = tg.id
        end
    end
    local tlist = {}
    for _, t in ipairs(triggers) do
        tlist[#tlist + 1] = {
            id = t.id, name = t.name, house = t.house, disabled = t.disabled,
            linked = t.linked,
            events = #(events_map[t.id] or {}),
            actions = #(actions_map[t.id] or {}),
            tags = tag_by_trigger[t.id] or {},
        }
    end
    return {
        ok = true, map_path = map_path,
        counts = {
            triggers = #triggers, teams = #teams, taskforces = #taskforces,
            scripts = #scripts, tags = #tags,
            waypoints = count_pairs("Waypoints"), celltags = count_pairs("CellTags"),
            houses = count_pairs("Houses"), variables = count_pairs("VariableNames"),
        },
        triggers = tlist,
        notes = {},
    }
end

local function act_graph(map_path)
    local triggers = read_triggers()
    local actions_map = read_actions_map(map_trigger_ids(triggers))
    local graph = build_graph(triggers, actions_map)
    return { ok = true, map_path = map_path, nodes = graph.nodes,
             edges = graph.edges, refs = graph.refs, cycles = graph.cycles }
end

local function act_draft(map_path)
    local triggers = read_triggers()
    local events_map = read_events_map(map_trigger_ids(triggers))
    local actions_map = read_actions_map(map_trigger_ids(triggers))
    local graph = build_graph(triggers, actions_map)
    local data, confidence, unclassified, questions, evidence =
        build_draft(triggers, events_map, actions_map, graph, map_path)
    return { ok = true, map_path = map_path, data = data, confidence = confidence,
             unclassified = unclassified, questions = questions, story_evidence = evidence }
end

local function act_analyze(map_path, args)
    local trigger_id = args.trigger_id
    if not trigger_id or trigger_id == "" then
        return { ok = false, error = "missing 'trigger_id'" }
    end
    local triggers = read_triggers()
    local t = nil
    for _, tr in ipairs(triggers) do
        if tr.id == trigger_id then t = tr break end
    end
    if not t then
        return { ok = false, error = "trigger not found in map [Triggers]: " .. tostring(trigger_id) }
    end
    local events = read_events_map({ trigger_id })[trigger_id] or {}
    local actions = read_actions_map({ trigger_id })[trigger_id] or {}
    return { ok = true, map_path = map_path, trigger = t,
             events = events, actions = actions }
end

local function act_import(map_path, args)
    if not SPEC then
        return { ok = false, error = "spec library (.spec_lib.lua) unavailable; cannot import" }
    end
    local payload = { action = "import", map_path = map_path, data = args.data }
    local ok, res = pcall(function()
        return json.decode(SPEC.dispatch(json.encode(payload)))
    end)
    if not ok then
        return { ok = false, error = "import failed: " .. tostring(res) }
    end
    if type(res) ~= "table" then
        return { ok = false, error = "import returned an invalid result" }
    end
    return res
end

local function act_report(map_path, args)
    local triggers = read_triggers()
    local events_map = read_events_map(map_trigger_ids(triggers))
    local actions_map = read_actions_map(map_trigger_ids(triggers))
    local graph = build_graph(triggers, actions_map)
    local tags = read_tags()
    local teams = read_teams()
    local user_flow = args.user_flow or ""
    local discrepancies = args.discrepancies or {}
    local report_path = derive_despec_path(map_path)
    local content = render_report(map_path, triggers, events_map, actions_map,
        graph, tags, teams, user_flow, discrepancies)
    local ok, err = pcall(write_file, report_path, content)
    if not ok then
        return { ok = false, error = "report write failed: " .. tostring(err) }
    end
    -- orphan check against an existing spec, if any
    local orphans = {}
    local spec_trigger_ids = {}
    if SPEC then
        local res = json.decode(SPEC.dispatch(json.encode({ action = "read", map_path = map_path })))
        if type(res) == "table" and res.ok and res.exists and type(res.data) == "table" then
            for _, en in ipairs(res.data.entries or {}) do
                for _, tg in ipairs(en.triggers or {}) do
                    spec_trigger_ids[tg.type] = true
                end
            end
        end
    end
    for _, t in ipairs(triggers) do
        if not spec_trigger_ids[t.id] then orphans[#orphans + 1] = t.id end
    end
    return { ok = true, report_path = report_path, orphans = orphans }
end

-- ============================================================================
-- dispatch(json_string) -> json_string
-- ============================================================================

local ACTIONS = {
    inventory = act_inventory,
    graph     = act_graph,
    draft     = act_draft,
    analyze   = act_analyze,
    import    = act_import,
    report    = act_report,
}

local function dispatch(json_str)
    if not json then
        return "{\"ok\":false,\"error\":\"spec library (.spec_lib.lua) unavailable "
            .. "(set SPEC_LIB_PATH or provide GetScriptRoot)\"}"
    end
    local ok, args = pcall(json.decode, json_str)
    if not ok or type(args) ~= "table" then
        return json.encode({ ok = false, error = "cannot parse arguments: " .. tostring(args) })
    end
    local action = args.action
    local handler = ACTIONS[action]
    if not handler then
        return json.encode({ ok = false, error = "unknown action: " .. tostring(action)
            .. " (expected: inventory, graph, draft, analyze, import, report)" })
    end
    local map_path = args.map_path
    if not map_path or map_path == "" then
        return json.encode({ ok = false, error = "missing 'map_path'" })
    end
    local ok2, res = pcall(handler, map_path, args)
    if not ok2 then
        return json.encode({ ok = false, error = "action '" .. action .. "' failed: "
            .. tostring(res) })
    end
    if type(res) ~= "table" then
        return json.encode({ ok = false, error = "action '" .. action
            .. "' returned an invalid result" })
    end
    return json.encode(res)
end

return { dispatch = dispatch }
