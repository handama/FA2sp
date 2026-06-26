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