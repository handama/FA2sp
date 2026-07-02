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