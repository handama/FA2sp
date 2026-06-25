# 十二、触发与标签

## `trigger` 类

### `trigger:new(id)` （构造函数）
- **说明**：构造触发对象。不会自动分配 ID。
- **参数**：`id` (`string`) — 触发 ID（可用 `get_free_id(1)` 获取）。
- **返回** (`trigger`)：触发对象。

**成员**：

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 触发 ID |
| `name` | `string` | 读/写 | 触发名称（第 3 个参数） |
| `owner_house` | `string` | 读/写 | 所属方（第 0 个参数，逗号分隔列表） |
| `repeatable` | `boolean` | 读/写 | 是否可重复（第 2 个参数） |
| `disabled` | `boolean` | 读/写 | 是否禁用（第 1 个参数） |
| `a` | `number` | 读/写 | 触发条件 A 类型 |
| `a_group` | `number` | 读/写 | A 的分组（第 11 个参数） |
| `a_house` | `string` | 读/写 | A 的所属方（第 7 个参数） |
| `a_param1` | `string` | 读/写 | A 参数 1（第 8 个参数） |
| `a_param2` | `string` | 读/写 | A 参数 2（第 9 个参数） |
| `b` | `number` | 读/写 | 触发条件 B 类型 |
| `b_group` | `number` | 读/写 | B 的分组（第 12 个参数） |
| `b_house` | `string` | 读/写 | B 的所属方 |
| `b_param1` | `string` | 读/写 | B 参数 1 |
| `b_param2` | `string` | 读/写 | B 参数 2 |
| `actions` | `TriggerActionCollection` | 读/写 | 动作列表 |
| `associated_shape` | `string` | 读/写 | 关联区域形状参数（第 4 个参数） |
| `area_a` | `number` | 读/写 | 区域 A（第 5 个参数） |
| `area_b` | `number` | 读/写 | 区域 B（第 6 个参数） |
| `percents` | `string` | 读/写 | 参数百分比状态 |
| `unknown` | `string` | 读/写 | 未知/保留参数 |

**方法**：

### `apply()`
- **说明**：将触发写入地图 INI 并刷新。
- **返回**：无。

### `delete()`
- **说明**：从地图 INI 中删除该触发。
- **返回**：无。

## 全局函数

### `get_trigger(id)`
- **说明**：通过 ID 获取触发对象。未找到时返回 `name` 为空的无效对象。
- **参数**：`id` (`string`) — 触发 ID。
- **返回** (`trigger`)：触发对象。

### `get_triggers()`
- **说明**：获取所有触发的 ID 数组。
- **返回** (`table<string>`)：ID 数组。

### `get_trigger_keys([load_from = "map"])`
- **说明**：获取触发节的所有键名（即 ID 列表）。
- **参数**：`load_from` (`string`, 可选) — 读取源。
- **返回** (`table<string>`)：ID 数组。

## `tag` 类

### `tag:new(id)` （构造函数）
- **说明**：构造标签对象。
- **参数**：`id` (`string`) — 标签 ID（可用 `get_free_id(2)` 获取）。
- **返回** (`tag`)：标签对象。

**成员**：

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 标签 ID |
| `associated_trigger` | `string` | 读/写 | 关联的触发 ID（第 1 个参数） |
| `location_name` | `string` | 读/写 | 位置标签文字（第 3 个参数） |
| `is_waypoint` | `boolean` | 读/写 | 是否为路径点模式（第 2 个参数） |
| `x` | `number` | 读/写 | X 坐标（第 4 个参数） |
| `y` | `number` | 读/写 | Y 坐标（第 5 个参数） |
| `name` | `string` | 读/写 | 标签名（第 6 个参数） |

**方法**：

### `apply()`
- **说明**：将标签写入地图 INI 并刷新。
- **返回**：无。

### `delete()`
- **说明**：从地图 INI 中删除该标签。
- **返回**：无。

## 全局函数

### `get_tag(id)`
- **说明**：通过 ID 获取标签对象。未找到时返回 `id` 为空的无效对象。
- **参数**：`id` (`string`) — 标签 ID。
- **返回** (`tag`)：标签对象。

### `get_tags()`
- **说明**：获取所有标签的 ID 数组。
- **返回** (`table<string>`)：ID 数组。

### `get_tag_keys([load_from = "map"])`
- **说明**：获取标签节的所有键名（ID 列表）。
- **参数**：`load_from` (`string`, 可选) — 读取源。
- **返回** (`table<string>`)：ID 数组。