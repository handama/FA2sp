
# Lua 脚本 API 文档

本文档详细描述了 FA2spHE 中 Lua 脚本环境所提供的所有函数、数据类型以及它们的用法。
文档中已附带了典型用例和注意事项，帮助理解具体使用方法。

---

## 基本约定

- **对象索引**：直接接受“索引”作为参数的函数（例如 `remove_unit(index)`）使用 **0 基索引**（C++ 内部数组下标）。返回的 Lua 数组表（如 `get_units()` 的结果）索引从 **1** 开始。具体约定以实际函数说明为准。
- **修改生效**：大部分地图对象的增删改不会立即刷新到界面，而是在 **脚本执行完毕后** 统一刷新。可以使用 `update_*` 系列函数触发刷新，也可以用 `redraw_window()` 立即重绘。触发、小队等需要通过对象的 `apply()` 方法写入 INI 文件。
  **重要**：对于通过 `get_infantry()`、`get_unit()` 等获取的对象，直接修改其属性（如 `facing`）后，必须将其从地图删除再重新放置（或调用 `place()` 方法），才能让修改生效并最终写入 INI。
- **默认值**：参数列表中以 `[可选参数 = 默认值]` 的形式标出。若不传递，则使用默认值。
- **类型**：`number` 为 Lua 整数或浮点数；`string` 为字符串；`boolean` 为 `true`/`false`；`table` 为 Lua 表；`object` 为特定类的实例；`nil` 表示无返回值或空。
- **文本编码**：程序内部均使用 ANSI（GBK） 编码，MCP 服务器输出的则为 UTF-8 编码。MCP 服务器会自动对编码进行转换，但 Lua 脚本直接读取/保存的文件则不会自动转换。
- **错误与警告**：部分操作会向输出窗口打印错误信息，本文档不再逐一说明。

---

## 一、全局状态变量（只读函数）

| 函数 | 返回类型 | 说明 |
|------|----------|------|
| `iso_size()` | `number` | 地图完整范围的宽度（宽+高，遍历单元格用） |
| `width()` | `number` | 地图有效范围的宽度（玩家视角） |
| `height()` | `number` | 地图有效范围的高度（玩家视角） |
| `local_width()` | `number` | 当前可视区域宽度（玩家视角） |
| `local_height()` | `number` | 当前可视区域高度（玩家视角） |
| `local_top()` | `number` | 可视区域距顶端的偏移 |
| `local_left()` | `number` | 可视区域距左侧的偏移 |
| `waypoint_count()` | `number` | 路径点数量（`Waypoints` 节键数量） |
| `unit_count()` | `number` | 车辆（Units）数量 |
| `infantry_count()` | `number` | 步兵（Infantry）数量 |
| `building_count()` | `number` | 建筑（Structures）数量 |
| `aircraft_count()` | `number` | 飞行器（Aircraft）数量 |
| `terrain_count()` | `number` | 地形对象（Terrain）数量 |
| `smudge_count()` | `number` | 污染（Smudge）数量 |
| `player_count()` | `number` | 玩家数量。多人地图返回路径点 0~7 的实际数量；单人地图返回 1 |
| `house_count()` | `number` | 所有阵营/所属方数量（`Houses` 节计数） |
| `country_count()` | `number` | 国家数量（`Countries` 节计数） |
| `node_count([house])` | `number` | 基地节点总数。若提供 `house` 参数，只统计该阵营的节点；否则统计全部 |
| `trigger_count()` | `number` | 触发（Triggers）数量 |
| `tile_count()` | `number` | 当前地形类型的 **地形** 种类总数 |
| `tile_set_count()` | `number` | 当前地形类型的 **地形组** 数量 |
| `tag_count()` | `number` | 标签（Tags）数量 |
| `theater()` | `string` | 当前地图地形名称，如 `"TEMPERATE"` |
| `is_multiplay()` | `boolean` | 是否为多人地图 |
| `language()` | `string` | 当前地编界面语言 |
| `exe_path()` | `string` | 地编程序所在目录路径 |
| `game_path()` | `string` | 游戏资源目录（含 mix 文件）路径 |
| `map_path()` | `string` | 当前地图文件的完整路径 |
| `scale_factor()` | `number` | 当前程序的缩放倍率（浮点数） |
| `available_houses()` | `table` | 当前可用的所属方（阵营）列表，每个元素为一个字符串 |

**重要：遍历单元格时，应遵循如下示例，而不是使用`width()`和`height()`**
**使用示例**：
```lua
-- 遍历地图上所有单元格
for x = 0, iso_size() - 1 do
    for y = 0, iso_size() - 1 do
        -- 操作每个单元格
    end
end
```

---

## 二、输出与交互

### `print(...)`
- **说明**：在输出窗口打印文本。可以接受任意数量和类型的参数，每个参数用空格分隔。
- **参数**：`...` — 可变参数，支持 `nil`, `string`, `number`, `boolean`。
- **返回**：无。
- **示例**：`print("Hello", 123, true)`

### `clear()`
- **说明**：清空输出窗口中的文本。
- **返回**：无。

### `avoid_time_out()`
- **说明**：向主程序分发一次消息，防止脚本执行时间过长导致界面卡死（“未响应”）。在长时间循环中应适时调用。
- **返回**：无。
- **使用示例**：
```lua
for i = 1, 10000 do
    -- 大量操作
    if i % 1000 == 0 then
        avoid_time_out()
    end
end
```

### `sleep(ms)`
- **说明**：暂停脚本执行指定的毫秒数。
- **参数**：`ms` (`number`) — 毫秒数。
- **返回**：无。

### `message_box(text, title, format)`
- **说明**：弹出消息对话框。
- **参数**：
  - `text` (`string`) — 显示的消息正文。
  - `title` (`string`) — 对话框标题。
  - `format` (`number`) — 消息框样式（见附录 A）。
- **返回** (`number`)：用户按下的按钮序号（从 1 开始），若无法识别则返回 -1。
- **示例**：
```lua
local ret = message_box("是否保存？", "提示", 2)
if ret == 1 then
    -- 按下了“是”
end
```

### `input_box(message)`
- **说明**：弹出输入对话框，让用户输入一行文本。
- **参数**：`message` (`string`) — 提示文字。
- **返回** (`string`)：用户输入的文本，取消时可能为空。
- **示例**：
```lua
local user_input = input_box("请输入新的触发名称")
```

### `read_input()`
- **说明**：读取 Lua 控制台输入窗口中的当前文本。
- **返回** (`string`)：输入框里的全部文本。

### `get_file_encoding(data)`
- **说明**：检测字节数据的字符编码格式。
- **参数**：`data` (`string`) — 要检测的原始字节数据。
- **返回** (`string`)：编码类型名称。可能的值：`"ANSI"`、`"UTF-8"`、`"UTF-8 BOM"`、`"UTF-8 (ASCII)"`、`"Unknown"`。
- **示例**：
```lua
local content = open_file()
local enc = get_file_encoding(content)
print("文件编码: " .. enc)
```

### `to_ansi(utf8_str)`
- **说明**：将 UTF-8 编码的字符串转换为 ANSI（GBK）编码，如已为 ANSI（GBK） 编码则不做变化。
- **参数**：`utf8_str` (`string`) — UTF-8 编码的字符串。
- **返回** (`string`)：转换后的 ANSI 编码字符串。

### `to_utf8(ansi_str)`
- **说明**：将 ANSI（GBK）编码的字符串转换为 UTF-8 编码，如已为 UTF-8 编码则不做变化。
- **参数**：`ansi_str` (`string`) — ANSI 编码的字符串。
- **返回** (`string`)：转换后的 UTF-8 编码字符串。

### `open_file()`
- **说明**：打开文件选择对话框，读取用户选择的文件。
- **返回** (`string`)：文件内容；若取消或失败返回空字符串。

### `save_file(content)`
- **说明**：打开文件保存对话框，将内容写入用户选择的文件。
- **参数**：`content` (`string`) — 要保存的文本。
- **返回** (`boolean`)：`true` 表示保存成功，`false` 表示失败。

### `end_script()`
- **说明**：立即终止当前 Lua 脚本的执行。
- **返回**：无。

### `exec(command, [options])`
- **说明**：执行外部程序/命令、打开 URL 或文件，支持同步/异步模式和输出捕获。
- **参数**：
  - `command` (`string`) — 要执行的命令、程序路径、URL 或文件路径。
  - `options` (`table`, 可选) — 配置选项表，支持以下键：
    - `async` (`boolean`, 默认 `false`) — 异步执行，不等待程序完成即返回。
    - `url` (`boolean`, 默认 `false`) — 当作 URL 用默认浏览器打开（不可与 `file` 同时使用）。
    - `file` (`boolean`, 默认 `false`) — 用系统关联的默认程序打开文件（不可与 `url` 同时使用）。
    - `capture` (`boolean`, 默认 `true`) — 捕获 stdout/stderr 输出并作为返回值。异步模式下自动设为 `false`。
    - `stream` (`boolean`, 默认 `false`) — 将命令输出实时写入 Lua Console 输出窗口（需 `capture=true`）。
    - `show` (`boolean`, 默认 `false`) — 显示进程窗口。
    - `cwd` (`string`, 默认 `""`) — 工作目录路径，为空则使用程序默认目录。
    - `timeout` (`number`, 默认 `0`) — 超时毫秒数，`0` 表示无限等待。
- **返回**：
  - 同步+capture 模式：返回命令输出内容的字符串。
  - 其他模式：成功返回 `true`，失败返回 `nil`。
- **错误处理**：失败时自动在 Lua Console 输出错误信息，同时返回 `nil`。
- **典型用法**：
```lua
-- 同步执行 ping 并捕获输出
local result = exec("ping 127.0.0.1 -n 3")
print("Ping result:")
print(result)

-- 实时查看 ping 输出
exec("ping 127.0.0.1 -n 5", { stream = true })

-- 在默认浏览器中打开网页
exec("https://github.com/handama/FA2sp", { url = true })

-- 用默认程序打开文件
exec("FA2sp.log", { file = true })

-- 带超时控制
local out = exec("ping 127.0.0.1 -n 10", { timeout = 5000 })
if out then
    print(out)
else
    print("Command timed out or failed")
end

-- 指定工作目录
exec("debug.log", { cwd = game_path().."debug\\", file = true })
```

---

### `select_box` 类

#### `select_box:new(title)` （构造函数）
- **说明**：创建单选对话框实例。
- **参数**：`title` (`string`) — 对话框标题。
- **返回** (`select_box`)：`select_box` 实例。
- **示例**：`local s = select_box("选择一个国家")`

**成员**：

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `caption` | `string` | 读/写 | 对话框标题 |
| `selected_key` | `string` | 只读 | 用户最后选择的选项键（`do_modal` 之后有效） |
| `selected_value` | `string` | 只读 | 用户最后选择的选项值 |

**方法**：

#### `add_option(key, [value = ""])`
- **说明**：添加一个选项。
- **参数**：
  - `key` (`string`) — 内部标识。
  - `value` (`string`, 可选) — 显示名，若不提供则只显示 `key`。默认为空字符串。
- **返回**：无。

#### `sort_options([by_value = false])`
- **说明**：对所有选项排序。
- **参数**：
  - `by_value` (`boolean`, 可选) — 若为 `true` 则按值排序，否则按键排序。默认为 `false`。
- **返回**：无。

#### `do_modal()`
- **说明**：弹出对话框。
- **返回** (`string`)：返回选中的 `key`；若取消则返回 `""`。
- **典型用法**：
```lua
local s = select_box:new("选择一个国家")
-- 从 Countries 节获取所有国家并加入选项
for i, house in pairs(get_values("Countries", "rules+map")) do
    s:add_option(house, translate_house(house))
end
s:sort_options(true)
local chosen = s:do_modal()
if chosen ~= "" then
    print("选择了：" .. chosen)
end
```

---

### `multi_select_box` 类

#### `multi_select_box:new(title)` （构造函数）
- **说明**：创建多选对话框实例。
- **参数**：`title` (`string`) — 对话框标题。
- **返回** (`multi_select_box`)：多选对话框实例。

**成员**：

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `caption` | `string` | 读/写 | 对话框标题 |
| `selected_keys` | `table` (字符串数组) | 只读 | 所有被选中项的键 |
| `selected_values` | `table` (字符串数组) | 只读 | 所有被选中项的值 |

**方法**：

#### `add_option(key, [value = ""])`
- **说明**：添加一个选项。
- **参数**：同 `select_box` 的 `add_option`。
- **返回**：无。

#### `sort_options([by_value = false])`
- **说明**：对所有选项排序。
- **参数**：同 `select_box` 的 `sort_options`。
- **返回**：无。

#### `do_modal()`
- **说明**：弹出多选对话框。
- **返回** (`table`)：包含所有选中项 `key` 的数组表。
- **示例**：遍历触发并提供多选，然后批量修改属性。
```lua
local box = multi_select_box:new("请选择触发")
for i, id in pairs(get_keys("Triggers")) do
    box:add_option(id, get_param("Triggers", id, 3))
end
box:sort_options(true)
local selected_triggers = box:do_modal()
for i, id in ipairs(selected_triggers) do
    local t = get_trigger(id)
    t.disabled = true
    t:apply()
end
```

---

### `LuaDialog` 类

自定义模态对话框，支持手动布局和自动排布两种模式，可添加 CheckBox、Edit、ComboBox、ListBox、MultiListBox 等控件。

#### `LuaDialog:new(title, [autoLayout = false], [width = 420], [height = 320])` （构造函数）
- **说明**：创建自定义对话框实例。当 `autoLayout` 为 `true` 时，`width` 和 `height` 会被忽略，窗口尺寸根据控件自动计算。
- **参数**：
  - `title` (`string`) — 对话框标题。
  - `autoLayout` (`boolean`, 可选) — 是否启用自动排布。默认为 `false`。
  - `width` (`number`, 可选) — 对话框宽度（像素）。默认为 `420`。
  - `height` (`number`, 可选) — 对话框高度（像素）。默认为 `320`。
- **返回** (`LuaDialog`)：对话框实例。

**方法**：

#### `add_checkbox(key, label, default_value, [x = 0], [y = 0], [w = 0], [h = 0])`
- **说明**：添加一个复选框控件。`label` 为控件自带文本。
- **参数**：
  - `key` (`string`) — 返回值键名。
  - `label` (`string`) — 控件显示文本。
  - `default_value` (`boolean`) — 默认选中状态。
  - `x, y, w, h` (`number`, 可选) — 位置与尺寸。自动排布模式下可省略。
- **返回**：无。

#### `add_edit(key, label, [default_value = ""], [x = 0], [y = 0], [w = 0], [h = 0])`
- **说明**：添加一个文本输入框控件。`label` 为控件上方静态标签。
- **参数**：
  - `key` (`string`) — 返回值键名。
  - `label` (`string`) — 标签文本。
  - `default_value` (`string`, 可选) — 默认值。默认为 `""`。
  - `x, y, w, h` (`number`, 可选) — 位置与尺寸。自动排布模式下可省略。
- **返回**：无。

#### `add_combobox(key, label, items, [default_value = ""], [readOnly = false], [x = 0], [y = 0], [w = 0], [h = 0])`
- **说明**：添加一个下拉框控件。`label` 为控件上方静态标签。此控件可以在编辑框内输入文本执行搜索，且性能更好，当选项列表为地图触发等大批量内容时，建议使用本窗口而不是 `select_box`。
- **参数**：
  - `key` (`string`) — 返回值键名。
  - `label` (`string`) — 标签文本。
  - `items` (`table<string>`) — 下拉选项列表（Lua 数组）。
  - `default_value` (`string`, 可选) — 默认选中项。默认为 `""`。
  - `readOnly` (`boolean`, 可选) — 是否只读（可以编辑文本执行搜索）。默认为 `false`。
  - `x, y, w, h` (`number`, 可选) — 位置与尺寸。自动排布模式下可省略。
- **返回**：无。

#### `add_listbox(key, label, items, [x = 0], [y = 0], [w = 0], [h = 0])`
- **说明**：添加一个单选列表框控件。`label` 为控件上方静态标签。
- **参数**：
  - `key` (`string`) — 返回值键名。
  - `label` (`string`) — 标签文本。
  - `items` (`table<string>`) — 列表选项（Lua 数组）。
  - `x, y, w, h` (`number`, 可选) — 位置与尺寸。自动排布模式下可省略。
- **返回**：无。

#### `add_multilistbox(key, label, items, [x = 0], [y = 0], [w = 0], [h = 0])`
- **说明**：添加一个多选列表框控件。`label` 为控件上方静态标签。
- **参数**：
  - `key` (`string`) — 返回值键名。
  - `label` (`string`) — 标签文本。
  - `items` (`table<string>`) — 列表选项（Lua 数组）。
  - `x, y, w, h` (`number`, 可选) — 位置与尺寸。自动排布模式下可省略。
- **返回**：无。

#### `add_label(key, text, [x = 0], [y = 0], [w = 0], [h = 0])`
- **说明**：添加一个纯文本标签控件。与 Edit 类似但只读，不参与对话框返回值。可通过 `set_text`/`set_visible`/`set_enabled` 等运行时方法操作。
- **参数**：
  - `key` (`string`) — 控件 key，用于后续操作。
  - `text` (`string`) — 显示文本。
  - `x, y, w, h` (`number`, 可选) — 位置与尺寸。自动排布模式下高度会根据文本内容通过 `DrawText` 真实渲染计算（自动换行）。
- **返回**：无。

#### `do_modal()`
- **说明**：弹出模态对话框，阻塞脚本直到用户关闭。
- **返回** (`table | nil`)：若用户点击确定，返回一个表，键为控件注册的 `key`，值为对应的用户输入：
  - CheckBox：`boolean`
  - Edit：`string`
  - ComboBox：`string`
  - ListBox：`string`（选中项）
  - MultiListBox：`table<string>`（选中项数组）
  
  若用户点击取消，返回 `nil`。

#### `on_event(key, eventType, callback)`
- **说明**：注册控件交互回调。当指定控件发生指定事件时，自动调用 callback。回调函数接收一个参数 `ctrlKey`（触发事件的控件 key），可通过 closure 捕获的 `dlg` 变量调用其他 API 方法来实现控件间交互。对话框打开时会自动触发一次初始事件，用于应用默认状态。
- **参数**：
  - `key` (`string`) — 控件 key（注册控件时指定的 key）。
  - `eventType` (`string`) — 事件类型，支持 `|` 分隔多个类型（如 `"selchange|editchange"` 任一匹配即触发）：
    - `"changed"` — CheckBox 点击切换后触发
    - `"selchange"` — ComboBox 选中项变化时触发
    - `"editchange"` — ComboBox 输入文本变化时触发（仅非只读）
    - `"change"` — Edit 文本变化时触发
  - `callback` (`function`) — 回调函数，签名 `function(ctrlKey)`。
- **返回**：无。

#### `get_bool(key)`
- **说明**：读取 CheckBox 控件的当前勾选状态（从控件实时读取，非 DoModal 结果缓存）。
- **参数**：`key` (`string`) — 控件 key。
- **返回** (`boolean`)：`true` 表示已勾选，`false` 表示未勾选。

#### `get_string(key)`
- **说明**：读取 Edit 或 ComboBox 控件的当前文本内容（从控件实时读取）。
- **参数**：`key` (`string`) — 控件 key。
- **返回** (`string`)：控件的当前文本。

#### `get_enabled(key)`
- **说明**：查询指定控件当前是否处于启用状态。
- **参数**：`key` (`string`) — 控件 key。
- **返回** (`boolean`)：`true` 表示启用，`false` 表示禁用。

#### `get_visible(key)`
- **说明**：查询指定控件当前是否可见。
- **参数**：`key` (`string`) — 控件 key。
- **返回** (`boolean`)：`true` 表示可见，`false` 表示隐藏。

#### `set_enabled(key, enabled)`
- **说明**：启用或禁用指定控件。禁用的控件呈灰色，无法与用户交互。
- **参数**：
  - `key` (`string`) — 控件 key。
  - `enabled` (`boolean`) — `true` 为启用，`false` 为禁用。
- **返回**：无。

#### `set_visible(key, visible)`
- **说明**：显示或隐藏指定控件。
- **参数**：
  - `key` (`string`) — 控件 key。
  - `visible` (`boolean`) — `true` 为显示，`false` 为隐藏。
- **返回**：无。

#### `set_list_items(key, items)`
- **说明**：重新加载 ListBox 的列表项（清除原有项并填入新项）。
- **参数**：
  - `key` (`string`) — 控件 key。
  - `items` (`table<string>`) — 新的列表项数组。
- **返回**：无。

#### `set_combo_items(key, items)`
- **说明**：重新加载 ComboBox 的列表项（清除原有项并填入新项），并自动选中第一项。
- **参数**：
  - `key` (`string`) — 控件 key。
  - `items` (`table<string>`) — 新的列表项数组。
- **返回**：无。

#### `set_text(key, text)`
- **说明**：设置 Edit 或 ComboBox 的文本内容。
- **参数**：
  - `key` (`string`) — 控件 key。
  - `text` (`string`) — 要设置的文本。
- **返回**：无。

#### `set_check(key, checked)`
- **说明**：设置 CheckBox 的勾选状态。
- **参数**：
  - `key` (`string`) — 控件 key。
  - `checked` (`boolean`) — `true` 为勾选，`false` 为取消勾选。
- **返回**：无。

#### `set_position(key, x, y, w, h)`
- **说明**：移动并调整指定控件的位置和尺寸（逻辑坐标，会自动乘以缩放系数）。当某个参数为 `0` 时，该维度保持不变。
- **参数**：
  - `key` (`string`) — 控件 key。
  - `x, y, w, h` (`number`) — 新的位置与尺寸（像素）。为 `0` 表示该维度不变。
- **返回**：无。

#### `set_window_size(width, height)`
- **说明**：调整整个对话框窗口的尺寸（逻辑坐标），同时自动重排确认/取消按钮到新位置。当某个参数为 `0` 时，该维度保持不变。
- **参数**：
  - `width` (`number`) — 新的窗口宽度（像素）；`0` 表示不变。
  - `height` (`number`) — 新的窗口高度（像素）；`0` 表示不变。
- **返回**：无。

**典型用法**：

**示例一：手动布局**
```lua
local dlg = LuaDialog:new("地图设置", false, 225, 480)

dlg:add_checkbox("fog", "启用战争迷雾", true, 10, 10, 200, 18)
dlg:add_edit("author", "作者名", "玩家", 10, 30, 200, 18)
dlg:add_combobox("size", "地图大小", {"小型", "中型", "大型"}, "中型", false, 10, 70, 200, 18)
dlg:add_label("desc", "这是一段说明文字", 10, 110, 200, 18)
dlg:add_multilistbox("sides", "可用阵营", {"盟军", "苏联", "尤里", "GDI", "Nod"}, 10, 130, 200, 120)
dlg:add_listbox("tech", "科技等级", {"T1", "T2", "T3", "T4"}, 10, 270, 200, 120)

local result = dlg:do_modal()
if result then
    print("迷雾: " .. tostring(result.fog))
    print("作者: " .. result.author)
    print("地图大小: " .. result.size)
    for i = 1, #result.sides do
        print("可用阵营: " .. result.sides[i])
    end
    print("科技等级: " .. result.tech)
else
    print("用户取消了对话框")
end
```

**示例二：自动排布模式**
```lua
local dlg = LuaDialog:new("地图设置", true)

dlg:add_checkbox("fog", "启用战争迷雾", true)
dlg:add_edit("author", "作者名", "玩家")
dlg:add_combobox("size", "地图大小", {"小型", "中型", "大型"}, "中型")
dlg:add_label("desc", "这是一段说明文字，在自动排布模式下可以自行计算高度")
dlg:add_multilistbox("sides", "可用阵营", {"盟军", "苏联", "尤里", "GDI", "Nod"})
dlg:add_listbox("tech", "科技等级", {"T1", "T2", "T3", "T4"})

local result = dlg:do_modal()
if result then
    print("迷雾: " .. tostring(result.fog))
    print("作者: " .. result.author)
    print("地图大小: " .. result.size)
    for i = 1, #result.sides do
        print("可用阵营: " .. result.sides[i])
    end
    print("科技等级: " .. result.tech)
else
    print("用户取消了对话框")
end
```

**示例三：控件交互**
```lua
local dlg = LuaDialog:new("交互示例", true)

dlg:add_checkbox("enable_extra", "启用额外选项", false)
dlg:add_edit("extra_name", "额外名称", "默认值")
dlg:add_combobox("type", "类型", {"A", "B", "C"}, "A", false)
dlg:add_listbox("items", "选项列表", {"a1", "a2"})

-- CheckBox 勾选时启用/禁用 Edit（初始默认 false 会触发，从而禁用 Edit）
dlg:on_event("enable_extra", "changed", function(key)
    dlg:set_enabled("extra_name", dlg:get_bool("enable_extra"))
end)

-- ComboBox 选中项\输入变化时重新加载 ListBox
dlg:on_event("type", "selchange", function(key)
    local v = dlg:get_string("type")
    if v == "A" then
        dlg:set_list_items("items", {"a1", "a2", "a3"})
        dlg:set_text("extra_name", "选项A")
    elseif v == "B" then
        dlg:set_list_items("items", {"b1", "b2"})
        dlg:set_text("extra_name", "选项B")
    else
        dlg:set_list_items("items", {"c1", "c2", "c3", "c4"})
        dlg:set_text("extra_name", "选项C")
    end
end)

local result = dlg:do_modal()
if result then
    print("启用额外选项: " .. tostring(result.enable_extra))
    print("额外名称: " .. result.extra_name)
    print("类型: " .. result.type)
    print("选项列表: " .. result.items)
end
```

**自动排布规则**：
- 控件从上到下纵向排列，起始位置 `(10, 10)`。
- 当累计高度超过 500 像素时，自动换到下一列（列间距 215 像素）。
- 控件默认尺寸：CheckBox/Edit/ComboBox 为 `200×18`，ListBox 为 `200×120`，Label 为 `200×h`（h 根据文本长度自动计算）。
- 步进间距：CheckBox/Label 为 `h + 6`，其余为 `h + 22`（含标签高度）。

---

## 三、地形与覆盖物

### `place_terrain(x, y, id)`
- **说明**：在指定单元格放置地形对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `id` (`string`) — 地形对象类型（如 `"TREE01"` ）。
- **返回**：无。
- **注意**：只影响地形对象（树木、石头等），不是地形块（tile）。

### `remove_terrain(x, y)`
- **说明**：删除指定单元格上的地形对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

### `place_smudge(x, y, id)`
- **说明**：在指定单元格放置污染/粒子效果。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `id` (`string`) — 污染类型标识。
- **返回**：无。

### `remove_smudge(x, y)`
- **说明**：删除指定单元格的污染。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

### `place_overlay(x, y, overlay, [overlay_data = -1])`
- **说明**：在单元格上绘制覆盖物（矿石、公路、地板等）。超出上限的值会被拒绝。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `overlay` (`number`) — 覆盖物索引，`-1` 表示保留当前覆盖物。
  - `overlay_data` (`number`, 可选) — 覆盖物数据，`-1` 表示保留当前值。默认为 `-1`。
- **返回**：无。

### `remove_overlay(x, y)`
- **说明**：移除单元格上的覆盖物（设为空）。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

### `smooth_ore(x1, y1, x2, y2)`
- **说明**：在矩形范围内平滑矿石过渡。若四个参数均为 `0`，则处理全图。坐标会自动调整为从小到大的范围。
- **参数**：
  - `x1, y1, x2, y2` (`number`) — 矩形对角坐标。
- **返回**：无。
- **示例**：`smooth_ore(10,10,50,50)` 或者 `smooth_ore(0,0,0,0)` 平滑全图。

### `place_wall(x, y, wall, damage_stage)`
- **说明**：放置围墙。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `wall` (`number`) — 围墙类型索引（从 0 开始）。
  - `damage_stage` (`number`) — 破损状态：1 = 完好，2 = 受损，3 = 严重受损；0 表示随机选择一种状态。
- **返回**：无。

---

## 四、路径点

### `get_waypoint(index)`
- **说明**：获取指定索引（`cell.waypoint`）的路径点数字。
- **参数**：
  - `index` (`number`) — 路径点索引。
- **返回** (`number`)：路径点 ID。如果该索引不存在，则返回 `-1`。

### `place_waypoint(x, y, [id = -1])`
- **说明**：在指定坐标放置路径点。若该格已有路径点，则不放置并返回该点 ID。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `id` (`number`, 可选) — 指定的路径点数字 ID，`-1` 表示自动分配。默认为 `-1`。
- **返回** (`number`)：放置的路径点 ID。
- **示例**：`local wid = place_waypoint(10, 20, 5)`

### `remove_waypoint(id)`
- **说明**：删除指定数字 ID 的路径点。
- **参数**：`id` (`number`) — 路径点 ID。
- **返回**：无。

### `remove_waypoint_at(x, y)`
- **说明**：删除指定坐标上的路径点。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

---

## 五、步兵 (`infantry`)

### 构造函数

#### `infantry(house, type, x, y)`
- **说明**：构造一个步兵对象（不立即放置到地图，需调用 `place()` 方法）。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 步兵类型 ID。
  - `x` (`number`) — X 坐标。
  - `y` (`number`) — Y 坐标。
- **返回** (`infantry`)：步兵对象。

### 成员

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `house` | `string` | 读/写 | 所属方。可用所属方使用`available_houses()`获取 |
| `type` | `string` | 读/写 | 步兵类型 ID |
| `health` | `string` | 读/写 | 生命值（字符串形式，如 "100"），取值0~256 |
| `x` | `number` | 读/写 | X 坐标 |
| `y` | `number` | 读/写 | Y 坐标 |
| `subcell` | `string` | 读/写 | 子单元格位置：0~4；0、1为中心，2为右侧，3为左侧，4为下侧；"-1" 表示不指定 |
| `status` | `string` | 读/写 | 状态（如 "Guard"） |
| `facing` | `string` | 读/写 | 面向，取值0~256 |
| `tag` | `string` | 读/写 | 关联的标签 ID |
| `veterancy` | `string` | 读/写 | 经验等级：0=新兵，100=老兵，200=精英 |
| `group` | `string` | 读/写 | 分组 |
| `above_ground` | `string` | 读/写 | 是否在桥上 ("0" 或 "1") |
| `auto_no_recruit` | `string` | 读/写 | 重组 A |
| `auto_yes_recruit` | `string` | 读/写 | 重组 B |

### 方法

#### `place()`
- **说明**：将当前步兵添加到地图（延迟刷新）。需脚本结束后或手动刷新才能看到。
- **返回**：无。

#### `remove()`
- **说明**：删除地图中属性完全相同的 **第一个** 步兵。
- **返回**：无。

### 全局函数

#### `place_infantry(house, type, x, y)`
- **说明**：快速放置步兵，使用默认属性（遵循格式预设），立即计划刷新。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 步兵类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回**：无。

#### `remove_infantry(index)`
- **说明**：通过 **0 基索引** 删除步兵。
- **参数**：`index` (`number`) — 步兵在内部数组中的索引（从 0 开始）。
- **返回**：无。

#### `remove_infantry(x, y, [sub_cell = -1])`
- **说明**：删除指定坐标（及可选子格）的步兵。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `sub_cell` (`number`, 可选) — 子单元格位置，`-1` 表示忽略子格。默认为 `-1`。
- **返回**：无。

#### `get_infantry(index)`
- **说明**：通过 **0 基索引** 获取步兵对象。
- **参数**：`index` (`number`) — 0 基索引。
- **返回** (`infantry`)：步兵对象；若未找到，返回一个 `x,y` 为 `-1` 的无效对象。

#### `get_infantry(x, y, [sub_cell = -1])`
- **说明**：获取指定坐标（及可选子格）的步兵对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `sub_cell` (`number`, 可选) — 子单元格位置，`-1` 表示忽略子格。
- **返回** (`infantry`)：步兵对象；若未找到，返回无效对象。

#### `get_infantries()`
- **说明**：返回 **所有** 步兵的数组表。
- **返回** (`table<infantry>`)：步兵对象数组（索引从 1 开始）。

**重要注释**：
- 若需修改步兵的属性（如 `facing`、`health`），推荐使用以下模式：
  1. 用 `get_infantries()` 获得所有步兵对象表。
  2. 遍历该表，对每个对象修改属性。
  3. 调用 `remove_infantry(i-1)`（注意索引转换）将其移除。
  4. 调用对象的 `obj:place()` 重新放置。
  5. 最后可调用 `update_infantry()` 或 `redraw_window()` 刷新界面。
- 示例：将所有步兵设为随机方向。
```lua
function get_random_facing()
    local facings = {0, 32, 64, 96, 128, 160, 192, 224}
    return facings[math.random(1, #facings)]
end

local objs = get_infantries()
for i = #objs, 1, -1 do
    local o = objs[i]
    remove_infantry(i-1)          -- 0基索引删除
    o.facing = tostring(get_random_facing())
    o:place()
end
update_infantry()
redraw_window()
```
- **索引注意事项**：`get_infantries()` 返回的数组索引从 1 开始，而 `remove_infantry(index)` 和 `get_infantry(index)` 使用的是从 0 开始的内部索引。因此在遍历并删除时，需将数组索引 -1 传入删除函数。为避免删除后索引变动，建议倒序遍历。

---

## 六、车辆 (`unit`)

### 构造函数

#### `unit(house, type, x, y)`
- **说明**：构造一个车辆对象（不立即放置，需调用 `place()`）。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 车辆类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回** (`unit`)：车辆对象。

### 成员

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `house` | `string` | 读/写 | 所属方。可用所属方使用`available_houses()`获取 |
| `type` | `string` | 读/写 | 车辆类型 ID |
| `health` | `string` | 读/写 | 血量，取值0~256 |
| `x` | `number` | 读/写 | X 坐标 |
| `y` | `number` | 读/写 | Y 坐标 |
| `status` | `string` | 读/写 | 状态 |
| `facing` | `string` | 读/写 | 面向，取值0~256 |
| `tag` | `string` | 读/写 | 关联标签的 ID |
| `veterancy` | `string` | 读/写 | 经验等级：0=新兵，100=老兵，200=精英 |
| `group` | `string` | 读/写 | 分组 |
| `above_ground` | `string` | 读/写 | 是否在桥上 ("0" 或 "1") |
| `auto_no_recruit` | `string` | 读/写 | 重组A |
| `auto_yes_recruit` | `string` | 读/写 | 重组B |
| `follow` | `string` | 读/写 | 跟随的车辆 ID |

### 方法

#### `place()`
- **说明**：将当前车辆添加到地图（延迟刷新）。
- **返回**：无。

#### `remove()`
- **说明**：删除地图中属性完全相同的 **第一个** 车辆。
- **返回**：无。

### 全局函数

#### `place_unit(house, type, x, y)`
- **说明**：快速放置车辆，使用默认属性，立即计划刷新。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 车辆类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回**：无。

#### `remove_unit(index)`
- **说明**：通过 **0 基索引** 删除车辆。
- **参数**：`index` (`number`) — 0 基索引。
- **返回**：无。

#### `remove_unit(x, y)`
- **说明**：删除指定坐标的车辆。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

#### `get_unit(index)`
- **说明**：通过 **0 基索引** 获取车辆对象。
- **参数**：`index` (`number`) — 0 基索引。
- **返回** (`unit`)：车辆对象；若未找到，返回 `x,y` 为 `-1` 的无效对象。

#### `get_unit(x, y)`
- **说明**：获取指定坐标的车辆对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回** (`unit`)：车辆对象；若未找到，返回无效对象。

#### `get_units()`
- **说明**：返回所有车辆的数组表。
- **返回** (`table<unit>`)：车辆对象数组。

**重要注释**：车辆对象的修改方法与步兵类似，需要“删除 - 修改属性 - 重新放置”的三步走。批量修改时务必倒序遍历以避免索引错乱。示例：
```lua
local objs = get_units()
for i = #objs, 1, -1 do
    local u = objs[i]
    remove_unit(i-1)    -- 0基索引
    u.facing = tostring(math.random(0, 7) * 32)
    u:place()
end
update_unit()
redraw_window()
```

---

## 七、飞行器 (`aircraft`)

### 构造函数

#### `aircraft(house, type, x, y)`
- **说明**：构造一个飞行器对象（不立即放置，需调用 `place()`）。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 飞行器类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回** (`aircraft`)：飞行器对象。

### 成员

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `house` | `string` | 读/写 | 所属方。可用所属方使用`available_houses()`获取 |
| `type` | `string` | 读/写 | 飞行器类型 ID |
| `health` | `string` | 读/写 | 血量，取值0~256 |
| `x` | `number` | 读/写 | X 坐标 |
| `y` | `number` | 读/写 | Y 坐标 |
| `status` | `string` | 读/写 | 状态 |
| `facing` | `string` | 读/写 | 面向，取值0~256 |
| `tag` | `string` | 读/写 | 关联标签的 ID |
| `veterancy` | `string` | 读/写 | 经验等级：0=新兵，100=老兵，200=精英 |
| `group` | `string` | 读/写 | 分组 |
| `above_ground` | `string` | 读/写 | 是否在桥上 ("0" 或 "1") |
| `auto_no_recruit` | `string` | 读/写 | 重组A |
| `auto_yes_recruit` | `string` | 读/写 | 重组B |

### 方法

#### `place()`
- **说明**：将当前飞行器添加到地图（延迟刷新）。
- **返回**：无。

#### `remove()`
- **说明**：删除地图中属性完全相同的 **第一个** 飞行器。
- **返回**：无。

### 全局函数

#### `place_aircraft(house, type, x, y)`
- **说明**：快速放置飞行器，使用默认属性，立即计划刷新。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 飞行器类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回**：无。

#### `remove_aircraft(index)`
- **说明**：通过 **0 基索引** 删除飞行器。
- **参数**：`index` (`number`) — 0 基索引。
- **返回**：无。

#### `remove_aircraft(x, y)`
- **说明**：删除指定坐标的飞行器。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

#### `get_aircraft(index)`
- **说明**：通过 **0 基索引** 获取飞行器对象。
- **参数**：`index` (`number`) — 0 基索引。
- **返回** (`aircraft`)：飞行器对象；若未找到，返回无效对象。

#### `get_aircraft(x, y)`
- **说明**：获取指定坐标的飞行器对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回** (`aircraft`)：飞行器对象；若未找到，返回无效对象。

#### `get_aircrafts()`
- **说明**：返回所有飞行器的数组表。
- **返回** (`table<aircraft>`)：飞行器对象数组。

**注意**：飞行器对象的修改方式与步兵、车辆相同，需要使用“删除-修改-放置”的流程。

---

## 八、建筑与节点

### 建筑 （`building`）

#### `building(house, type, x, y)` （构造函数）
- **说明**：构造建筑对象（不立即放置，需调用 `place()`）。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 建筑类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回** (`building`)：建筑对象。

**成员**：

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `house` | `string` | 读/写 | 所属方。可用所属方使用`available_houses()`获取 |
| `type` | `string` | 读/写 | 建筑类型 ID |
| `health` | `string` | 读/写 | 血量，取值0~256 |
| `x` | `number` | 读/写 | X 坐标 |
| `y` | `number` | 读/写 | Y 坐标 |
| `facing` | `string` | 读/写 | 面向 |
| `tag` | `string` | 读/写 | 关联标签的 ID |
| `ai_sell` | `string` | 读/写 | AI 变卖 |
| `ai_rebuild` | `string` | 读/写 | AI 重建（实际无效） |
| `powered` | `string` | 读/写 | 耗能/工作状态 |
| `upgrades` | `string` | 读/写 | 组件数 |
| `spot_light` | `string` | 读/写 | 探照灯 |
| `upgrade1` | `string` | 读/写 | 组件 1 的类型 ID |
| `upgrade2` | `string` | 读/写 | 组件 2 的类型 ID |
| `upgrade3` | `string` | 读/写 | 组件 3 的类型 ID |
| `ai_repair` | `string` | 读/写 | AI 修复 |
| `nominal` | `string` | 读/写 | 显示名称（实际无效） |

**方法**：

#### `place()`
- **说明**：将建筑写入地图 INI（不会立即刷新）。
- **返回**：无。

#### `place_node([delete_building = false])`
- **说明**：为当前建筑创建基地节点。若 `delete_building=true`，则删除地图上属性完全匹配的建筑。
- **参数**：
  - `delete_building` (`boolean`, 可选) — 是否删除建筑，默认为 `false`。
- **返回**：无。

#### `remove()`
- **说明**：删除地图上属性完全匹配的第一座建筑。
- **返回**：无。

### 全局函数

#### `place_building(house, type, x, y, [ignore_overlap = false])`
- **说明**：快速放置建筑。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 建筑类型 ID。
  - `x, y` (`number`) — 坐标。
  - `ignore_overlap` (`boolean`, 可选) — 若为 `true` 可跳过重叠检查，默认为 `false`。
- **返回**：无。

#### `remove_building(index)`
- **说明**：通过 **0 基索引** 删除建筑。
- **参数**：`index` (`number`) — 0 基索引。
- **返回**：无。

#### `remove_building(x, y)`
- **说明**：删除指定坐标的建筑。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

#### `get_building(index)`
- **说明**：通过 **0 基索引** 获取建筑对象。
- **参数**：`index` (`number`) — 0 基索引。
- **返回** (`building`)：建筑对象；未找到时返回 `x,y` 为 `-1` 的无效对象。

#### `get_building(x, y)`
- **说明**：获取指定坐标的建筑对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回** (`building`)：建筑对象；未找到时返回无效对象。

#### `get_buildings()`
- **说明**：返回所有建筑的数组表。
- **返回** (`table<building>`)：建筑对象数组。

**重要注释**：修改建筑属性同样需要移除再重新放置。批量修改时的索引映射与单位一致。示例：随机化建筑血量。
```lua
local buildings = get_buildings()
for i = #buildings, 1, -1 do
    local b = buildings[i]
    remove_building(i-1)
    b.health = tostring(math.random(1, 256))
    b:place()
end
update_building()
redraw_window()
```

### 基地节点函数

#### `place_node(house, type, x, y, [index = -1])`
- **说明**：添加一个基地节点。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 节点类型/建筑类型。
  - `x, y` (`number`) — 坐标。
  - `index` (`number`, 可选) — 插入位置（0 基），`-1` 则追加到末尾。默认为 `-1`。
- **返回**：无。

#### `remove_node(x, y)`
- **说明**：删除指定坐标的基地节点。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

---

## 九、地形与单元格

### 单元格 （`cell`）

**获取方式**：`get_cell(x, y)` 返回。不能手动构造。

**成员** (属性)：

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `x` | `number` | 只读 | X 坐标 |
| `y` | `number` | 只读 | Y 坐标 |
| `unit` | `number` | 只读 | 车辆索引 (Units 节内) |
| `infantry_1` | `number` | 只读 | 第一个步兵索引 |
| `infantry_2` | `number` | 只读 | 第二个步兵索引 |
| `infantry_3` | `number` | 只读 | 第三个步兵索引 |
| `aircraft` | `number` | 只读 | 飞行器索引 |
| `building` | `number` | 只读 | 建筑索引 |
| `terrain` | `number` | 只读 | 地形对象索引 |
| `terrain_type` | `number` | 只读 | 地形对象类型索引（rules 中 TerrainTypes 小节） |
| `smudge` | `number` | 只读 | 污染索引 |
| `smudge_type` | `number` | 只读 | 污染类型索引（rules 中 SmudgeTypes 小节） |
| `waypoint` | `number` | 只读 | 路径点索引 |
| `node_building` | `number` | 只读 | 基地节点关联的建筑类型索引 |
| `node_id` | `number` | 只读 | 基地节点索引 |
| `node_house` | `string` | 只读 | 基地节点所属阵营 |
| `overlay` | `number` | 读/写 | 覆盖物索引（0~65535， 65535 代表空） |
| `overlay_data` | `number` | 读/写 | 覆盖物数据（0~255） |
| `tile` | `number` | 读/写 | 地形索引 |
| `subtile` | `number` | 读/写 | 子地形索引 |
| `height` | `number` | 读/写 | 高度 |
| `cell_tag` | `number` | 只读 | 单元格标记索引 |
| `tube` | `number` | 只读 | 隧道索引 |
| `tube_data` | `number` | 只读 | 隧道方向数据 |
| `hidden` | `boolean` | 读/写 | 是否被“隐藏单个地形块”标记 |
| `alt_image` | `number` | 只读 | 替换图像索引 |

**方法**：

#### `is_hidden()`
- **说明**：判断该单元格是否因任何原因（单个隐藏或同类地形组隐藏）被隐藏。
- **返回** (`boolean`)：是否隐藏。

#### `is_multi_selected()`
- **说明**：判断该单元格是否处于多选模式选中状态。
- **返回** (`boolean`)：是否多选。

#### `apply()`
- **说明**：将对 `overlay`、`overlay_data`、`tile`、`subtile`、`height`、`hidden` 等属性的修改 **立即写入地图**，并刷新该单元格的预览。**重要**：修改属性后必须调用此方法才能生效！
- **返回**：无。

**提示**
- 修改 `tile`、`overlay` 等属性后，**必须调用 `cell:apply()`**，否则修改不会写入地图 INI。若需要立即看到效果，之后可调用 `redraw_window()`。
- 对于 `infantry_1`、`waypoint`等索引，可以通过 `get_infantry()`、`get_waypoint()`等函数获取对应游戏对象。 

**常见模式**：
```lua
local cell = get_cell(X, Y)
cell.tile = 0   -- 某种地形索引
cell.subtile = 0
local unit = get_unit(cell.unit)
cell:apply()
```

### 地形块 （`tile`） 

**说明**：表示一个具体的地形块（如一块悬崖的某一部分）。不能手动构造。
**获取方式**：`get_tile_block(tile_index, sub_index)` 或 `get_whole_tile(tile_index)`。

**成员** (均为只读)：

| 属性 | 类型 | 说明 |
|------|------|------|
| `x` | `number` | 该地形块在地形中的相对 X 偏移 |
| `y` | `number` | 该地形块在地形中的相对 Y 偏移 |
| `valid` | `boolean` | 该地形块是否有效 |
| `tile_index` | `number` | 所属地形的索引 |
| `tile_sub_index` | `number` | 子地形索引 |
| `height` | `number` | 该地形块的原始高度 |
| `alt_count` | `number` | 替换图像的数量 |
| `tile_set` | `number` | 所属地形组的索引 |
| `ramp_type` | `number` | 斜坡类型 |
| `land_type` | `string` | 地表类型字符串（见附录 B） |

### 地形操作函数

#### `get_tile_block(tile_index, subtile_index)`
- **说明**：获取指定的地形块对象。
- **参数**：
  - `tile_index` (`number`) — 地形索引。
  - `subtile_index` (`number`) — 子地形索引。
- **返回** (`tile`)：地形块对象。

#### `get_whole_tile(tile_index)`
- **说明**：获取指定地形的所有有效子块。
- **参数**：
  - `tile_index` (`number`) — 地形索引。
- **返回** (`table<tile>`)：该地形所有有效子块组成的数组。

#### `place_whole_tile(x, y, tile_index)`
- **说明**：在指定坐标放置整个地形（自动选取最佳子块）。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `tile_index` (`number`) — 地形索引。
- **返回**：无。

#### `place_tile(x, y, tile_obj, [height = -1], [alt_type = -1])`
- **说明**：手动放置一个 `tile` 对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `tile_obj` (`tile`) — 地形块对象。
  - `height` (`number`, 可选) — 高度，`-1` 表示使用当前单元格高度加上块高度。默认为 `-1`。
  - `alt_type` (`number`, 可选) — 替换图像类型，`-1` 表示随机选择。默认为 `-1`。
- **返回**：无。

#### `set_height(x, y, height)`
- **说明**：单独设置单元格高度。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `height` (`number`) — 高度值。
- **返回**：无。

#### `hide_cell(x, y, [type = 1])`
- **说明**：隐藏/显示单个单元格。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `type` (`number`, 可选) — 操作类型：1=隐藏，2=显示，3=取反。默认为 1。
- **返回**：无。

#### `hide_tile_set(tileset_index, [type = 1])`
- **说明**：隐藏/显示整个地形组。
- **参数**：
  - `tileset_index` (`number`) — 地形组索引。
  - `type` (`number`, 可选) — 操作类型，同 `hide_cell`。
- **返回**：无。

#### `multi_select_cell(x, y, [type = 1])`
- **说明**：多选/取消多选单元格。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `type` (`number`, 可选) — 操作类型，同 `hide_cell`。
- **返回**：无。

#### `multi_select_tile_set(tileset_index, [type = 1])`
- **说明**：多选/取消多选整个地形组。
- **参数**：
  - `tileset_index` (`number`) — 地形组索引。
  - `type` (`number`, 可选) — 操作类型。
- **返回**：无。

#### `create_shore(x1, y1, x2, y2)`
- **说明**：在矩形范围内自动生成海岸线。
- **参数**：
  - `x1, y1, x2, y2` (`number`) — 矩形对角坐标。
- **返回**：无。

#### `smooth_lat(x1, y1, x2, y2)`
- **说明**：平滑矩形范围内的地块过渡（lat 优化）。
- **参数**：
  - `x1, y1, x2, y2` (`number`) — 矩形对角坐标。
- **返回**：无。

#### `create_slope(x1, y1, x2, y2)`
- **说明**：在矩形范围内生成斜坡。
- **参数**：
  - `x1, y1, x2, y2` (`number`) — 矩形对角坐标。
- **返回**：无。

**补充**：进行大量地形修改前，建议调用 `save_undo()` 以允许用户回退。完成修改后调用 `redraw_window()` 和 `update_minimap()` 刷新界面。

---

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
- **示例**：
```lua
local vehicles = get_values("VehicleTypes", "rules+map")
for _, vehicle in ipairs(vehicles) do
    -- 获取所有车辆 ID
    -- 对于注册表小节，应该通过value获取注册项
end
```

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

---

## 十一、FA2 扩展功能

### 刷新函数

这些函数会强制从 INI 重新加载数据到内存，并在脚本结束后应用。若要立即显示，请接着调用 `redraw_window()`。

- `update_infantry()`
- `update_unit()`
- `update_aircraft()`
- `update_building()`
- `update_terrain()`
- `update_waypoint()`
- `update_node()`
- `update_overlay()`
- `update_tube()`
- `update_smudge()`
- `update_tiles()`
- `update_trigger()`：刷新触发（会通知已打开的触发编辑器窗口）。

**使用提示**：
- 通常在批量修改单位/建筑等对象后，调用对应的 `update_xxx()` 函数确保内存数据与 INI 同步，然后调用 `redraw_window()` 立即刷新视图。
- 对于直接通过 `write_string` 修改的 INI 内容，也需要相应 `update_xxx()` 才能让界面看到变化。

### 界面更新

#### `redraw_window()`
- **说明**：立即重绘主地图窗口。
- **返回**：无。

#### `update_minimap([x, y])`
- **说明**：重绘小地图。不传参数则全图重绘；传入坐标则只重绘那一格。
- **参数**：
  - `x, y` (`number`, 可选) — 要重绘的单元格坐标。
- **返回**：无。

### 快照

#### `create_snapshot()`
- **说明**：保存当前地图的完整状态（覆盖物、地形、INI）为一个快照，并返回其版本号。控制台会打印恢复命令提示，如 `restore_snapshot(1)`。
- **返回** (`number`)：快照版本号。
- **用法**：在执行有风险的大规模修改前调用，以便后续恢复。
```lua
if message_box("是否创建快照？", "提示", 2) == 1 then
    create_snapshot()
end
```

#### `restore_snapshot(version)`
- **说明**：恢复指定版本的快照。会弹出确认对话框，需要用户进行确认。
- **参数**：`version` (`number`) — 快照版本号。
- **返回**：无。

#### `clear_snapshot()`
- **说明**：清除所有已保存的快照。
- **返回**：无。

### 撤销与重做

#### `save_undo()`
- **说明**：记录当前覆盖物与地形的撤销点。通常在进行大量地形修改前调用。
- **返回**：无。

#### `save_undo_objects()`
- **说明**：记录游戏对象（建筑、车辆、飞行器、步兵、地形对象、污染、基地节点、隧道、路径点、单元标记）、地图注释、测距工具、可视区域大小的撤销点。通常在进行大量游戏对象修改前调用。
- **返回**：无。

### 其他

#### `in_map(x, y)`
- **说明**：判断坐标是否在地图范围内。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回** (`boolean`)：是否在地图内。

#### `move_to(x, y)` / `move_to(waypoint_number)`
- **说明**：移动视野居中到指定坐标或数字路径点位置。若只传一个参数，则视为路径点数字。
- **参数**：
  - 若两个参数：`x, y` (`number`) — 目标坐标。
  - 若一个参数：`waypoint_number` (`number`) — 路径点数字。
- **返回**：无。

#### `running_lua_brush()`
- **说明**：返回当前脚本是否通过“脚本刷”模式执行。
- **返回** (`boolean`)：是否脚本刷模式。

### 脚本刷全局变量

仅在“脚本刷”运行时存在：

| 变量 | 类型 | 说明 |
|------|------|------|
| `X` | `number` | 鼠标点击的 X 坐标 |
| `Y` | `number` | 鼠标点击的 Y 坐标 |
| `first_run` | `boolean` | 是否是本次刷模式的首次点击 |
| `control_down` | `boolean` | 是否按下了 Ctrl 键 |
| `holding_click` | `boolean` | 是否由长按鼠标左键触发 |

**脚本刷示例**：
```lua
if running_lua_brush() then
    if not holding_click then
        local cell = get_cell(X, Y)
        -- 根据 first_run 等变量实现首次点击和后续点击的不同行为
    end
else
    message_box("本脚本仅能通过“脚本刷”运行！", "错误", 8)
end
```

---

## 十二、触发与标签

### `tag` 类

不能直接构造，作为 `trigger` 的成员出现。

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 标签 ID |
| `name` | `string` | 只读 | 标签名称 |
| `type` | `string` | 只读 | 重复类型 ("0"/"1"/"2") |

### `trigger` 类

#### `trigger([id])` （构造函数）
- **说明**：构造触发对象。不提供 `id` 则自动分配可用 ID。
- **参数**：`id` (`string`, 可选) — 触发 ID。
- **返回** (`trigger`)：触发实例。

**成员**：

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 触发 ID |
| `name` | `string` | 读/写 | 名称 |
| `country` | `string` | 读/写 | 国家，属于Countries注册表 |
| `tags` | `table<tag>` | 只读 | 关联的标签列表 |
| `attached_trigger` | `string` | 读/写 | 下级触发 ID |
| `disabled` | `boolean` | 读/写 | 是否禁用 |
| `easy` | `boolean` | 读/写 | 简单难度启用 |
| `medium` | `boolean` | 读/写 | 普通难度启用 |
| `hard` | `boolean` | 读/写 | 困难难度启用 |
| `events` | `table<string>` | 只读 | 事件列表，每个字符串为完整的逗号分隔表达式 |
| `actions` | `table<string>` | 只读 | 行为列表，每个字符串为完整的逗号分隔表达式 |

**方法**：

#### `add_tag(id, name, repeat)`
- **说明**：添加标签。`id`/`name` 若为空字符串，则自动生成；`repeat` 为 0、1 或 2。
- **参数**：
  - `id` (`string`) — 标签 ID，可为空。
  - `name` (`string`) — 标签名称，可为空。
  - `repeat` (`number` 或 `string`) — 重复类型：0、1 或 2。
- **返回**：无。

#### `add_event(event_str, [index = 0])`
- **说明**：添加事件。`index=0` 追加到末尾，`index>=1` 插入到指定位置。
- **参数**：
  - `event_str` (`string`) — 事件表达式字符串。
  - `index` (`number`, 可选) — 插入位置（1 基），0 表示末尾。默认为 0。
- **返回**：无。

#### `add_action(action_str, [index = 0])`
- **说明**：添加行为。
- **参数**：
  - `action_str` (`string`) — 行为表达式字符串。
  - `index` (`number`, 可选) — 插入位置，同 `add_event`。
- **返回**：无。

#### `replace_event(event_str, index)`
- **说明**：替换从 1 开始的第 `index` 个事件。
- **参数**：
  - `event_str` (`string`) — 新事件字符串。
  - `index` (`number`) — 位置（1 基）。
- **返回**：无。

#### `replace_action(action_str, index)`
- **说明**：替换第 `index` 个行为。
- **参数**：
  - `action_str` (`string`) — 新动作字符串。
  - `index` (`number`) — 位置（1 基）。
- **返回**：无。

#### `get_event_type(event_index, param_index)`
- **说明**：获取事件参数的类型字符串（见附录 D）。索引均从 1 开始。
- **参数**：
  - `event_index` (`number`) — 事件索引。
  - `param_index` (`number`) — 参数索引。
- **返回** (`string`)：参数类型字符串。

#### `get_action_type(action_index, param_index)`
- **说明**：获取行为参数的类型字符串。
- **参数**：
  - `action_index` (`number`) — 行为索引。
  - `param_index` (`number`) — 参数索引。
- **返回** (`string`)：参数类型字符串。

#### `delete_tag(index, remove_ini)`
- **说明**：删除第 `index` 个标签（1 基）。
- **参数**：
  - `index` (`number`) — 标签索引。
  - `remove_ini` (`boolean`) — 是否从 INI 中移除。
- **返回**：无。

#### `delete_tags(remove_ini)`
- **说明**：删除所有标签。
- **参数**：`remove_ini` (`boolean`) — 是否从 INI 中移除。
- **返回**：无。

#### `delete_event(index)`
- **说明**：删除事件（1 基）。
- **参数**：`index` (`number`) — 事件索引。
- **返回**：无。

#### `delete_action(index)`
- **说明**：删除行为（1 基）。
- **参数**：`index` (`number`) — 行为索引。
- **返回**：无。

#### `change_id(new_id)`
- **说明**：修改触发 ID。
- **参数**：`new_id` (`string`) — 新 ID。
- **返回**：无。

#### `release_id()`
- **说明**：丢弃当前占用的 ID，使其可以被新触发使用（已写入 INI 的仍不可用）。
- **返回**：无。

#### `apply()`
- **说明**：将所有修改写入 INI。除删除标签外，所有修改都要调用此方法。
- **返回**：无。

#### `delete(keep_tag)`
- **说明**：删除此触发。`keep_tag=true` 保留标签；`false` 同时删除所有关联标签。
- **参数**：`keep_tag` (`boolean`) — 是否保留标签。
- **返回**：无。

**静态函数**：

#### `delete_trigger(id, keep_tag)`
- **说明**：删除指定 ID 的触发。
- **参数**：
  - `id` (`string`) — 触发 ID。
  - `keep_tag` (`boolean`) — 是否保留标签。
- **返回**：无。

#### `delete_tag(id, keep_trigger)`
- **说明**：删除指定标签。
- **参数**：
  - `id` (`string`) — 标签 ID。
  - `keep_trigger` (`boolean`) — 是否保留关联触发。
- **返回**：无。

#### `get_trigger(id)`
- **说明**：获取指定 ID 的触发对象。
- **参数**：`id` (`string`) — 触发 ID。
- **返回** (`trigger` 或 `nil`)：触发对象，若不存在则为 `nil`。

#### `get_triggers()`
- **说明**：获取所有触发的 ID。
- **返回** (`table<int, string>`)：值数组。

#### `place_celltag(x, y, tag_id)`
- **说明**：放置单元标记。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `tag_id` (`string`) — 标签 ID。
- **返回**：无。

#### `remove_celltag(x, y)`
- **说明**：移除指定坐标的单元标记。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

#### `remove_celltags(tag_id)`
- **说明**：移除指定标签的所有单元标记。
- **参数**：`tag_id` (`string`) — 标签 ID。
- **返回**：无。

#### `int_to_float(value)`
- **说明**：将一个32位整数强制转换为浮点数。
- **参数**：`value` (`number`) — 输入整数。
- **返回**：(`number`): 输出浮点数。

#### `float_to_int(value)`
- **说明**：将一个浮点数强制转换为32位无符号整数。
- **参数**：`value` (`number`) — 输入浮点数。
- **返回**：(`number`): 输出整数。

**关键用法提示**：
- 任何对触发属性的修改，最后必须调用 `trigger:apply()`，否则不会保存到地图文件。
- 创建新触发时，通常先调用 `trigger:new()`，然后分别设置属性、添加事件和行为，最后 `apply()`。
- 触发一般都需要一个标签。使用`trigger:new()`创建的触发，如果没有特殊说明，一般需要同时创建一个标签，使用`add_tag("", "", 0)`创建一个默认名称的重复类型为0的标签。
- 批量修改触发时，遍历 `get_triggers()` 获取所有触发 ID，用 `get_trigger(id)` 获取对象，修改后逐一 `apply()`。
- 触发事件的表达式字符串中，第一位参数是事件类型，后续才是事件参数。例如在事件表达式`1,0,2`中，事件类型是1，事件参数分别为0和2。一个事件可能有2个或3个参数。
- 触发行为的表达式字符串中，第一位参数是行为类型，后续才是行为参数。例如在行为表达式`1,0,2,0,0,0,0,A`中，行为类型是1，事件参数是后续七个字符串。一个行为只能有7个参数。
- 由于事件参数数量以及事件和行为的每个参数具体如何填写较为复杂，不提供对应的接口。在新增事件、行为，或修改已有事件、行为的类型时，应当询问用户，确认正确的表达式。

**示例**：禁用所有名称包含 “debug” 的触发。
```lua
for i, id in pairs(get_triggers()) do
    local t = get_trigger(id)
    if string.find(t.name:lower(), "debug") then
        t.disabled = true
        t:apply()
    end
end
```

---

## 十三、AI 触发 (`ai_trigger`)

#### `ai_trigger([id])` （构造函数）
- **说明**：构造 AI 触发对象。不提供 `id` 则自动分配。
- **参数**：`id` (`string`, 可选) — AI 触发 ID。
- **返回** (`ai_trigger`)：AI 触发实例。

**成员**：

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | AI 触发 ID |
| `name` | `string` | 读/写 | 名称 |
| `team1` | `string` | 读/写 | 小队 1 ID |
| `team2` | `string` | 读/写 | 小队 2 ID |
| `country` | `string` | 读/写 | 国家，属于Countries注册表。<all>表示不限定国家 |
| `tech_level` | `string` | 读/写 | 科技等级 |
| `condition` | `string` | 读/写 | 比较条件 |
| `object` | `string` | 读/写 | 比较对象类型 |
| `comparator` | `string` | 读/写 | 比较方式（可填 `"<"` `"<="` `"=="` `">="` `">"` `"!="` 或数字） |
| `amount` | `number` | 读/写 | 比较数量 |
| `side` | `string` | 读/写 | 阵营 |
| `init_weight` | `number` | 读/写 | 初始权重 |
| `min_weight` | `number` | 读/写 | 最小权重 |
| `max_weight` | `number` | 读/写 | 最大权重 |
| `is_for_skirmish` | `boolean` | 读/写 | 遭遇战（无效） |
| `is_base_defense` | `boolean` | 读/写 | 基地防御（无效） |
| `easy` | `boolean` | 读/写 | 简单难度 |
| `medium` | `boolean` | 读/写 | 普通难度 |
| `hard` | `boolean` | 读/写 | 困难难度 |
| `enabled` | `boolean` | 读/写 | 整体启用状态 |

**方法**：

#### `apply()`
- **说明**：将修改写入 INI。
- **返回**：无。

#### `change_id(new_id)`
- **说明**：修改 ID。
- **参数**：`new_id` (`string`) — 新 ID。
- **返回**：无。

#### `release_id()`
- **说明**：释放当前 ID。
- **返回**：无。

#### `delete()`
- **说明**：删除此 AI 触发。
- **返回**：无。

**静态函数**：

#### `delete_ai_trigger(id)`
- **说明**：删除指定 ID 的 AI 触发。
- **参数**：`id` (`string`) — ID。
- **返回**：无。

#### `get_ai_trigger(id)`
- **说明**：获取 AI 触发对象。
- **参数**：`id` (`string`) — ID。
- **返回** (`ai_trigger` 或 `nil`)：对象或 `nil`。

#### `get_ai_triggers()`
- **说明**：获取所有AI触发的 ID。
- **返回** (`table<int, string>`)：值数组。

**使用提醒**：所有修改必须在 `apply()` 后生效。

---

## 十四、动作脚本 (`script`)

#### `script([id])` （构造函数）
- **说明**：构造脚本对象。不提供 `id` 则自动分配。
- **参数**：`id` (`string`, 可选) — 脚本 ID。
- **返回** (`script`)：脚本实例。

**成员**：

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 脚本 ID |
| `name` | `string` | 读/写 | 名称 |
| `actions` | `table<int>` | 只读 | 脚本行为数组（整数） |
| `params` | `table<int>` | 只读 | 对应行为的参数数组（整数） |

**方法**：

#### `add_action(action_number, param_number)`
- **说明**：添加一行脚本行为。
- **参数**：
  - `action_number` (`number`) — 行为编号。
  - `param_number` (`number`) — 参数编号。
- **返回**：无。

#### `delete_action(index)`
- **说明**：删除第 `index` 行（1 基）。
- **参数**：`index` (`number`) — 行索引。
- **返回**：无。

#### `replace_action(index, action_number, param_number)`
- **说明**：替换第 `index` 行行为（1 基）。
- **参数**：
	- `index` (`number`) — 行索引。
	-  `action_number` (`number`) — 行为编号。
	- `param_number` (`number`) — 参数编号。
- **返回**：无。

#### `apply()`
- **说明**：写入 INI。
- **返回**：无。

#### `change_id(new_id)`
- **说明**：修改 ID。
- **参数**：`new_id` (`string`) — 新 ID。
- **返回**：无。

#### `release_id()`
- **说明**：释放当前 ID。
- **返回**：无。

#### `delete()`
- **说明**：删除此脚本。
- **返回**：无。

**静态函数**：

#### `delete_script(id)`
- **说明**：删除指定脚本。
- **参数**：`id` (`string`) — 脚本 ID。
- **返回**：无。

#### `get_script(id)`
- **说明**：获取脚本对象。
- **参数**：`id` (`string`) — ID。
- **返回** (`script` 或 `nil`)：对象或 `nil`。

#### `get_scripts()`
- **说明**：获取所有脚本的 ID。
- **返回** (`table<int, string>`)：值数组。

#### `get_script_type(action_type, [extra = false])`
- **说明**：获取指定脚本行为的参数类型（`extra=true` 时获取额外参数类型）。详见附录 D。
- **参数**：
  - `action_type` (`number`) — 脚本行为类型。
  - `extra` (`boolean`, 可选) — 是否获取额外参数类型，默认为 `false`。
- **返回** (`string`)：类型字符串。

#### `script_has_extra(action_type)`
- **说明**：获取指定脚本行为的是否有额外参数。
- **参数**：`action_type` (`number`) — 脚本行为类型。
- **返回** (`boolean`)：是否有额外参数。

#### `get_script_extra(param, [get_extra = false])`
-   **说明**：获取行为参数的额外或基本部分。行为参数由低 16 位（基本参数）和高 16 位（额外参数）组合而成。
-   **参数**：
    -   `param`  (`number`) — 组合的行为参数值。
    -   `get_extra`  (`bool`, 可选) — 若为  `true`  则返回额外参数（高 16 位），否则返回基本参数（低 16 位）。默认为  `false`。
-   **返回**  (`number`)：基本参数或额外参数的值。

#### `combine_script_extra(param, extra_param)`
-   **说明**：将基本参数和额外参数组合成一个完整的行为参数（32 位整数）。
-   **参数**：
    -   `param`  (`number`) — 基本参数（将被置于低 16 位）。
    -   `extra_param`  (`number`) — 额外参数（将被置于高 16 位）。
-   **返回**  (`number`)：组合后的行为参数值。

**使用提醒**：所有修改必须在 `apply()` 后生效。

---

## 十五、特遣部队 (`task_force`)

#### `task_force([id])` （构造函数）
- **说明**：构造特遣部队对象。不提供 `id` 则自动分配。
- **参数**：`id` (`string`, 可选) — ID。
- **返回** (`task_force`)：实例。

**成员**：

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 特遣部队 ID |
| `name` | `string` | 读/写 | 名称 |
| `group` | `string` | 读/写 | 分组 |
| `numbers` | `table<int>` | 只读 | 每个成员的数量 |
| `units` | `table<string>` | 只读 | 每个成员的单位类型 |

**方法**：

#### `add_number(num, unit_type)`
- **说明**：添加成员，数量为 `num`，类型为 `unit_type`。最多 6 种单位。
- **参数**：
  - `num` (`number`) — 数量。
  - `unit_type` (`string`) — 单位类型 ID。
- **返回**：无。

#### `delete_number(index)`
- **说明**：删除第 `index` 个成员（1 基）。
- **参数**：`index` (`number`) — 成员索引。
- **返回**：无。

#### `replace_number(index, num, unit_type)`
- **说明**：替换第 `index` 个成员（1 基）， 数量为 `num`，类型为 `unit_type`。
- **参数**：
	- `index` (`number`) — 成员索引。
	- `num` (`number`) — 数量。
	- `unit_type` (`string`) — 单位类型 ID。
- **返回**：无。

#### `apply()`
- **说明**：写入 INI。
- **返回**：无。

#### `change_id(new_id)`
- **说明**：修改 ID。
- **参数**：`new_id` (`string`) — 新 ID。
- **返回**：无。

#### `release_id()`
- **说明**：释放当前 ID。
- **返回**：无。

#### `delete()`
- **说明**：删除此特遣部队。
- **返回**：无。

**静态函数**：

#### `delete_task_force(id)`
- **说明**：删除指定特遣部队。
- **参数**：`id` (`string`) — ID。
- **返回**：无。

#### `get_task_force(id)`
- **说明**：获取特遣部队对象。
- **参数**：`id` (`string`) — ID。
- **返回** (`task_force` 或 `nil`)：对象或 `nil`。

#### `get_task_forces()`
- **说明**：获取所有特遣部队的 ID。
- **返回** (`table<int, string>`)：值数组。

**使用提醒**：所有修改必须在 `apply()` 后生效。

---

## 十六、作战小队 (`team`)

#### `team([id])` （构造函数）
- **说明**：构造作战小队对象。不提供 `id` 则自动分配。
- **参数**：`id` (`string`, 可选) — ID。
- **返回** (`team`)：实例。

**成员**：

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 小队 ID |
| `name` | `string` | 读/写 | 名称 |
| `country` | `string` | 读/写 | 国家，属于Countries注册表。<all>表示不限定国家 |
| `task_force` | `string` | 读/写 | 关联的特遣部队 ID |
| `script` | `string` | 读/写 | 关联的脚本 ID |
| `tag` | `string` | 读/写 | 关联的标签 ID |
| `veteran_level` | `string` | 读/写 | 经验等级 |
| `priority` | `string` | 读/写 | 优先级 |
| `max` | `string` | 读/写 | 最大数量 |
| `techlevel` | `string` | 读/写 | 科技等级 |
| `transport_waypoint` | `string` | 读/写 | 运输机起始路径点（字母或数字均可） |
| `group` | `string` | 读/写 | 分组 |
| `waypoint` | `string` | 读/写 | 出现路径点（字母或数字均可） |
| `mind_control_decision` | `string` | 读/写 | 心控结果 |
| `full` | `boolean` | 读/写 | 预装载 |
| `whiner` | `boolean` | 读/写 | 平民哀叫 |
| `droppod` | `boolean` | 读/写 | 运输机空投 |
| `suicide` | `boolean` | 读/写 | 忽视阻拦 |
| `loadable` | `boolean` | 读/写 | 可装载 |
| `prebuild` | `boolean` | 读/写 | 预建造 |
| `annoyance` | `boolean` | 读/写 | 骚扰效果 |
| `ion_immune` | `boolean` | 读/写 | 无视离子风暴 |
| `recruiter` | `boolean` | 读/写 | 忽视分组 |
| `reinforce` | `boolean` | 读/写 | 增援部队 |
| `aggressive` | `boolean` | 读/写 | 侵略部队 |
| `autocreate` | `boolean` | 读/写 | 自动建造 |
| `guard_slower` | `boolean` | 读/写 | 保护缓慢单位 |
| `on_trans_only` | `boolean` | 读/写 | 只为传送 |
| `avoid_threats` | `boolean` | 读/写 | 躲避威胁 |
| `loose_recruit` | `boolean` | 读/写 | 用后即弃 |
| `is_base_defense` | `boolean` | 读/写 | 用语基地防御 |
| `use_transport_origin` | `boolean` | 读/写 | 使用运输机起始路径点 |
| `only_target_house_enemy` | `boolean` | 读/写 | 仅攻击敌对方 |
| `transports_return_on_unload` | `boolean` | 读/写 | 运载器下客后返回 |
| `are_team_members_recruitable` | `boolean` | 读/写 | 小队成员可重组 |

**方法**：

#### `apply()`
- **说明**：写入 INI。
- **返回**：无。

#### `change_id(new_id)`
- **说明**：修改 ID。
- **参数**：`new_id` (`string`) — 新 ID。
- **返回**：无。

#### `release_id()`
- **说明**：释放当前 ID。
- **返回**：无。

#### `delete()`
- **说明**：删除此小队。
- **返回**：无。

**静态函数**：

#### `delete_team(id)`
- **说明**：删除指定作战小队。
- **参数**：`id` (`string`) — ID。
- **返回**：无。

#### `get_team(id)`
- **说明**：获取作战小队对象。
- **参数**：`id` (`string`) — ID。
- **返回** (`team` 或 `nil`)：对象或 `nil`。

#### `get_teams()`
- **说明**：获取所有作战小队的 ID。
- **返回** (`table<int, string>`)：值数组。

**重要**：与触发类似，所有对 `team`、`task_force`、`script` 的修改都需要在最后调用各自的 `apply()` 方法。创建新的小队时，通常先创建关联的特遣部队和脚本（或引用已有），再创建小队并设定关联 ID。

**典型创建流程**：
```lua
local t = team:new()
local s = script:new()
local task = task_force:new()

t.name = "我的小队"
s.name = "我的脚本"
task.name = "我的特遣"
t.house = "Americans"
t.task_force = task.id
t.script = s.id

task:add_number(3, "E1")
task:add_number(2, "HTNK")
s:add_action(11, 11)  -- 行为11，参数11

t:apply()
s:apply()
task:apply()
```
---


## 十七、变量 
#### `get_variable_value(index, [is_global = false])`
- **说明**：返回变量的默认值。
- **参数**：
  - `index` (`int`) — 从 0 开始的变量索引。
  - `is_global` (`bool`, 可选) — 若为 `true` 则返回对应的全局变量，否则返回局部变量。默认为 `false`。
- **返回** (`int`)：变量的默认值；若变量不存在则返回 `-1`。

#### `get_variable_name(index, [is_global = false])`
- **说明**：返回变量的名称。
- **参数**：
  - `index` (`int`) — 从 0 开始的变量索引。
  - `is_global` (`bool`, 可选) — 若为 `true` 则返回对应的全局变量，否则返回局部变量。默认为 `false`。
- **返回** (`string`)：变量的名称；若变量不存在则返回 `"MISSING"`。

#### `set_variable_value(index, value)`
- **说明**：设置局部变量的默认值。
- **参数**：
  - `index` (`int`) — 从 0 开始的变量索引。
  - `value` (`int`) — 新的默认值。
- **返回**：无。

#### `set_variable_name(index, name)`
- **说明**：设置局部变量的名称。
- **参数**：
  - `index` (`int`) — 从 0 开始的变量索引。
  - `name` (`string`) — 新的名称。
- **返回**：无。

#### `add_variable(name, value)`
- **说明**：新增一个局部变量。
- **参数**：
  - `name` (`string`) — 变量名。
  - `value` (`int`) — 默认值。
- **返回** (`int`)：从 0 开始的新变量的索引。

## 附录

### A. `message_box` 格式码

| 数值 | 样式 |
|------|------|
| 0 | 确认 |
| 1 | 确认 / 取消 |
| 2 | 是 / 否 |
| 3 | 是 / 否 / 取消 |
| 4 | 重试 / 取消 |
| 5 | 终止 / 重试 / 忽略 |
| 6 | 取消 / 重试 / 继续 |
| 7 | 带信息图标的确认 |
| 8 | 带警告图标的确认 |
| 9 | 带错误图标的确认 |
| 10 | 带询问图标的是 / 否 |
| 11 | 带信息图标的确认 / 取消 |
| 12 | 带警告图标的确认 / 取消 |
| 13 | 带错误图标的确认 / 取消 |
| 14 | 带询问图标的确认 / 取消 |
| 15 | 带错误图标的重试 / 取消 |
| 16 | 带错误图标的终止 / 重试 / 忽略 |

### B. `tile.land_type` 返回值

| 地表原始数值 | 返回字符串 |
|-------------|----------|
| 0x0, 0xD | `"clear"` |
| 0x1, 0x2, 0x3, 0x4 | `"ice"` |
| 0x5 | `"tunnel"` |
| 0x6 | `"railroad"` |
| 0x7, 0x8, 0xF | `"rock"` |
| 0x9 | `"water"` |
| 0xA | `"beach"` |
| 0xB, 0xC | `"road"` |
| 0xE | `"rough"` |

### C. `load_from` 参数取值

| 值 | 读取文件 |
|----|----------|
| `"map"` | 当前地图文件 |
| `"fadata"` | FAData.ini（地编目录下） |
| `"rules"` | rulesmd.ini |
| `"rules+map"` | rulesmd.ini + 地图文件 |
| `"art"` | artmd.ini |
| `"sound"` | soundmd.ini |
| `"theme"` | thememd.ini |
| `"ai"` | aimd.ini |
| `"ai+map"` | aimd.ini + 地图文件 |
| `"eva"` | evamd.ini |
| `"theater"` | 当前地形文件 |
| `"xxx.ini"` | 游戏目录下相对路径的 INI 文件（支持 mix 内读取） |

### D. 参数类型返回值 (`get_event_type` / `get_action_type` / `get_script_type`)

| 返回字符串 | 含义 |
|-----------|------|
| `"WAYPOINT_STR"` | 字母格式路径点 |
| `"WAYPOINT_NUM"` | 数字格式路径点 |
| `"COUNTRY"` | 国家索引 |
| `"TECHNO"` | 科技类型 ID |
| `"TRIGGER"` | 触发 ID |
| `"CSF"` | CSF 标签 |
| `"TAG"` | 标签 ID |
| `"FLOAT"` | 浮点数 |
| `"TEAM"` | 小队 ID |
| `"VARIABLE_GLOBAL"` | 全局变量索引 |
| `"VARIABLE_LOCAL"` | 局部变量索引 |
| 其他数字字符串 | FAData.ini 中 `[NewParamTypes]` 的键名 |