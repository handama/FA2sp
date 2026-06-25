# 五、步兵 (`infantry`)

## 构造函数

### `infantry(house, type, x, y)`
- **说明**：构造一个步兵对象（不立即放置到地图，需调用 `place()` 方法）。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 步兵类型 ID。
  - `x` (`number`) — X 坐标。
  - `y` (`number`) — Y 坐标。
- **返回** (`infantry`)：步兵对象。

## 成员

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `house` | `string` | 读/写 | 所属方 |
| `type` | `string` | 读/写 | 步兵类型 ID |
| `health` | `string` | 读/写 | 生命值（字符串形式，如 "100"），取值0~256 |
| `x` | `number` | 读/写 | X 坐标 |
| `y` | `number` | 读/写 | Y 坐标 |
| `subcell` | `string` | 读/写 | 子单元格位置：0~4；0、1为中心，2为右侧，3为左侧，4为下侧；"-1" 表示不指定 |
| `status` | `string` | 读/写 | 状态（如 "Guard"） |
| `facing` | `string` | 读/写 | 面向，取值0~256 |
| `tag` | `string` | 读/写 | 关联的标签 ID |
| `veterancy` | `string` | 读/写 | 经验等级：0=新兵，100=老兵，200=精英 |
| `group` | `string` | 读/写 | 分组 |
| `above_ground` | `string` | 读/写 | 是否在桥上 ("0" 或 "1") |
| `auto_no_recruit` | `string` | 读/写 | 重组 A |
| `auto_yes_recruit` | `string` | 读/写 | 重组 B |

## 方法

### `place()`
- **说明**：将当前步兵添加到地图（延迟刷新）。需脚本结束后或手动刷新才能看到。
- **返回**：无。

### `remove()`
- **说明**：删除地图中属性完全相同的 **第一个** 步兵。
- **返回**：无。

## 全局函数

### `place_infantry(house, type, x, y)`
- **说明**：快速放置步兵，使用默认属性（遵循格式预设），立即计划刷新。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 步兵类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回**：无。

### `remove_infantry(index)`
- **说明**：通过 **0 基索引** 删除步兵。
- **参数**：`index` (`number`) — 步兵在内部数组中的索引（从 0 开始）。
- **返回**：无。

### `remove_infantry(x, y, [sub_cell = -1])`
- **说明**：删除指定坐标（及可选子格）的步兵。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `sub_cell` (`number`, 可选) — 子单元格位置，`-1` 表示忽略子格。默认为 `-1`。
- **返回**：无。

### `get_infantry(index)`
- **说明**：通过 **0 基索引** 获取步兵对象。
- **参数**：`index` (`number`) — 0 基索引。
- **返回** (`infantry`)：步兵对象；若未找到，返回一个 `x,y` 为 `-1` 的无效对象。

### `get_infantry(x, y, [sub_cell = -1])`
- **说明**：获取指定坐标（及可选子格）的步兵对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `sub_cell` (`number`, 可选) — 子单元格位置，`-1` 表示忽略子格。
- **返回** (`infantry`)：步兵对象；若未找到，返回无效对象。

### `get_infantries()`
- **说明**：返回 **所有** 步兵的数组表。
- **返回** (`table<infantry>`)：步兵对象数组（索引从 1 开始）。

## 重要注释
- 若需修改步兵的属性（如 `facing`、`health`），推荐使用以下模式：
  1. 用 `get_infantries()` 获得所有步兵对象表。
  2. 遍历该表，对每个对象修改属性。
  3. 调用 `remove_infantry(i-1)`（注意索引转换）将其移除。
  4. 调用对象的 `obj:place()` 重新放置。
  5. 最后可调用 `update_infantry()` 或 `redraw_window()` 刷新界面。
- **索引注意事项**：`get_infantries()` 返回的数组索引从 1 开始，而 `remove_infantry(index)` 和 `get_infantry(index)` 使用的是从 0 开始的内部索引。因此在遍历并删除时，需将数组索引 -1 传入删除函数。为避免删除后索引变动，建议倒序遍历。

**示例**：
```lua
function get_random_facing()
    local facings = {0, 32, 64, 96, 128, 160, 192, 224}
    return facings[math.random(1, #facings)]
end

local objs = get_infantries()
for i = #objs, 1, -1 do
    local o = objs[i]
    remove_infantry(i-1)          -- 0基索引删除
    o.facing = tostring(get_random_facing())
    o:place()
end
update_infantry()
redraw_window()
```