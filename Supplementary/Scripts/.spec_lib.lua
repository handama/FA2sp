-- ============================================================================
-- .spec_lib.lua - Map spec management library for FA2sp MCP
-- ----------------------------------------------------------------------------
-- Purpose: Persist the map design intent (story -> screenplay -> implementation)
--          as a structured .spec.md file next to the map, so the design process
--          can be resumed across sessions/agents.
--
-- Usage (via MCP 'spec' tool or manually):
--     local S = dofile([[...path.../.spec_lib.lua]])
--     print(S.dispatch('{"action":"read","map_path":"C:/maps/mymap.map"}'))
--
-- ENCODING NOTES (important):
--   * This source file must stay PURE ASCII. Strings that pass through the MCP
--     run_lua path are converted to the internal (ANSI) encoding; dofile does
--     NOT convert file content, so any Chinese literal here would not match
--     runtime strings.
--   * The .spec.md file is EXTERNAL storage: it is written as UTF-8 and, when
--     read back, converted to ANSI (to_ansi) so the Lua runtime stays uniform
--     GBK internally. write_file converts ANSI -> UTF-8 via to_utf8. to_ansi /
--     to_utf8 are globals provided by the FA2sp runtime (identity for
--     already-converted input).
--   * dispatch() returns a JSON string; bytes >= 0x80 are kept raw and the
--     MCP layer converts them back to UTF-8 for the client.
--   * Internal status keys are ASCII: designing / implemented / verified /
--     deprecated.
-- ============================================================================

local BEGIN_MARK  = "<!-- SPEC-DATA-BEGIN -->"
local END_MARK    = "<!-- SPEC-DATA-END -->"
local VERSION     = 1

local STATUSES = { designing = true, implemented = true, verified = true, deprecated = true }

-- ============================================================================
-- Minimal JSON support (pure Lua, ASCII source)
-- ============================================================================
local json = {}

local function json_escape_char(c)
    local n = c:byte()
    if n == 34 then return "\\\""          -- "
    elseif n == 92 then return "\\\\"      -- \
    elseif n == 8 then return "\\b"
    elseif n == 9 then return "\\t"
    elseif n == 10 then return "\\n"
    elseif n == 12 then return "\\f"
    elseif n == 13 then return "\\r"
    elseif n < 32 then return string.format("\\u%04x", n)
    else return c end                       -- bytes >= 0x80 pass through raw
end

function json.encode(v)
    local t = type(v)
    if t == "string" then
        return '"' .. v:gsub('[%z\1-\31\\"]', json_escape_char) .. '"'
    elseif t == "number" then
        if v ~= v or v == math.huge or v == -math.huge then return "null" end
        return string.format("%.14g", v)
    elseif t == "boolean" then
        return v and "true" or "false"
    elseif t == "table" then
        local is_arr = (#v > 0) or (next(v) == nil)
        -- heuristic: pure array if all keys are 1..n consecutive
        local n = #v
        local count = 0
        for _ in pairs(v) do count = count + 1 end
        if count == n and n > 0 then is_arr = true
        elseif count > 0 and n == 0 then is_arr = false end
        if is_arr then
            local parts = {}
            for i = 1, n do parts[i] = json.encode(v[i]) end
            return "[" .. table.concat(parts, ",") .. "]"
        else
            local parts = {}
            local keys = {}
            for k in pairs(v) do keys[#keys + 1] = k end
            table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)
            for i, k in ipairs(keys) do
                parts[i] = json.encode(k) .. ":" .. json.encode(v[k])
            end
            return "{" .. table.concat(parts, ",") .. "}"
        end
    end
    return "null"
end

-- Recursive-descent JSON decoder. \uXXXX escapes: codepoints < 0x80 become a
-- raw byte; others are emitted as UTF-8 bytes (best effort; the MCP client
-- normally sends raw text, so escapes are rare).
local function json_decode_value(str, pos)
    local c = str:sub(pos, pos)
    if c == "{" then
        local obj = {}
        pos = pos + 1
        c = str:sub(pos, pos)
        if c == "}" then return obj, pos + 1 end
        while true do
            while str:sub(pos, pos):match("%s") do pos = pos + 1 end
            local k, np = json_decode_value(str, pos)
            if type(k) ~= "string" then error("json: expected string key at " .. pos) end
            pos = np
            while str:sub(pos, pos):match("%s") do pos = pos + 1 end
            if str:sub(pos, pos) ~= ":" then error("json: expected ':' at " .. pos) end
            pos = pos + 1
            while str:sub(pos, pos):match("%s") do pos = pos + 1 end
            local v, np2 = json_decode_value(str, pos)
            obj[k] = v
            pos = np2
            while str:sub(pos, pos):match("%s") do pos = pos + 1 end
            c = str:sub(pos, pos)
            if c == "}" then return obj, pos + 1 end
            if c ~= "," then error("json: expected ',' or '}' at " .. pos) end
            pos = pos + 1
        end
    elseif c == "[" then
        local arr = {}
        pos = pos + 1
        c = str:sub(pos, pos)
        if c == "]" then return arr, pos + 1 end
        while true do
            while str:sub(pos, pos):match("%s") do pos = pos + 1 end
            local v, np = json_decode_value(str, pos)
            arr[#arr + 1] = v
            pos = np
            while str:sub(pos, pos):match("%s") do pos = pos + 1 end
            c = str:sub(pos, pos)
            if c == "]" then return arr, pos + 1 end
            if c ~= "," then error("json: expected ',' or ']' at " .. pos) end
            pos = pos + 1
        end
    elseif c == '"' then
        local out = {}
        pos = pos + 1
        while true do
            local ch = str:sub(pos, pos)
            if ch == "" then error("json: unterminated string") end
            if ch == '"' then return table.concat(out), pos + 1 end
            if ch == "\\" then
                local e = str:sub(pos + 1, pos + 1)
                if e == "n" then out[#out + 1] = "\n"
                elseif e == "t" then out[#out + 1] = "\t"
                elseif e == "r" then out[#out + 1] = "\r"
                elseif e == "b" then out[#out + 1] = "\b"
                elseif e == "f" then out[#out + 1] = "\f"
                elseif e == "/" then out[#out + 1] = "/"
                elseif e == "\\" then out[#out + 1] = "\\"
                elseif e == '"' then out[#out + 1] = '"'
                elseif e == "u" then
                    local hex = str:sub(pos + 2, pos + 5)
                    if not hex:match("^%x%x%x%x$") then error("json: bad \\u escape") end
                    local cp = tonumber(hex, 16)
                    if cp < 0x80 then
                        out[#out + 1] = string.char(cp)
                    else
                        -- best-effort UTF-8 encoding
                        if cp < 0x800 then
                            out[#out + 1] = string.char(0xC0 + math.floor(cp / 0x40),
                                                        0x80 + (cp % 0x40))
                        else
                            out[#out + 1] = string.char(0xE0 + math.floor(cp / 0x1000),
                                                        0x80 + (math.floor(cp / 0x40) % 0x40),
                                                        0x80 + (cp % 0x40))
                        end
                    end
                    pos = pos + 4
                else
                    error("json: bad escape \\" .. e)
                end
                pos = pos + 2
            else
                out[#out + 1] = ch
                pos = pos + 1
            end
        end
    elseif c == "t" and str:sub(pos, pos + 3) == "true" then return true, pos + 4
    elseif c == "f" and str:sub(pos, pos + 4) == "false" then return false, pos + 5
    elseif c == "n" and str:sub(pos, pos + 3) == "null" then return nil, pos + 4
    else
        local num = str:match("^-?%d+%.?%d*[eE]?[+-]?%d*", pos)
        if num and num ~= "" then
            local v = tonumber(num)
            if not v then error("json: bad number at " .. pos) end
            return v, pos + #num
        end
        error("json: unexpected char '" .. c .. "' at " .. pos)
    end
end

function json.decode(s)
    if type(s) ~= "string" then error("json.decode: expected string") end
    local pos = 1
    while s:sub(pos, pos):match("%s") do pos = pos + 1 end
    local v, np = json_decode_value(s, pos)
    return v
end

-- ============================================================================
-- File / path helpers
-- ============================================================================

local function normalize_path(p)
    return (p:gsub("\\", "/"))
end

-- mymap.map  ->  mymap.spec.md  (same directory, same base name)
local function derive_spec_path(map_path)
    local p = normalize_path(map_path)
    local base = p:match("^(.*)%.[^%.%/]+$") or p
    return base .. ".spec.md"
end

-- External storage is UTF-8; convert to ANSI for the GBK Lua runtime
-- (to_ansi is a global provided by the FA2sp runtime).
local function read_file(path)
    local f = io.open(path, "rb")
    if not f then return nil end
    local content = f:read("*a")
    f:close()
    return to_ansi(content)
end

-- Atomic-ish write: write tmp, verify, remove target, rename.
-- Convert ANSI -> UTF-8 before writing (to_utf8 is a global provided by the
-- FA2sp runtime); read_file converts it back, so verification compares ANSI.
local function write_file(path, content)
    local tmp = path .. ".tmp"
    local f = io.open(tmp, "wb")
    if not f then error("cannot open temp file for write: " .. tmp) end
    f:write(to_utf8(content))
    f:close()
    local back = read_file(tmp)
    if back ~= content then
        os.remove(tmp)
        error("spec write verification failed")
    end
    local ok, err = os.remove(path)
    if not ok and err then
        -- target may not exist yet; keep going only if it truly does not exist
    end
    local ok2, err2 = os.rename(tmp, path)
    if not ok2 then
        error("spec rename failed: " .. tostring(err2))
    end
end

-- ============================================================================
-- Spec load / save
-- ============================================================================

-- Returns data table, or (nil, reason) where reason in {"no_spec","corrupt"}
local function load_spec(spec_path)
    local content = read_file(spec_path)
    if not content then return nil, "no_spec" end
    local s = content:find(BEGIN_MARK, 1, true)
    local e = content:find(END_MARK, s and (s + #BEGIN_MARK) or 1, true)
    if not s or not e or e <= s then return nil, "corrupt" end
    local block = content:sub(s + #BEGIN_MARK, e - 1)
    local ok, data = pcall(json.decode, block)
    if not ok or type(data) ~= "table" then return nil, "corrupt" end
    return data
end

local function now_str()
    return os.date("%Y-%m-%d %H:%M:%S")
end

local function render_markdown(data)
    local out = {}
    out[#out + 1] = "# " .. data.title .. " Spec"
    out[#out + 1] = "> map: " .. data.map_path .. "  |  updated: " .. data.updated
    out[#out + 1] = ""
    out[#out + 1] = "## Story"
    out[#out + 1] = (data.story and data.story ~= "") and data.story or "(empty)"
    out[#out + 1] = ""
    out[#out + 1] = "## Screenplay"
    -- build line index -> order
    local line_order = {}
    for i, ln in ipairs(data.lines) do line_order[ln.id] = i end
    local function entry_sort_key(en)
        local lo = line_order[en.line] or 99
        local seq = tonumber(en.id:match("S(%d+)$")) or 0
        return lo * 100000 + seq
    end
    local sorted_entries = {}
    for _, en in ipairs(data.entries) do sorted_entries[#sorted_entries + 1] = en end
    table.sort(sorted_entries, function(a, b)
        return entry_sort_key(a) < entry_sort_key(b)
    end)
    for _, ln in ipairs(data.lines) do
        out[#out + 1] = "### " .. ln.id .. " " .. (ln.name or "")
        local any = false
        for _, en in ipairs(sorted_entries) do
            if en.line == ln.id then
                any = true
                local pre = (en.depends_on and #en.depends_on > 0)
                    and (" | pre: " .. table.concat(en.depends_on, ", ")) or ""
                local dep = (en.status == "deprecated") and " [deprecated]" or ""
                out[#out + 1] = "- " .. en.id .. " " .. (en.summary or "") .. dep .. pre
            end
        end
        if not any then out[#out + 1] = "- (no entries)" end
        out[#out + 1] = ""
    end
    out[#out + 1] = "## Implementation"
    out[#out + 1] = "| Entry | Status | Triggers |"
    out[#out + 1] = "|---|---|---|"
    for _, en in ipairs(sorted_entries) do
        local tcell = {}
        for _, tg in ipairs(en.triggers or {}) do
            local nm = (tg.name and tg.name ~= "") and (tg.name .. "(" .. tg.type .. ")") or tg.type
            tcell[#tcell + 1] = nm
        end
        out[#out + 1] = "| " .. en.id .. " | " .. (en.status or "designing") .. " | "
            .. ((#tcell > 0) and table.concat(tcell, "; ") or "-") .. " |"
    end
    out[#out + 1] = ""
    out[#out + 1] = BEGIN_MARK
    out[#out + 1] = json.encode(data)
    out[#out + 1] = END_MARK
    return table.concat(out, "\r\n") .. "\r\n"
end

local function save_spec(spec_path, data)
    data.updated = now_str()
    write_file(spec_path, render_markdown(data))
end

-- ============================================================================
-- DAG helpers (depends_on cycle detection)
-- ============================================================================

-- Returns list of entries that form a cycle (non-empty) or nil.
local function find_cycle(data)
    local index = {}   -- entry_id -> entry
    for _, en in ipairs(data.entries) do index[en.id] = en end
    local color = {}   -- 0 = unvisited, 1 = in stack, 2 = done
    local stack = {}
    local cycle = nil
    local function visit(id)
        color[id] = 1
        stack[#stack + 1] = id
        local en = index[id]
        for _, dep in ipairs(en.depends_on or {}) do
            if index[dep] then
                local c = color[dep]
                if c == 1 then
                    -- found cycle: report from dep to end of stack
                    cycle = {}
                    local on = false
                    for _, s in ipairs(stack) do
                        if s == dep then on = true end
                        if on then cycle[#cycle + 1] = s end
                    end
                    return true
                elseif not c then
                    if visit(dep) then return true end
                end
            end
        end
        stack[#stack] = nil
        color[id] = 2
        return false
    end
    for _, en in ipairs(data.entries) do
        if not color[en.id] then
            if visit(en.id) then break end
        end
    end
    return cycle
end

-- ============================================================================
-- Trigger existence validation (FA2sp globals; guarded)
-- ============================================================================

-- Returns true/false when map is available, nil (plus reason) when unavailable.
-- NOTE: get_triggers() returns a sol3 container userdata (not a plain table),
--       so do not rely on type(x) == "table"; iterate with ipairs / #.
local function trigger_exists(trigger_type)
    local ok, ids = pcall(get_triggers)
    if not ok then return nil, "map trigger validation failed: " .. tostring(ids) end
    for _, v in ipairs(ids) do
        if tostring(v) == trigger_type then return true end
    end
    return false
end

-- All trigger ids currently in the map, or nil.
local function map_trigger_ids()
    local ok, ids = pcall(get_triggers)
    if not ok then return nil end
    local out = {}
    for _, v in ipairs(ids) do out[#out + 1] = tostring(v) end
    return out
end

-- ============================================================================
-- Action implementations
-- ============================================================================

local function fmt_line_id(n) return string.format("L%02d", n) end
local function fmt_entry_id(line_id, seq) return line_id .. string.format("S%02d", seq) end

local function act_init(data, args)
    if data then
        return { ok = false, error = "spec already exists: " .. args.spec_path }
    end
    local fresh = {
        version  = VERSION,
        map_path = args.map_path,
        title    = (args.title and args.title ~= "") and args.title
                    or (args.map_path:match("([^/]+)%.[^%.%/]+$") or args.map_path),
        updated  = now_str(),
        story    = "",
        lines    = {},
        entries  = {},
        next_seq = {},
    }
    save_spec(args.spec_path, fresh)
    return { ok = true, spec_path = args.spec_path }
end

local function act_read(data, args)
    if not data then
        return { ok = true, exists = false, spec_path = args.spec_path,
                 reason = "no_spec (run init first or create a fresh map)" }
    end
    return { ok = true, exists = true, spec_path = args.spec_path, data = data }
end

local function act_update_story(data, args)
    if args.text == nil then return { ok = false, error = "missing 'text'" } end
    data.story = args.text
    save_spec(args.spec_path, data)
    return { ok = true }
end

local function act_add_line(data, args)
    if not args.name or args.name == "" then
        return { ok = false, error = "missing 'name'" }
    end
    local max_n = 0
    for _, ln in ipairs(data.lines) do
        local n = tonumber(ln.id:match("L(%d+)$")) or 0
        if n > max_n then max_n = n end
    end
    local id = fmt_line_id(max_n + 1)
    data.lines[#data.lines + 1] = { id = id, name = args.name }
    data.next_seq[id] = 1
    save_spec(args.spec_path, data)
    return { ok = true, line = id }
end

local function act_add_entry(data, args)
    local line = args.line
    if not line then return { ok = false, error = "missing 'line'" } end
    local line_exists = false
    for _, ln in ipairs(data.lines) do if ln.id == line then line_exists = true break end end
    if not line_exists then
        return { ok = false, error = "line not found: " .. tostring(line) .. " (use add_line first)" }
    end
    local seq = data.next_seq[line] or 1
    local id = fmt_entry_id(line, seq)
    local depends = {}
    if args.depends_on then
        for _, d in ipairs(args.depends_on) do depends[#depends + 1] = tostring(d) end
    end
    local entry = {
        id = id, line = line,
        summary = args.summary or "",
        depends_on = depends,
        status = "designing",
        triggers = {},
    }
    -- validate depends_on now (existence + self)
    local idset = {}
    for _, en in ipairs(data.entries) do idset[en.id] = true end
    for _, d in ipairs(depends) do
        if d == id then return { ok = false, error = "entry cannot depend on itself" } end
        if not idset[d] then
            return { ok = false, error = "depends_on references unknown entry: " .. d }
        end
    end
    data.entries[#data.entries + 1] = entry
    local cyc = find_cycle(data)
    if cyc then
        data.entries[#data.entries] = nil
        return { ok = false, error = "depends_on would create a cycle: "
                    .. table.concat(cyc, " -> ") }
    end
    data.next_seq[line] = seq + 1
    save_spec(args.spec_path, data)
    return { ok = true, entry_id = id }
end

local function find_entry(data, id)
    for _, en in ipairs(data.entries) do if en.id == id then return en end end
    return nil
end

local function act_update_entry(data, args)
    local id = args.entry_id
    local en = id and find_entry(data, id)
    if not en then return { ok = false, error = "entry not found: " .. tostring(id) } end
    if args.summary ~= nil then en.summary = args.summary end
    if args.depends_on ~= nil then
        local depends = {}
        for _, d in ipairs(args.depends_on) do depends[#depends + 1] = tostring(d) end
        local idset = {}
        for _, e2 in ipairs(data.entries) do idset[e2.id] = true end
        for _, d in ipairs(depends) do
            if d == id then return { ok = false, error = "entry cannot depend on itself" } end
            if not idset[d] then
                return { ok = false, error = "depends_on references unknown entry: " .. d }
            end
        end
        en.depends_on = depends
        local cyc = find_cycle(data)
        if cyc then
            return { ok = false, error = "depends_on would create a cycle: "
                        .. table.concat(cyc, " -> ") }
        end
    end
    save_spec(args.spec_path, data)
    return { ok = true }
end

local function act_deprecate_entry(data, args)
    local en = find_entry(data, args.entry_id)
    if not en then return { ok = false, error = "entry not found: " .. tostring(args.entry_id) } end
    en.status = "deprecated"
    save_spec(args.spec_path, data)
    local note = nil
    if en.triggers and #en.triggers > 0 then
        note = "entry still has " .. #en.triggers .. " linked trigger(s); validate() will report them"
    end
    return { ok = true, note = note }
end

local function act_link_trigger(data, args)
    local en = find_entry(data, args.entry_id)
    if not en then return { ok = false, error = "entry not found: " .. tostring(args.entry_id) } end
    if en.status == "deprecated" then
        return { ok = false, error = "cannot link trigger to deprecated entry " .. en.id }
    end
    local ttype = args.trigger_type
    if not ttype or ttype == "" then
        return { ok = false, error = "missing 'trigger_type'" }
    end
    -- 1) existence check against the map (hard error when map is available)
    local exists, val_reason = trigger_exists(ttype)
    if exists == false then
        return { ok = false, error = "trigger_type not found in map [Triggers]: " .. ttype
                    .. " (create the trigger first, then link)" }
    end
    local warnings = {}
    if exists == nil then
        warnings[#warnings + 1] = val_reason or "map trigger validation skipped (map not loaded?)"
    end
    -- 2) duplicate-across-entries warning (allowed)
    for _, e2 in ipairs(data.entries) do
        if e2.id ~= en.id then
            for _, tg in ipairs(e2.triggers or {}) do
                if tg.type == ttype then
                    warnings[#warnings + 1] = "trigger " .. ttype .. " also linked to " .. e2.id
                end
            end
        end
    end
    -- 3) replace existing mapping on the same entry, else append
    local replaced = false
    for _, tg in ipairs(en.triggers or {}) do
        if tg.type == ttype then
            tg.name = args.display_name or ""
            replaced = true
            break
        end
    end
    if not replaced then
        en.triggers[#en.triggers + 1] = { type = ttype, name = args.display_name or "" }
    end
    save_spec(args.spec_path, data)
    local res = { ok = true, entry_id = en.id, trigger_type = ttype }
    if #warnings > 0 then res.warnings = warnings end
    return res
end

local function act_unlink_trigger(data, args)
    local en = find_entry(data, args.entry_id)
    if not en then return { ok = false, error = "entry not found: " .. tostring(args.entry_id) } end
    local removed = false
    for i = #en.triggers, 1, -1 do
        if en.triggers[i].type == args.trigger_type then
            table.remove(en.triggers, i)
            removed = true
        end
    end
    if not removed then
        return { ok = false, error = "trigger not linked to this entry: " .. tostring(args.trigger_type) }
    end
    save_spec(args.spec_path, data)
    return { ok = true }
end

local function act_set_status(data, args)
    local en = find_entry(data, args.entry_id)
    if not en then return { ok = false, error = "entry not found: " .. tostring(args.entry_id) } end
    local st = args.status
    if not STATUSES[st] then
        return { ok = false, error = "invalid status: " .. tostring(st)
                    .. " (expected designing|implemented|verified|deprecated)" }
    end
    en.status = st
    save_spec(args.spec_path, data)
    return { ok = true }
end

-- Import a complete spec payload (produced by the despec workflow) into a
-- fresh spec file. Validates the whole structure before writing: field types,
-- line references, entry id pattern, depends_on existence and acyclicity,
-- status values, and trigger existence in the map. Refuses to overwrite an
-- existing spec (same contract as init).
local function act_import(data, args)
    if data then
        return { ok = false, error = "spec already exists: " .. args.spec_path
                    .. " (import refuses to overwrite; read it first, or remove it to rebuild)" }
    end
    local src = args.data
    if type(src) ~= "table" then
        return { ok = false, error = "missing 'data' (the full spec payload to import)" }
    end
    local issues = {}
    local function err(m) issues[#issues + 1] = m end
    if type(src.title) ~= "string" then err("data.title must be a string") end
    if type(src.story) ~= "string" then err("data.story must be a string") end
    if type(src.lines) ~= "table" then err("data.lines must be an array") end
    if type(src.entries) ~= "table" then err("data.entries must be an array") end
    if #issues > 0 then
        return { ok = false, error = "import validation failed: " .. table.concat(issues, "; ") }
    end
    local line_ids = {}
    for _, ln in ipairs(src.lines) do
        if type(ln.id) ~= "string" or ln.id == "" then
            err("line without a valid id"); break
        end
        if line_ids[ln.id] then err("duplicate line id: " .. ln.id) end
        line_ids[ln.id] = true
        if type(ln.name) ~= "string" then err("line " .. ln.id .. " missing name") end
    end
    local idset = {}
    local next_seq = {}
    for _, en in ipairs(src.entries) do
        if type(en.id) ~= "string" or en.id == "" then
            err("entry without a valid id"); break
        end
        if idset[en.id] then err("duplicate entry id: " .. en.id) end
        idset[en.id] = true
        if not line_ids[en.line] then
            err("entry " .. en.id .. " references unknown line " .. tostring(en.line))
        end
        local line, seq = en.id:match("^(L%d+)S(%d+)$")
        if not line or line ~= en.line then
            err("entry id " .. en.id .. " does not match the LxxSyy pattern of its line")
        end
        local s = tonumber(seq)
        next_seq[en.line] = math.max(next_seq[en.line] or 0, (s or 0) + 1)
        if type(en.summary) ~= "string" then err("entry " .. en.id .. " missing summary") end
        if not STATUSES[en.status] then
            err("entry " .. en.id .. " invalid status " .. tostring(en.status))
        end
        if type(en.triggers) ~= "table" then
            err("entry " .. en.id .. " missing triggers list")
        end
        for _, d in ipairs(en.depends_on or {}) do
            if d == en.id then err("entry " .. en.id .. " depends on itself") end
        end
    end
    for _, en in ipairs(src.entries) do
        for _, d in ipairs(en.depends_on or {}) do
            if not idset[d] then err("entry " .. en.id .. " depends on unknown entry " .. d) end
        end
    end
    if #issues > 0 then
        return { ok = false, error = "import validation failed: " .. table.concat(issues, "; ") }
    end
    local cyc = find_cycle(src)
    if cyc then
        return { ok = false, error = "import would create a depends_on cycle: "
                    .. table.concat(cyc, " -> ") }
    end
    local warnings = {}
    for _, en in ipairs(src.entries) do
        for _, tg in ipairs(en.triggers or {}) do
            if type(tg.type) ~= "string" or tg.type == "" then
                return { ok = false, error = "entry " .. en.id
                            .. " has a trigger mapping without a valid 'type'" }
            end
            local exists, reason = trigger_exists(tg.type)
            if exists == false then
                return { ok = false, error = "trigger_type not found in map [Triggers]: "
                            .. tg.type .. " (linked from entry " .. en.id .. ")" }
            end
            if exists == nil then
                warnings[#warnings + 1] = reason
                    or "map trigger validation skipped (map not loaded?)"
            end
        end
    end
    local fresh = {
        version  = VERSION,
        map_path = args.map_path,
        title    = src.title,
        updated  = now_str(),
        story    = src.story,
        lines    = {},
        entries  = {},
        next_seq = next_seq,
    }
    for _, ln in ipairs(src.lines) do
        fresh.lines[#fresh.lines + 1] = { id = ln.id, name = ln.name }
    end
    for _, en in ipairs(src.entries) do
        local entry = {
            id = en.id, line = en.line,
            summary = en.summary,
            depends_on = {},
            status = en.status,
            triggers = {},
        }
        for _, d in ipairs(en.depends_on or {}) do entry.depends_on[#entry.depends_on + 1] = d end
        for _, tg in ipairs(en.triggers or {}) do
            entry.triggers[#entry.triggers + 1] = { type = tg.type, name = tg.name or "" }
        end
        fresh.entries[#fresh.entries + 1] = entry
    end
    save_spec(args.spec_path, fresh)
    local res = { ok = true, spec_path = args.spec_path,
                  lines = #fresh.lines, entries = #fresh.entries }
    if #warnings > 0 then res.warnings = warnings end
    return res
end

local function act_validate(data, args)
    local issues = {}
    local idset = {}
    for _, en in ipairs(data.entries) do idset[en.id] = true end
    local line_ids = {}
    for _, ln in ipairs(data.lines) do line_ids[ln.id] = true end
    -- structural checks
    for _, en in ipairs(data.entries) do
        if not line_ids[en.line] then
            issues[#issues + 1] = { level = "error", message = en.id .. " references unknown line " .. en.line }
        end
        if not STATUSES[en.status] then
            issues[#issues + 1] = { level = "error", message = en.id .. " has invalid status " .. tostring(en.status) }
        end
        if en.status == "deprecated" and en.triggers and #en.triggers > 0 then
            issues[#issues + 1] = { level = "warning", message = en.id .. " is deprecated but still has linked trigger(s)" }
        end
        for _, d in ipairs(en.depends_on or {}) do
            if d == en.id then
                issues[#issues + 1] = { level = "error", message = en.id .. " depends on itself" }
            elseif not idset[d] then
                issues[#issues + 1] = { level = "error", message = en.id .. " depends on unknown entry " .. d }
            end
        end
    end
    local cyc = find_cycle(data)
    if cyc then
        issues[#issues + 1] = { level = "error", message = "depends_on cycle: " .. table.concat(cyc, " -> ") }
    end
    -- dangling trigger mappings (spec references triggers missing in map)
    local linked = {}
    local refs = {}
    for _, en in ipairs(data.entries) do
        for _, tg in ipairs(en.triggers or {}) do
            refs[#refs + 1] = tg.type
            linked[tg.type] = (linked[tg.type] or 0) + 1
        end
    end
    local map_ids = map_trigger_ids()
    if map_ids then
        local mset = {}
        for _, v in ipairs(map_ids) do mset[v] = true end
        for _, t in ipairs(refs) do
            if not mset[t] then
                issues[#issues + 1] = { level = "error", message = "dangling mapping: trigger " .. t .. " no longer exists in map" }
            end
        end
        -- orphan map triggers (exist in map but never referenced by spec)
        for _, v in ipairs(map_ids) do
            if not linked[v] then
                issues[#issues + 1] = { level = "info", message = "orphan trigger in map (not referenced by spec): " .. v }
            end
        end
    else
        issues[#issues + 1] = { level = "info", message = "map trigger validation skipped (map not loaded?)" }
    end
    local stats = { entries = #data.entries, lines = #data.lines }
    local counts = { designing = 0, implemented = 0, verified = 0, deprecated = 0 }
    for _, en in ipairs(data.entries) do
        local s = en.status or "designing"
        counts[s] = (counts[s] or 0) + 1
    end
    stats.status = counts
    return { ok = true, issues = issues, stats = stats }
end

-- ============================================================================
-- dispatch(json_string) -> json_string
-- ============================================================================

local ACTIONS = {
    init             = act_init,
    read             = act_read,
    update_story     = act_update_story,
    add_line         = act_add_line,
    add_entry        = act_add_entry,
    update_entry     = act_update_entry,
    deprecate_entry  = act_deprecate_entry,
    link_trigger     = act_link_trigger,
    unlink_trigger   = act_unlink_trigger,
    set_status       = act_set_status,
    validate         = act_validate,
    import           = act_import,
}

local function dispatch(json_str)
    local ok, args = pcall(json.decode, json_str)
    if not ok or type(args) ~= "table" then
        return json.encode({ ok = false, error = "cannot parse arguments: " .. tostring(args) })
    end
    local action = args.action
    local handler = ACTIONS[action]
    if not handler then
        return json.encode({ ok = false, error = "unknown action: " .. tostring(action)
            .. " (expected: init, read, update_story, add_line, add_entry, update_entry,"
            .. " deprecate_entry, link_trigger, unlink_trigger, set_status, validate, import)" })
    end
    local map_path = args.map_path
    if not map_path or map_path == "" then
        return json.encode({ ok = false, error = "missing 'map_path'" })
    end
    args.spec_path = derive_spec_path(map_path)
    local ctx = { action = action, map_path = map_path, spec_path = args.spec_path }
    -- Load the spec for every action. read/init are allowed to receive nil
    -- (missing spec); every other action requires an existing, well-formed spec.
    local data = nil
    local reason
    data, reason = load_spec(args.spec_path)
    if not data and reason ~= "no_spec" then
        return json.encode({ ok = false, error = "spec load failed (" .. tostring(reason)
            .. "): " .. args.spec_path })
    end
    if not data and action ~= "read" and action ~= "init" and action ~= "import" then
        return json.encode({ ok = false, error = "no spec found (" .. args.spec_path
            .. "). Run init first." })
    end
    local ok2, res = pcall(handler, data, args)
    if not ok2 then
        return json.encode({ ok = false, error = "action '" .. action .. "' failed: " .. tostring(res) })
    end
    if type(res) ~= "table" then
        return json.encode({ ok = false, error = "action '" .. action .. "' returned invalid result" })
    end
    return json.encode(res)
end

return { dispatch = dispatch, json = json }
