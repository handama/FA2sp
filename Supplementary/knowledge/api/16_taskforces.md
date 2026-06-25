# 十五、特遣部队 (`TaskForce`)

## `TaskForce` 类

### `TaskForce:new(id)` （构造函数）
- **说明**：构造特遣部队对象。
- **参数**：`id` (`string`) — 特遣部队 ID（可用 `get_free_id(5)` 获取）。
- **返回** (`TaskForce`)：特遣部队对象。

**成员**：

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 特遣部队 ID |
| `name` | `string` | 读/写 | 特遣部队名称（第 0 个参数） |
| `group` | `number` | 读/写 | 分组（第 1 个参数） |
| `unknown` | `number` | 读/写 | 未知（第 2 个参数） |
| `units` | `TaskForceUnitCollection` | 读/写 | 单位列表（第 3 个参数起） |

**方法**：

### `apply()`
- **说明**：将特遣部队写入地图 INI 并刷新。
- **返回**：无。

### `delete()`
- **说明**：从地图 INI 中删除该特遣部队。
- **返回**：无。

## 全局函数

### `get_task_force(id)`
- **说明**：通过 ID 获取特遣部队对象。未找到时返回 `name` 为空的无效对象。
- **参数**：`id` (`string`) — 特遣部队 ID。
- **返回** (`TaskForce`)：特遣部队对象。

### `get_task_forces()`
- **说明**：获取所有特遣部队的 ID 数组。
- **返回** (`table<string>`)：ID 数组。