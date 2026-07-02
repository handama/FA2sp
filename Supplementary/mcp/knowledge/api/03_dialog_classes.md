# 对话框类

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