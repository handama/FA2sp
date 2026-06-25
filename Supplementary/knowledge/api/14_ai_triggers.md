# 十三、AI 触发 (`AITrigger`)

## `AITrigger` 类

### `AITrigger:new(id)` （构造函数）
- **说明**：构造 AI 触发对象。
- **参数**：`id` (`string`) — AI 触发 ID（可用 `get_free_id(6)` 获取）。
- **返回** (`AITrigger`)：AI 触发对象。

**成员**：

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | AI 触发 ID |
| `enabled` | `boolean` | 读/写 | 是否启用（第 2 个参数） |
| `side` | `string` | 读/写 | 阵营（第 1 个参数） |
| `condition_type` | `number` | 读/写 | 条件类型（第 0 个参数） |
| `condition_param` | `string` | 读/写 | 条件参数（第 4 个参数） |
| `condition_house` | `string` | 读/写 | 条件阵营（第 3 个参数） |
| `condition_count_comparison` | `number` | 读/写 | 比较类型（第 5 个参数） |
| `condition_count` | `number` | 读/写 | 数值（第 6 个参数） |
| `action_team` | `string` | 读/写 | 小队 ID |
| `action_team_script` | `string` | 读/写 | 小队用动作脚本 ID |
| `action_type` | `number` | 读/写 | 动作类型：1=扔小队，2=改全局，4=无动作 |
| `action_waypoint` | `string` | 读/写 | 路径点 ID |
| `action_value` | `number` | 读/写 | 未知/动作值 |
| `action_param` | `string` | 读/写 | 参数 |
| `minimum_rank` | `number` | 读/写 | 最小 AI 等级（第 10 个参数） |
| `maximum_rank` | `number` | 读/写 | 最大 AI 等级（第 11 个参数） |
| `minimum_players` | `number` | 读/写 | 最小玩家数（第 12 个参数） |
| `maximum_players` | `number` | 读/写 | 最大玩家数（第 13 个参数） |
| `tech_level` | `number` | 读/写 | 科技等级 |
| `unknown` | `string` | 读/写 | 未知参数 |

**方法**：

### `apply()`
- **说明**：将 AI 触发写入地图 INI 并刷新。
- **返回**：无。

### `delete()`
- **说明**：从地图 INI 中删除该 AI 触发。
- **返回**：无。

## 全局函数

### `get_ai_trigger(id)`
- **说明**：通过 ID 获取 AI 触发对象。未找到时返回 `id` 为空的无效对象。
- **参数**：`id` (`string`) — AI 触发 ID。
- **返回** (`AITrigger`)：AI 触发对象。

### `get_ai_triggers()`
- **说明**：获取所有 AI 触发的 ID 数组。
- **返回** (`table<string>`)：ID 数组。