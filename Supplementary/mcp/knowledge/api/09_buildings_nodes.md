## 八、建筑与节点

### 建筑 （`building`）

#### `building(house, type, x, y)` （构造函数）
- **说明**：构造建筑对象（不立即放置，需调用 `place()`）。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 建筑类型 ID。
  - `x, y` (`number`) — 坐标。
- **返回** (`building`)：建筑对象。

**成员**：

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `house` | `string` | 读/写 | 所属方。可用所属方使用`available_houses()`获取 |
| `type` | `string` | 读/写 | 建筑类型 ID |
| `health` | `string` | 读/写 | 血量，取值0~256 |
| `x` | `number` | 读/写 | X 坐标 |
| `y` | `number` | 读/写 | Y 坐标 |
| `facing` | `string` | 读/写 | 面向 |
| `tag` | `string` | 读/写 | 关联标签的 ID |
| `ai_sell` | `string` | 读/写 | AI 变卖 |
| `ai_rebuild` | `string` | 读/写 | AI 重建（实际无效） |
| `powered` | `string` | 读/写 | 耗能/工作状态 |
| `upgrades` | `string` | 读/写 | 组件数 |
| `spot_light` | `string` | 读/写 | 探照灯 |
| `upgrade1` | `string` | 读/写 | 组件 1 的类型 ID |
| `upgrade2` | `string` | 读/写 | 组件 2 的类型 ID |
| `upgrade3` | `string` | 读/写 | 组件 3 的类型 ID |
| `ai_repair` | `string` | 读/写 | AI 修复 |
| `nominal` | `string` | 读/写 | 显示名称（实际无效） |

**方法**：

#### `place()`
- **说明**：将建筑写入地图 INI（不会立即刷新）。
- **返回**：无。

#### `place_node([delete_building = false])`
- **说明**：为当前建筑创建基地节点。若 `delete_building=true`，则删除地图上属性完全匹配的建筑。
- **参数**：
  - `delete_building` (`boolean`, 可选) — 是否删除建筑，默认为 `false`。
- **返回**：无。

#### `remove()`
- **说明**：删除地图上属性完全匹配的第一座建筑。
- **返回**：无。

### 全局函数

#### `place_building(house, type, x, y, [ignore_overlap = false])`
- **说明**：快速放置建筑。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 建筑类型 ID。
  - `x, y` (`number`) — 坐标。
  - `ignore_overlap` (`boolean`, 可选) — 若为 `true` 可跳过重叠检查，默认为 `false`。
- **返回**：无。

#### `remove_building(index)`
- **说明**：通过 **0 基索引** 删除建筑。
- **参数**：`index` (`number`) — 0 基索引。
- **返回**：无。

#### `remove_building(x, y)`
- **说明**：删除指定坐标的建筑。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。

#### `get_building(index)`
- **说明**：通过 **0 基索引** 获取建筑对象。
- **参数**：`index` (`number`) — 0 基索引。
- **返回** (`building`)：建筑对象；未找到时返回 `x,y` 为 `-1` 的无效对象。

#### `get_building(x, y)`
- **说明**：获取指定坐标的建筑对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回** (`building`)：建筑对象；未找到时返回无效对象。

#### `get_buildings()`
- **说明**：返回所有建筑的数组表。
- **返回** (`table<building>`)：建筑对象数组。

**重要注释**：修改建筑属性同样需要移除再重新放置。批量修改时的索引映射与单位一致。示例：随机化建筑血量。
```lua
local buildings = get_buildings()
for i = #buildings, 1, -1 do
    local b = buildings[i]
    remove_building(i-1)
    b.health = tostring(math.random(1, 256))
    b:place()
end
update_building()
redraw_window()
```

### 基地节点函数

#### `place_node(house, type, x, y, [index = -1])`
- **说明**：添加一个基地节点。
- **参数**：
  - `house` (`string`) — 所属方。
  - `type` (`string`) — 节点类型/建筑类型。
  - `x, y` (`number`) — 坐标。
  - `index` (`number`, 可选) — 插入位置（0 基），`-1` 则追加到末尾。默认为 `-1`。
- **返回**：无。

#### `remove_node(x, y)`
- **说明**：删除指定坐标的基地节点。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。
