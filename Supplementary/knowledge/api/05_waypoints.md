# 四、路径点

### `place_waypoint(x, y, [id = -1])`
- **说明**：在指定坐标放置路径点。若该格已有路径点，则不放置并返回该点 ID。
- **参数**：
  - `x, y` (`number`) — 坐标。
  - `id` (`number`, 可选) — 指定的路径点数字 ID，`-1` 表示自动分配。默认为 `-1`。
- **返回** (`number`)：放置的路径点 ID。
- **示例**：`local wid = place_waypoint(10, 20, 5)`

### `remove_waypoint(id)`
- **说明**：删除指定数字 ID 的路径点。
- **参数**：`id` (`number`) — 路径点 ID。
- **返回**：无。

### `remove_waypoint_at(x, y)`
- **说明**：删除指定坐标上的路径点。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回**：无。