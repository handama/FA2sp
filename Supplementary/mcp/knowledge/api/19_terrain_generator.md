## 十八、地形生成器 (`terrain_generator`)

地形生成器类封装了 CTerrainGenerator 窗口的全部功能：预设的加载、保存与管理，以及对指定区域或多选区域的地形、覆盖物、地形对象、污染和斜坡的随机生成。实例自带独立的预设数据（读取/写入程序目录下的 `TerrainGenerator.ini`），与地形生成器窗口是否打开无关。

### 构造函数

#### `terrain_generator:new()`
- **说明**：创建地形生成器实例，并自动从 `TerrainGenerator.ini` 加载与当前地图类型（Theater）匹配的预设（与地形生成器窗口一致，只载入 `Theaters` 包含当前地图类型的预设）。
- **返回** (`terrain_generator`)：地形生成器实例。
- **示例**：`local tg = terrain_generator:new()`

### 成员

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `override` | `boolean` | 读/写 | 应用前先清除区域内的对应对象（等价窗口的 Override 勾选） |
| `ignore_landtypes` | `boolean` | 读/写 | 生成时忽略地表类型限制（等价窗口的 IgnoreLandtypes 勾选） |

### 预设管理

#### `list()`
- **说明**：列出所有已加载预设（仅包含与当前地图类型匹配的预设）。每个元素为包含 `id`、`name`、`theaters` 字段的表。
- **返回** (`table<table>`)：预设信息数组，如 `{{id="000", name="地表美化01", theaters={"TEMPERATE"}}, ...}`。
- **示例**：
```lua
local tg = terrain_generator:new()
for i, p in ipairs(tg:list()) do
    print(p.id .. " - " .. p.name .. " [" .. table.concat(p.theaters, ",") .. "]")
end
```

#### `load(id)`
- **说明**：按 ID 载入预设，使其成为当前预设。仅能载入已加载（与当前地图类型匹配）的预设。
- **参数**：`id` (`string`) — 预设 ID（如 `"001"`）。
- **返回** (`boolean`)：是否加载成功。
- **示例**：`tg:load("001")`

#### `get_id()`
- **说明**：返回当前预设的 ID。
- **返回** (`string`)：当前预设 ID；未载入时返回空字符串。

#### `get_name()`
- **说明**：返回当前预设的名称。
- **返回** (`string`)：当前预设名称。

#### `get_theaters()`
- **说明**：返回当前预设适用的地图类型列表。
- **返回** (`table<string>`)：地图类型数组，如 `{"TEMPERATE","URBAN"}`。

#### `add(name)`
- **说明**：新建一个预设（自动分配首个空闲的 3 位数字 ID），并使其成为当前预设。
- **参数**：`name` (`string`) — 新预设名称。
- **返回** (`string`)：新预设的 ID。

#### `copy(id)`
- **说明**：复制指定预设（新 ID，名称追加“Copy”），并使其成为当前预设。
- **参数**：`id` (`string`) — 源预设 ID。
- **返回** (`string`)：新预设的 ID；源预设不存在时返回空字符串。

#### `remove(id)`
- **说明**：从 `TerrainGenerator.ini` 中删除指定预设。
- **参数**：`id` (`string`) — 预设 ID。
- **返回** (`boolean`)：是否删除成功。

#### `save()`
- **说明**：把当前预设的全部参数写入 `TerrainGenerator.ini` 并落盘。修改预设参数后调用此方法保存。
- **返回**：无。

> 提示：`save()`、`add()`、`copy()`、`remove()` 写盘后，若地形生成器窗口已打开，会自动刷新窗口中的预设列表，无需手动刷新。

#### `reload()`
- **说明**：重新从磁盘加载 `TerrainGenerator.ini` 中与当前地图类型匹配的预设（放弃未保存的修改）。
- **返回**：无。

### 应用生成

#### `apply(x1, y1, x2, y2)`
- **说明**：把当前预设应用到指定矩形区域（坐标范围闭合，即包含 x2,y2）。生成行为与窗口的 Apply 按钮一致，包含撤销记录。
- **参数**：`x1, y1, x2, y2` (`number`) — 区域坐标（单元格坐标）。
- **返回**：无。

#### `apply_selection()`
- **说明**：把当前预设应用到地图上当前多选选中的单元格（只作用于选中格，等价窗口的多选应用）。
- **返回**：无。没有多选区域时输出提示并返回。

#### `clear(x1, y1, x2, y2, [clear_type = -1])`
- **说明**：清除指定矩形区域内当前预设所包含类别的对象（等价窗口的 Clear 按钮）。
- **参数**：
  - `x1, y1, x2, y2` (`number`) — 区域坐标。
  - `clear_type` (`int`, 可选) — 只清除指定类别：`-1` 全部，`0` 地形，`1` 地形对象，`2` 覆盖物，`3` 污染。默认为 `-1`。
- **返回**：无。

#### `clear_selection([clear_type = -1])`
- **说明**：清除当前多选区域内当前预设所包含类别的对象。
- **参数**：
  - `clear_type` (`int`, 可选) — 只清除指定类别：`-1` 全部，`0` 地形，`1` 地形对象，`2` 覆盖物，`3` 污染。默认为 `-1`。
- **返回**：无。

### 预设参数设置

以下方法均修改当前预设，修改后需调用 `save()` 落盘。组索引从 0 开始，最多 9 组（与 ini 键 `TileSet0~9`、`TerrainType0~9`、`Overlay0~9`、`Smudge0~9` 对应）。添加新组时 `index` 必须等于当前组数；修改已有组时 `index` 为其下标。

#### `set_name(name)`
- **说明**：设置当前预设名称。
- **参数**：`name` (`string`) — 名称。
- **返回**：无。

#### `set_scale(scale)`
- **说明**：设置地块精细度（数值越小地块越碎，推荐 30 左右）。
- **参数**：`scale` (`int`) — 精细度。
- **返回**：无。

#### `set_theaters(theaters)`
- **说明**：设置适用的地图类型（Theater）列表。
- **参数**：`theaters` (`table<string>`) — 地图类型名数组，如 `{"TEMPERATE","URBAN"}`。
- **返回**：无。

#### `set_tileset(index, tileset, chance, [available_indexes])`
- **说明**：设置地形组。`tileset` 为地形组编号（0~9999 内为官方地形组，>=10000 为自定义地形组）；`available_indexes` 为可选的地形块相对索引列表（相对于该地形组起始索引），不提供则使用该地形组全部可用地形块。
- **参数**：
  - `index` (`int`) — 组索引（0~9）。
  - `tileset` (`int`) — 地形组编号。
  - `chance` (`number`) — 生成权重（浮点数，0 表示不使用）。
  - `available_indexes` (`string`, 可选) — 逗号或空格分隔的相对索引，如 `"0,2,4"`。
- **返回**：无。

#### `remove_tileset(index)`
- **说明**：删除指定地形组。
- **参数**：`index` (`int`) — 组索引。
- **返回**：无。

#### `set_terrain(index, items, chance)`
- **说明**：设置地形对象组。
- **参数**：
  - `index` (`int`) — 组索引（0~9）。
  - `items` (`table<string>`) — 地形对象类型 ID 数组，如 `{"TREE0","TREE1"}`。
  - `chance` (`number`) — 生成权重。
- **返回**：无。

#### `remove_terrain(index)`
- **说明**：删除指定地形对象组。
- **参数**：`index` (`int`) — 组索引。
- **返回**：无。

#### `set_overlay(index, overlays, chance, [available_data])`
- **说明**：设置覆盖物组。`available_data` 为可选的覆盖物图像数据索引列表（逗号或空格分隔），不提供则使用全部可用图像。
- **参数**：
  - `index` (`int`) — 组索引（0~9）。
  - `overlays` (`table<int>`) — 覆盖物类型索引数组。
  - `chance` (`number`) — 生成权重。
  - `available_data` (`string`, 可选) — 允许的图像数据索引，如 `"0,1,2"`。
- **返回**：无。

#### `remove_overlay(index)`
- **说明**：删除指定覆盖物组。
- **参数**：`index` (`int`) — 组索引。
- **返回**：无。

#### `set_smudge(index, items, chance)`
- **说明**：设置污染组。
- **参数**：
  - `index` (`int`) — 组索引（0~9）。
  - `items` (`table<string>`) — 污染类型 ID 数组，如 `{"CRATER1","CRATER2"}`。
  - `chance` (`number`) — 生成权重。
- **返回**：无。

#### `remove_smudge(index)`
- **说明**：删除指定污染组。
- **参数**：`index` (`int`) — 组索引。
- **返回**：无。

#### `set_ramp(percent, min_height, max_height)`
- **说明**：启用斜坡（坡度）模式并设置参数。三项参数必须均为非负值才启用；应用时会在区域内按连通块生成渐变高度。
- **参数**：
  - `percent` (`int`) — 粗糙度百分比（0~100）。
  - `min_height` (`int`) — 最低高度（0~13）。
  - `max_height` (`int`) — 最高高度（1~14）。
- **返回**：无。

#### `disable_ramp()`
- **说明**：关闭斜坡模式。
- **返回**：无。

#### `set_preserve_anchor_heights(preserve)`
- **说明**：设置斜坡生成时是否保留锚点高度。
- **参数**：`preserve` (`boolean`) — 是否保留。
- **返回**：无。

#### `set_avoid_nonmorphable_tiles(avoid)`
- **说明**：设置斜坡生成时是否避开不可变形的地形块。
- **参数**：`avoid` (`boolean`) — 是否避开。
- **返回**：无。

### 斜坡锚点

锚点为全局状态（与地形生成器窗口共用）。锚点用于在斜坡生成时固定某些顶点的高度。

> 提示：创建 `terrain_generator` 实例时会记录全局锚点集合，实例销毁（Lua 垃圾回收）时自动还原，脚本中的锚点操作不会残留到全局状态。

#### `get_anchors()`
- **说明**：返回当前所有锚点。
- **返回** (`table<table>`)：锚点数组，每个元素为 `{x=..., y=..., height=...}`，如 `{{x=78, y=78, height=10}, ...}`。

#### `add_anchor(x, y, height)`
- **说明**：在指定坐标放置一个高度锚点。
- **参数**：
  - `x, y` (`int`) — 坐标。
  - `height` (`int`) — 锚点高度。
- **返回**：无。

#### `remove_anchor(x, y)`
- **说明**：删除指定坐标的锚点。
- **参数**：`x, y` (`int`) — 坐标。
- **返回**：无。

#### `clear_anchors()`
- **说明**：清空全部锚点。
- **返回**：无。

### 查询

| 方法 | 返回类型 | 说明 |
|------|----------|------|
| `get_scale()` | `int` | 当前预设的精细度 |
| `get_tileset_list()` | `table<int>` | 当前预设各地形组的地形组编号 |
| `get_tileset_chances()` | `table<number>` | 当前预设各地形组的权重 |
| `get_ramp_percent()` | `int` | 斜坡粗糙度（未启用斜坡时为 -1） |
| `get_ramp_min_height()` | `int` | 斜坡最低高度（未启用斜坡时为 -1） |
| `get_ramp_max_height()` | `int` | 斜坡最高高度（未启用斜坡时为 -1） |

### 典型用法

**加载已有预设并应用到区域**：
```lua
local tg = terrain_generator:new()
if tg:load("001") then
    tg:apply(5, 5, 20, 20)   -- 对矩形区域生成
    tg:apply_selection()     -- 对当前多选区域生成
end
```

**新建预设、设置参数并应用**：
```lua
local tg = terrain_generator:new()
local id = tg:add("沙漠地形")
tg:set_scale(30)
tg:set_tileset(0, 41, 100, "0")        -- 地形组 0，仅用第一个地形块
tg:set_overlay(0, {0}, 50)             -- 覆盖物类型 0，权重 50
tg:set_smudge(0, {"CRATER01", "CRATER02"}, 30)
tg:set_ramp(30, 2, 8)                  -- 启用斜坡
tg:add_anchor(10, 10, 6)               -- 固定 (10,10) 顶点高度为 6
tg:apply(10, 10, 30, 30)
tg:save()                              -- 保存预设到 ini
```

**清除区域内容**：
```lua
local tg = terrain_generator:new()
tg:load("001")
tg:clear(5, 5, 20, 20)      -- 清除区域内对象
tg:clear_selection()        -- 清除多选区域内对象
```

**注意**：`list()` 返回每个预设的 `id`（如 `"001"`），载入时使用 `tg:load(p.id)`。与地形生成器窗口一致，只有 `Theaters` 包含当前地图类型的预设才会被加载；其他地图类型的预设需切换地图后使用。若地形生成器窗口正处于打开状态，两者的预设数据以 `TerrainGenerator.ini` 文件为同步点。
