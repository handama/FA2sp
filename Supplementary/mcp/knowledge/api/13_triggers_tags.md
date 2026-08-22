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
- trigger类主要适用于对大批量触发进行简单修改，若修改程度较为复杂，或涉及新建事件、行为等操作，建议使用`触发编辑器`。
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

### 触发编辑器（无头，`te_` 系列）

- **说明**：本组函数操作一个隐藏的“触发编辑器”实例，复用编辑器全部编辑逻辑（事件/行为类型加载、参数选项填充、参数联动、名称与描述生成）。可以返回可用事件、行为，选中事件、行为的可用参数等信息，而无需处理原始ini文本、参数位置等底层数据。它与数据层 `trigger` 类互为补充：`trigger` 类适合已知裸值时的批量读写；`te_` 系列适合在编辑器规则下探索（类型含义、参数可填内容）与校验、再写入（自动套用名称/描述/编码规则）。
- **与 `trigger` 类混用时的数据新鲜度**：
  - `te_` 编辑后 INI 立即变化，旧的 `trigger` 对象为快照已失效，需重新 `get_trigger(id)`。
  - `trigger:apply()` 写 INI 后，编辑器内部缓存过期：每次 `te_select_trigger` 都会重读 INI，天然安全；但已在编辑中（已 select 未重新 select）的触发需主动重新 `te_select_trigger`。
- **索引约定**：事件序号、行为序号、参数槽位均从 **1** 开始；`te_get_*_options` 的返回表从 1 开始；`te_get_selected_trigger` 无选中触发时返回 `nil`。
- **参数设置两种模式**：
  - `te_set_event_param` / `te_set_action_param`（直接设置）：原样写入参数值，不校验选项列表，用于已知裸值的场景。
  - `te_set_event_param_fuzzy` / `te_set_action_param_fuzzy`（模糊设置）：在选项列表中按“裸值精确 → 显示文本精确 → 显示文本包含（不区分大小写）”三级匹配，失败时向输出窗口报告错误并**不做任何修改**。推荐用于按名称/描述查找参数。

#### `te_new_trigger([name = ""], [house = ""])`
- **说明**：新建一个触发并自动选中它。分配新 ID 时会在内部与 Lua `trigger` 类保留的 ID 同步，避免撞号。新建的触发不自动创建默认事件/行为，需要自行用 `te_add_event`/`te_add_action` 添加。
- **返回** (`table` 或 `nil`)：`{ id = "<触发ID>" }`。

#### `te_select_trigger(id)`
- **说明**：按 ID 选中一个已有触发。
- **参数**：`id` (`string`) — 触发 ID。
- **返回** (`boolean`)：是否选中成功。

#### `te_get_selected_trigger()`
- **说明**：获取当前选中触发的 ID。
- **返回** (`string` 或 `nil`)：当前选中触发 ID；无选中时为 `nil`。

#### `te_get_trigger()`
- **说明**：获取当前选中触发的完整信息，包括全部事件与行为及其参数槽位。
- **返回** (`table` 或 `nil`)：结构如下：
  - `id`, `name`, `house`, `attached_trigger` (`string`)
  - `disabled`, `easy`, `medium`, `hard` (`boolean`)
  - `repeat_type` (`string`)
  - `events` / `actions` (`table<int, table>`)：每项为事件/行为信息，取 `nil` 表示该项无效，有效时含 `ok=true`, `num`, `name`, `desc`, `params`（数组，每项含 `slot`, `used`, `desc`, `type`, `value`, `display`）。

#### `te_set_trigger_prop(key, value)`
- **说明**：设置当前触发的一个属性。
- **参数**：`key` (`string`) — 取值 `name`/`house`/`attached_trigger`/`disabled`/`easy`/`medium`/`hard`/`repeat_type`；`value` (`string`) — 新值（布尔类属性接受 `"1"`/`"true"`/`"yes"`）。
- **返回** (`boolean`)：是否成功。

#### `te_delete_trigger([keep_tags = false])`
- **说明**：删除当前选中触发（自动跳过删除确认框）。
- **参数**：`keep_tags` (`boolean`) — 是否保留关联标签。
- **返回** (`boolean`)：是否成功。

#### `te_get_event_types([filter = ""], [max = 50])`
#### `te_get_action_types([filter = ""], [max = 50])`
- **说明**：查询事件/行为类型元数据，不依赖选中触发，可独立使用。`filter` 会在名称与描述中做不区分大小写的包含匹配。
- **返回** (`table<int, table>`)：每项含 `num`, `name`, `desc`。

#### `te_get_event_type(num)` / `te_get_action_type(num)`
- **说明**：精确查询指定编号 `num` 的事件/行为类型的详细说明（对应编辑器里的描述框内容）。不依赖选中触发。若该类型不存在返回 `nil`。
- **返回** (`table` 或 `nil`)：含 `num`, `name`, `desc`。

#### `te_select_event(idx)` / `te_select_action(idx)`
- **说明**：选中第 `idx`（从 1 开始）个事件/行为并返回其信息。
- **返回** (`table` 或 `nil`)：含 `ok`, `num`, `name`, `desc`, `params`。

#### `te_get_event_options(slot, [filter = ""], [max = 50])` / `te_get_action_options(slot, [filter = ""], [max = 50])`
- **说明**：获取当前选中事件/行为第 `slot`（从 1 开始）个参数槽的可用选项（由该参数的类型/联动规则决定）。
- **返回** (`table<int, table>`)：每项含 `value`（裸值）、`text`（显示文本）。

#### `te_set_event_type(num)` / `te_set_action_type(num)`
- **说明**：将当前选中事件/行为的类型改为 `num`，并按新类型重载参数槽与描述。
- **返回** (`table` 或 `nil`)：新的条目信息。

#### `te_set_event_param(slot, value)` / `te_set_action_param(slot, value)`
- **说明**：直接写入当前选中事件/行为第 `slot` 个参数槽的裸值，并触发参数联动（可能影响其它参数槽的选项）。
- **返回** (`table` 或 `nil`)：含 `value`, `display`。

#### `te_set_event_param_fuzzy(slot, text)` / `te_set_action_param_fuzzy(slot, text)`
- **说明**：按文本模糊匹配设置当前选中事件/行为第 `slot` 个参数槽的选项（见上文匹配规则）。
- **返回** (`table` 或 `nil`)：含匹配到的 `value`, `display`。

#### `te_add_event()` / `te_add_action()`
- **说明**：在当前选中触发末尾新增一个事件/行为。
- **返回** (`boolean`)：是否成功。

#### `te_clone_event(idx)` / `te_clone_action(idx)`
- **说明**：克隆第 `idx`（从 1 开始）个事件/行为。
- **返回** (`boolean`)：是否成功。

#### `te_delete_event(idx)` / `te_delete_action(idx)`
- **说明**：删除第 `idx`（从 1 开始）个事件/行为（自动跳过删除确认框）。
- **返回** (`boolean`)：是否成功。

**示例**：查询所有含“生产”的事件类型。
```lua
for i, t in ipairs(te_get_event_types("生产")) do
    print(t.num, t.name)
end
```

**示例**：按名称模糊设置某触发第一个行为的第一个参数。
```lua
te_select_trigger("00000000-TR")
te_select_action(1)
local r = te_set_action_param_fuzzy(1, "美国")
if r == nil then print("未找到匹配选项") end
```