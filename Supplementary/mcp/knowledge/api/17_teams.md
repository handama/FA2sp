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