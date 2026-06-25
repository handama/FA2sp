# 六、车辆 (`unit`)

## 构造函数

### `unit(house, type, x, y)`
- **说明**：构造一个车辆对象（不立即放置，需调用 `place()`）。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 车辆类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回** (`unit`)：车辆对象。

## 成员

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `house` | `string` | 读/写 | 所属方 |
| `type` | `string` | 读/写 | 车辆类型 ID |
| `health` | `string` | 读/写 | 血量，取值0~256 |
| `x` | `number` | 读/写 | X 坐标 |
| `y` | `number` | 读/写 | Y 坐标 |
| `status` | `string` | 读/写 | 状态 |
| `facing` | `string` | 读/写 | 面向，取值0~256 |
| `tag` | `string` | 读/写 | 关联标签的 ID |
| `veterancy` | `string` | 读/写 | 经验等级：0=新兵，100=老兵，200=精英 |
| `group` | `string` | 读/写 | 分组 |
| `above_ground` | `string` | 读/写 | 是否在桥上 ("0" 或 "1") |
| `auto_no_recruit` | `string` | 读/写 | 重组A |
| `auto_yes_recruit` | `string` | 读/写 | 重组B |
| `follow` | `string` | 读/写 | 跟随的车辆 ID |

## 方法

### `place()`
- **说明**：将当前车辆添加到地图（延迟刷新）。
- **返回**：无。

### `remove()`
- **说明**：删除地图中属性完全相同的 **第一个** 车辆。
- **返回**：无。

## 全局函数

### `place_unit(house, type, x, y)`
- **说明**：快速放置车辆，使用默认属性，立即计划刷新。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 车辆类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回**：无。

### `remove_unit(index)`
- **说明**：通过 **0 基索引** 删除车辆。
- **参数**：`index` (`number`) — 0 基索引。
- **返回**：无。

### `remove_unit(x, y)`
- **说明**：删除指定坐标的车辆。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

### `get_unit(index)`
- **说明**：通过 **0 基索引** 获取车辆对象。
- **参数**：`index` (`number`) — 0 基索引。
- **返回** (`unit`)：车辆对象；若未找到，返回 `x,y` 为 `-1` 的无效对象。

### `get_unit(x, y)`
- **说明**：获取指定坐标的车辆对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回** (`unit`)：车辆对象；若未找到，返回无效对象。

### `get_units()`
- **说明**：返回所有车辆的数组表。
- **返回** (`table<unit>`)：车辆对象数组。

## 重要注释
车辆对象的修改方法与步兵类似，需要"删除 - 修改属性 - 重新放置"的三步走。批量修改时务必倒序遍历以避免索引错乱。

**示例**：
```lua
local objs = get_units()
for i = #objs, 1, -1 do
    local u = objs[i]
    remove_unit(i-1)    -- 0基索引
    u.facing = tostring(math.random(0, 7) * 32)
    u:place()
end
update_unit()
redraw_window()
```