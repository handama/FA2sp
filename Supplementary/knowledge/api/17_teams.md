# 十六、作战小队 (`Team`)

## `Team` 类

### `Team:new(id)` （构造函数）
- **说明**：构造作战小队对象。
- **参数**：`id` (`string`) — 小队 ID（可用 `get_free_id(3)` 获取）。
- **返回** (`Team`)：小队对象。

**成员**：

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 小队 ID |
| `name` | `string` | 读/写 | 小队名称（第 0 个参数） |
| `veteran_level` | `number` | 读/写 | 老兵等级（第 1 个参数） |
| `waypoint` | `number` | 读/写 | 路径点（第 2 个参数） |
| `group` | `number` | 读/写 | 小组（第 3 个参数） |
| `waypoint_2` | `number` | 读/写 | 第二个路径点（第 4 个参数） |
| `script` | `string` | 读/写 | 动作脚本 ID（第 5 个参数） |
| `task_force` | `string` | 读/写 | 特遣部队 ID（第 6 个参数） |
| `waypoint_3` | `number` | 读/写 | 第三个路径点（第 7 个参数） |
| `guard_range` | `number` | 读/写 | 警戒范围（第 8 个参数） |
| `autocreate` | `boolean` | 读/写 | 是否自动建立（第 9 个参数） |
| `prebuild` | `boolean` | 读/写 | 是否预先建造（第 10 个参数） |
| `reinforce` | `boolean` | 读/写 | 是否可增援（第 11 个参数） |
| `whiner` | `boolean` | 读/写 | Whiner（第 12 个参数） |
| `lose_immune` | `boolean` | 读/写 | 失败免疫（第 13 个参数） |
| `mercenary` | `boolean` | 读/写 | 雇佣兵（第 14 个参数） |
| `priority` | `number` | 读/写 | 优先级（第 15 个参数） |
| `max` | `number` | 读/写 | 最大数量（第 16 个参数） |
| `tech_level` | `number` | 读/写 | 科技等级（第 17 个参数） |
| `facing` | `number` | 读/写 | 面向（第 18 个参数） |
| `unknown` | `string` | 读/写 | 未知参数 |
| `house` | `string` | 读/写 | 所属方（第 19 个参数） |
| `recruiter` | `boolean` | 读/写 | Recruiter（第 20 个参数） |
| `aggressive` | `boolean` | 读/写 | 是否主动攻击（第 21 个参数） |
| `suicide` | `boolean` | 读/写 | 是否自杀式攻击（第 22 个参数） |
| `loadable` | `boolean` | 读/写 | 是否可装载（第 23 个参数） |
| `full` | `boolean` | 读/写 | 是否满载（第 24 个参数） |
| `annoyance` | `boolean` | 读/写 | 烦扰模式（第 25 个参数） |
| `guard_slower` | `boolean` | 读/写 | 警戒减速（第 26 个参数） |
| `avoid_units` | `boolean` | 读/写 | 是否避开单位（第 27 个参数） |
| `ion_sensitive` | `boolean` | 读/写 | 离子炮敏感（第 28 个参数） |
| `transports_return` | `boolean` | 读/写 | 运输返回（第 29 个参数） |
| `are_team_members` | `boolean` | 读/写 | AreTeamMembers（第 30 个参数） |
| `is_team_type` | `boolean` | 读/写 | IsTeamType（第 31 个参数） |
| `unit_escort` | `boolean` | 读/写 | 单位护卫（第 32 个参数） |
| `distance` | `number` | 读/写 | 距离（第 33 个参数） |
| `transport_waypoint` | `number` | 读/写 | 运输路径点（第 34 个参数） |
| `attack_what` | `number` | 读/写 | 攻击目标类型（第 35 个参数） |
| `spawn_count` | `number` | 读/写 | 生成量（第 37 个参数） |
| `spawn_regen` | `number` | 读/写 | 生成恢复（第 38 个参数） |
| `spawn_regen_limit` | `number` | 读/写 | 生成恢复上限（第 39 个参数） |
| `spawn_initial` | `number` | 读/写 | 初始生成量（第 40 个参数） |

**方法**：

### `apply()`
- **说明**：将小队写入地图 INI 并刷新。
- **返回**：无。

### `delete()`
- **说明**：从地图 INI 中删除该小队。
- **返回**：无。

## 全局函数

### `get_team(id)`
- **说明**：通过 ID 获取小队对象。未找到时返回 `name` 为空的无效对象。
- **参数**：`id` (`string`) — 小队 ID。
- **返回** (`Team`)：小队对象。

### `get_teams()`
- **说明**：获取所有小队的 ID 数组。
- **返回** (`table<string>`)：ID 数组。