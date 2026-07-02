## 九、地形与单元格

### 单元格 （`cell`）

**获取方式**：`get_cell(x, y)` 返回。不能手动构造。

**成员** (属性)：

| 属性 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `x` | `number` | 只读 | X 坐标 |
| `y` | `number` | 只读 | Y 坐标 |
| `unit` | `number` | 只读 | 车辆索引 (Units 节内) |
| `infantry_1` | `number` | 只读 | 第一个步兵索引 |
| `infantry_2` | `number` | 只读 | 第二个步兵索引 |
| `infantry_3` | `number` | 只读 | 第三个步兵索引 |
| `aircraft` | `number` | 只读 | 飞行器索引 |
| `building` | `number` | 只读 | 建筑索引 |
| `terrain` | `number` | 只读 | 地形对象索引 |
| `terrain_type` | `number` | 只读 | 地形对象类型索引（rules 中 TerrainTypes 小节） |
| `smudge` | `number` | 只读 | 污染索引 |
| `smudge_type` | `number` | 只读 | 污染类型索引（rules 中 SmudgeTypes 小节） |
| `waypoint` | `number` | 只读 | 路径点索引 |
| `node_building` | `number` | 只读 | 基地节点关联的建筑类型索引 |
| `node_id` | `number` | 只读 | 基地节点索引 |
| `node_house` | `string` | 只读 | 基地节点所属阵营 |
| `overlay` | `number` | 读/写 | 覆盖物索引（0~65535， 65535 代表空） |
| `overlay_data` | `number` | 读/写 | 覆盖物数据（0~255） |
| `tile` | `number` | 读/写 | 地形索引 |
| `subtile` | `number` | 读/写 | 子地形索引 |
| `height` | `number` | 读/写 | 高度 |
| `cell_tag` | `number` | 只读 | 单元格标记索引 |
| `tube` | `number` | 只读 | 隧道索引 |
| `tube_data` | `number` | 只读 | 隧道方向数据 |
| `hidden` | `boolean` | 读/写 | 是否被“隐藏单个地形块”标记 |
| `alt_image` | `number` | 只读 | 替换图像索引 |

**方法**：

#### `is_hidden()`
- **说明**：判断该单元格是否因任何原因（单个隐藏或同类地形组隐藏）被隐藏。
- **返回** (`boolean`)：是否隐藏。

#### `is_multi_selected()`
- **说明**：判断该单元格是否处于多选模式选中状态。
- **返回** (`boolean`)：是否多选。

#### `apply()`
- **说明**：将对 `overlay`、`overlay_data`、`tile`、`subtile`、`height`、`hidden` 等属性的修改 **立即写入地图**，并刷新该单元格的预览。**重要**：修改属性后必须调用此方法才能生效！
- **返回**：无。

**提示**
- 修改 `tile`、`overlay` 等属性后，**必须调用 `cell:apply()`**，否则修改不会写入地图 INI。若需要立即看到效果，之后可调用 `redraw_window()`。
- 对于 `infantry_1`、`building`等索引，可以通过 `get_infantry()`、`get_building()`等函数获取对应游戏对象。 

**常见模式**：
```lua
local cell = get_cell(X, Y)
cell.tile = 0   -- 某种地形索引
cell.subtile = 0
local unit = get_unit(cell.unit)
cell:apply()
```

### 地形块 （`tile`） 

**说明**：表示一个具体的地形块（如一块悬崖的某一部分）。不能手动构造。
**获取方式**：`get_tile_block(tile_index, sub_index)` 或 `get_whole_tile(tile_index)`。

**成员** (均为只读)：

| 属性 | 类型 | 说明 |
|------|------|------|
| `x` | `number` | 该地形块在地形中的相对 X 偏移 |
| `y` | `number` | 该地形块在地形中的相对 Y 偏移 |
| `valid` | `boolean` | 该地形块是否有效 |
| `tile_index` | `number` | 所属地形的索引 |
| `tile_sub_index` | `number` | 子地形索引 |
| `height` | `number` | 该地形块的原始高度 |
| `alt_count` | `number` | 替换图像的数量 |
| `tile_set` | `number` | 所属地形组的索引 |
| `ramp_type` | `number` | 斜坡类型 |
| `land_type` | `string` | 地表类型字符串（见附录 B） |

### 地形操作函数

#### `get_tile_block(tile_index, subtile_index)`
- **说明**：获取指定的地形块对象。
- **参数**：
  - `tile_index` (`number`) — 地形索引。
  - `subtile_index` (`number`) — 子地形索引。
- **返回** (`tile`)：地形块对象。

#### `get_whole_tile(tile_index)`
- **说明**：获取指定地形的所有有效子块。
- **参数**：
  - `tile_index` (`number`) — 地形索引。
- **返回** (`table<tile>`)：该地形所有有效子块组成的数组。

#### `place_whole_tile(x, y, tile_index)`
- **说明**：在指定坐标放置整个地形（自动选取最佳子块）。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `tile_index` (`number`) — 地形索引。
- **返回**：无。

#### `place_tile(x, y, tile_obj, [height = -1], [alt_type = -1])`
- **说明**：手动放置一个 `tile` 对象。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `tile_obj` (`tile`) — 地形块对象。
  - `height` (`number`, 可选) — 高度，`-1` 表示使用当前单元格高度加上块高度。默认为 `-1`。
  - `alt_type` (`number`, 可选) — 替换图像类型，`-1` 表示随机选择。默认为 `-1`。
- **返回**：无。

#### `set_height(x, y, height)`
- **说明**：单独设置单元格高度。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `height` (`number`) — 高度值。
- **返回**：无。

#### `hide_cell(x, y, [type = 1])`
- **说明**：隐藏/显示单个单元格。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `type` (`number`, 可选) — 操作类型：1=隐藏，2=显示，3=取反。默认为 1。
- **返回**：无。

#### `hide_tile_set(tileset_index, [type = 1])`
- **说明**：隐藏/显示整个地形组。
- **参数**：
  - `tileset_index` (`number`) — 地形组索引。
  - `type` (`number`, 可选) — 操作类型，同 `hide_cell`。
- **返回**：无。

#### `multi_select_cell(x, y, [type = 1])`
- **说明**：多选/取消多选单元格。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `type` (`number`, 可选) — 操作类型，同 `hide_cell`。
- **返回**：无。

#### `multi_select_tile_set(tileset_index, [type = 1])`
- **说明**：多选/取消多选整个地形组。
- **参数**：
  - `tileset_index` (`number`) — 地形组索引。
  - `type` (`number`, 可选) — 操作类型。
- **返回**：无。

#### `create_shore(x1, y1, x2, y2)`
- **说明**：在矩形范围内自动生成海岸线。
- **参数**：
  - `x1, y1, x2, y2` (`number`) — 矩形对角坐标。
- **返回**：无。

#### `smooth_lat(x1, y1, x2, y2)`
- **说明**：平滑矩形范围内的地块过渡（lat 优化）。
- **参数**：
  - `x1, y1, x2, y2` (`number`) — 矩形对角坐标。
- **返回**：无。

#### `create_slope(x1, y1, x2, y2)`
- **说明**：在矩形范围内生成斜坡。
- **参数**：
  - `x1, y1, x2, y2` (`number`) — 矩形对角坐标。
- **返回**：无。

**补充**：进行大量地形修改前，建议调用 `save_undo()` 以允许用户回退。完成修改后调用 `redraw_window()` 和 `update_minimap()` 刷新界面。
