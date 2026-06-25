# 一、全局状态变量（只读函数）

| 函数 | 返回类型 | 说明 |
|------|----------|------|
| `iso_size()` | `number` | 地图对角线长度（宽+高） |
| `width()` | `number` | 地图宽度（单元格） |
| `height()` | `number` | 地图高度（单元格） |
| `local_width()` | `number` | 当前可视区域宽度 |
| `local_height()` | `number` | 当前可视区域高度 |
| `local_top()` | `number` | 可视区域距顶端的偏移（像素/格?） |
| `local_left()` | `number` | 可视区域距左侧的偏移 |
| `waypoint_count()` | `number` | 路径点数量（`Waypoints` 节键数量） |
| `unit_count()` | `number` | 车辆（Units）数量 |
| `infantry_count()` | `number` | 步兵（Infantry）数量 |
| `building_count()` | `number` | 建筑（Structures）数量 |
| `aircraft_count()` | `number` | 飞行器（Aircraft）数量 |
| `terrain_count()` | `number` | 地形对象（Terrain）数量 |
| `smudge_count()` | `number` | 污染（Smudge）数量 |
| `player_count()` | `number` | 玩家数量。多人地图返回路径点 0~7 的实际数量；单人地图返回 1 |
| `house_count()` | `number` | 所有阵营/所属方数量（`Houses` 节计数） |
| `country_count()` | `number` | 国家数量（`Countries` 节计数） |
| `node_count([house])` | `number` | 基地节点总数。若提供 `house` 参数，只统计该阵营的节点；否则统计全部 |
| `trigger_count()` | `number` | 触发（Triggers）数量 |
| `tile_count()` | `number` | 当前地形类型的 **地形块种类** 总数 |
| `tile_set_count()` | `number` | 当前地形类型的 **地形组** 数量 |
| `tag_count()` | `number` | 标签（Tags）数量 |
| `theater()` | `string` | 当前地图地形名称，如 `"TEMPERATE"` |
| `is_multiplay()` | `boolean` | 是否为多人地图 |
| `language()` | `string` | 当前地编界面语言 |
| `exe_path()` | `string` | 地编程序所在目录路径 |
| `game_path()` | `string` | 游戏资源目录（含 mix 文件）路径 |
| `map_path()` | `string` | 当前地图文件的完整路径 |
| `scale_factor()` | `number` | 当前程序的缩放倍率（浮点数） |
| `available_houses()` | `table` | 当前可用的所属方（阵营）列表，每个元素为一个字符串 |

**使用示例**：
```lua
-- 遍历地图上所有单元格时，可用 width() 和 height() 获取边界
for x = 0, width() - 1 do
    for y = 0, height() - 1 do
        -- 操作每个单元格
    end
end
```