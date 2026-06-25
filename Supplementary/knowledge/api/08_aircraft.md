# 七、飞行器 (`aircraft`)

## 构造函数

### `aircraft(house, type, x, y)`
- **说明**：构造一个飞行器对象（不立即放置，需调用 `place()`）。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 飞行器类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回** (`aircraft`)：飞行器对象。

## 成员

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `house` | `string` | 读/写 | 所属方 |
| `type` | `string` | 读/写 | 飞行器类型 ID |
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

## 方法

### `place()`
- **说明**：将当前飞行器添加到地图（延迟刷新）。
- **返回**：无。

### `remove()`
- **说明**：删除地图中属性完全相同的 **第一个** 飞行器。
- **返回**：无。

## 全局函数

### `place_aircraft(house, type, x, y)`
- **说明**：快速放置飞行器，使用默认属性，立即计划刷新。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 飞行器类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回**：无。

### `remove_aircraft(index)`
- **说明**：通过 **0 基索引** 删除飞行器。
- **参数**：`index` (`number`) — 0 基索引。
- **返回**：无。

### `remove_aircraft(x, y)`
- **说明**：删除指定坐标的飞行器。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

### `get_aircraft(index)`
- **说明**：通过 **0 基索引** 获取飞行器对象。
- **参数**：`index` (`number`) — 0 基索引。
- **返回** (`aircraft`)：飞行器对象；若未找到，返回无效对象。

### `get_aircraft(x, y)`
- **说明**：获取指定坐标的飞行器对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回** (`aircraft`)：飞行器对象；若未找到，返回无效对象。

### `get_aircrafts()`
- **说明**：返回所有飞行器的数组表。
- **返回** (`table<aircraft>`)：飞行器对象数组。

**注意**：飞行器对象的修改方式与步兵、车辆相同，需要使用"删除-修改-放置"的流程。