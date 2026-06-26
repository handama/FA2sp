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