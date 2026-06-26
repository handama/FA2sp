## 十、INI 读取与写入

### 基础读写

#### `get_string(section, key, [default = ""], [load_from = "map"])`
- **说明**：从指定 INI 节和键读取字符串值。
- **参数**：
  - `section` (`string`) — 节名。
  - `key` (`string`) — 键名。
  - `default` (`string`, 可选) — 若不存在时的默认值，默认为 `""`。
  - `load_from` (`string`, 可选) — 读取源，默认为 `"map"`（见附录 C）。
- **返回** (`string`)：读取到的字符串。

#### `get_integer(section, key, [default = 0], [load_from = "map"])`
- **说明**：读取整数值。
- **参数**：
  - `section`, `key` (`string`) — 同上。
  - `default` (`number`, 可选) — 默认值，默认为 `0`。
  - `load_from` (`string`, 可选) — 读取源。
- **返回** (`number`)：整数。

#### `get_float(section, key, [default = 0.0], [load_from = "map"])`
- **说明**：读取浮点数。若原始值末尾含 `%`，会被自动除以 100。
- **参数**：
  - `section`, `key` (`string`)。
  - `default` (`number`, 可选) — 默认为 `0.0`。
  - `load_from` (`string`, 可选)。
- **返回** (`number`)：浮点数。

#### `get_bool(section, key, [default = false], [load_from = "map"])`
- **说明**：读取布尔值。规则：`1`~`9`、`T`、`Y` 开头为 `true`；`0`、`F`、`N` 为 `false`。
- **参数**：
  - `section`, `key` (`string`)。
  - `default` (`boolean`, 可选) — 默认为 `false`。
  - `load_from` (`string`, 可选)。
- **返回** (`boolean`)：布尔值。

#### `write_string(section, key, value)`
- **说明**：向 **地图文件** 写入键值对。不能用于外部 INI。
- **参数**：
  - `section` (`string`) — 节名。
  - `key` (`string`) — 键名。
  - `value` (`string`) — 值。
- **返回**：无。

#### `delete_key(section, key)`
- **说明**：删除地图文件中的键。
- **参数**：
  - `section`, `key` (`string`) — 指定键。
- **返回**：无。

#### `delete_section(section)`
- **说明**：删除地图文件中的整个节。
- **参数**：`section` (`string`) — 节名。
- **返回**：无。

### 枚举与解析

#### `get_sections([load_from = "map"])`
- **说明**：返回所有节名的数组表。
- **参数**：`load_from` (`string`, 可选) — 读取源。
- **返回** (`table<int, string>`)：节名数组，索引从 1 开始。

#### `get_keys(section, [load_from = "map"])`
- **说明**：返回该节下所有键名的数组表。
- **参数**：
  - `section` (`string`) — 节名。
  - `load_from` (`string`, 可选) — 读取源。
- **返回** (`table<int, string>`)：键名数组。

#### `get_values(section, [load_from = "map"])`
- **说明**：返回该节下所有值的数组表（顺序与键对应）。
- **参数**：同上。
- **返回** (`table<int, string>`)：值数组。

#### `get_key_value_pairs(section, [load_from = "map"])`
- **说明**：返回字典（键→值）。
- **参数**：同上。
- **返回** (`table<string, string>`)：键值对字典。

#### `get_ordered_key_value_pairs(section, [load_from = "map"])`
- **说明**：返回有序的键值对列表。每个元素是一个含有 `[1] = key, [2] = value` 的表。
- **参数**：同上。
- **返回** (`table<int, table>`)：有序键值对列表。

#### `get_ordered_values(section, [load_from = "map"])`
- **说明**：将节视为注册表，返回 **从 0 开始的整数键** 到值的对应表。**注意**：Lua 表数组部分从 1 开始，所以 0 号键位于最后。
- **参数**：同上。
- **返回** (`table<number, string>`)：整数键到值的映射表。

### 参数操作

#### `split_string(str, [delimiter = ","])`
- **说明**：按分隔符拆分字符串（仅支持单字符），返回数组表。
- **参数**：
  - `str` (`string`) — 要拆分的字符串。
  - `delimiter` (`string`, 可选) — 分隔符，默认为 `","`。
- **返回** (`table<string>`)：拆分后的字符串数组。
- **示例**：
```lua
local parts = split_string("20-24,19,30", ",")
for _, part in ipairs(parts) do
    -- 进一步处理
end
```

#### `get_param(section, key, index, [delimiter = ","], [load_from = "map"])`
- **说明**：获取 INI 中逗号分隔值的第 `index` 项（**从 1 开始**）。
- **参数**：
  - `section`, `key` (`string`) — 节和键。
  - `index` (`number`) — 索引（1 基）。
  - `delimiter` (`string`, 可选) — 分隔符，默认为 `","`。
  - `load_from` (`string`, 可选) — 读取源。
- **返回** (`string`)：指定索引的字符串项。
- **典型用法**：获取触发的名称（Triggers 节的第 3 个参数）：
```lua
local name = get_param("Triggers", trigger_id, 3)
```

#### `set_param(section, key, value, index, [delimiter = ","])`
- **说明**：修改 INI 中逗号分隔值的第 `index` 项，并写回地图。索引从 1 开始。
- **参数**：
  - `section`, `key` (`string`) — 节和键。
  - `value` (`string`) — 新值。
  - `index` (`number`) — 索引。
  - `delimiter` (`string`, 可选) — 分隔符，默认为 `","`。
- **返回**：无。

#### `get_param_str(str, index, [delimiter = ","])` 
- **说明**：直接解析逗号分隔字符串，返回第 `index` 项。
- **参数**：
  - `str` (`string`) — 输入字符串。
  - `index` (`number`) — 索引（1 基）。
  - `delimiter` (`string`, 可选) — 分隔符，默认为 `","`。
- **返回** (`string`)：第 `index` 项。

#### `set_param_str(str, value, index, [delimiter = ","])` 
- **说明**：修改字符串中逗号分隔值的第 `index` 项，返回新字符串。
- **参数**：
  - `str` (`string`) — 原始字符串。
  - `value` (`string`) — 新值。
  - `index` (`number`) — 索引。
  - `delimiter` (`string`, 可选) — 分隔符，默认为 `","`。
- **返回** (`string`)：修改后的新字符串。

#### `trim_index(value)`
- **说明**：截断第一个空格后的部分，常用于处理带名字的索引。
- **参数**：`value` (`string`) — 输入字符串。
- **返回** (`string`)：截断后的字符串。

### 特殊 ID 与转换

#### `get_free_waypoint()`
- **说明**：返回第一个未使用的路径点数字字符串（如 `"5"`）。
- **返回** (`string`)：未使用的路径点数字字符串。

#### `get_free_key(section)`
- **说明**：返回指定节中第一个未使用的数字键（从 0 开始自增）。
- **参数**：`section` (`string`) — 节名。
- **返回** (`string`)：未使用的数字键字符串。

#### `get_free_id([type = 0])`
- **说明**：返回可用于触发、标签、小队等的第一个可用 ID。若偏好设置中启用了 `UseSeparateIndexing`，则 ID 会带有对应后缀（如 `"00000001-TR"`）。
- **参数**：
  - `type` (`number`, 可选) — ID 类型：0=通用，1=Trigger，2=Tag，3=Team，4=Script，5=TaskForce，6=AITrigger。默认为 0。
- **返回** (`string`)：可用 ID 字符串。
- **示例**：创建新的触发或小队时，通常用此函数获取未被占用的 ID。
```lua
local new_id = get_free_id()   -- 通用ID
local trigger = trigger:new(new_id)
```

#### `waypoint_to_string(wp)`
- **说明**：将数字路径点转为字母格式（如 `"A"`、`"AA"`）。
- **参数**：`wp` (`number` 或 `string`) — 路径点数字。
- **返回** (`string`)：字母格式路径点。

#### `string_to_waypoint(str)`
- **说明**：将字母格式路径点转为数字字符串（如 `"A"` → `"0"`）。
- **参数**：`str` (`string`) — 字母格式路径点。
- **返回** (`string`)：数字字符串路径点。

#### `get_uiname(id)`
- **说明**：获取类型 ID 对应的 UI 名称（如 `"HTNK"`）。
- **参数**：`id` (`string`) — 类型 ID。
- **返回** (`string`)：UI 名称。

#### `translate_house(id, [back = false])`
- **说明**：翻译阵营/国家名称。
- **参数**：
  - `id` (`string`) — 输入的名称或 ID。
  - `back` (`boolean`, 可选) — 为 `true` 时从译文反向还原原文，默认为 `false`。
- **返回** (`string`)：翻译后的名称。
