## 十一、FA2 扩展功能

### 刷新函数

这些函数会强制从 INI 重新加载数据到内存，并在脚本结束后应用。若要立即显示，请接着调用 `redraw_window()`。

- `update_infantry()`
- `update_unit()`
- `update_aircraft()`
- `update_building()`
- `update_terrain()`
- `update_waypoint()`
- `update_node()`
- `update_overlay()`
- `update_tube()`
- `update_smudge()`
- `update_tiles()`
- `update_trigger()`：刷新触发（会通知已打开的触发编辑器窗口）。

**使用提示**：
- 通常在批量修改单位/建筑等对象后，调用对应的 `update_xxx()` 函数确保内存数据与 INI 同步，然后调用 `redraw_window()` 立即刷新视图。
- 对于直接通过 `write_string` 修改的 INI 内容，也需要相应 `update_xxx()` 才能让界面看到变化。

### 界面更新

#### `redraw_window()`
- **说明**：立即重绘主地图窗口。
- **返回**：无。

#### `update_minimap([x, y])`
- **说明**：重绘小地图。不传参数则全图重绘；传入坐标则只重绘那一格。
- **参数**：
  - `x, y` (`number`, 可选) — 要重绘的单元格坐标。
- **返回**：无。

### 快照

#### `create_snapshot()`
- **说明**：保存当前地图的完整状态（覆盖物、地形、INI）为一个快照，并返回其版本号。控制台会打印恢复命令提示，如 `restore_snapshot(1)`。
- **返回** (`number`)：快照版本号。
- **用法**：在执行有风险的大规模修改前调用，以便后续恢复。
```lua
if message_box("是否创建快照？", "提示", 2) == 1 then
    create_snapshot()
end
```

#### `restore_snapshot(version)`
- **说明**：恢复指定版本的快照。会弹出确认对话框，需要用户进行确认。
- **参数**：`version` (`number`) — 快照版本号。
- **返回**：无。

#### `clear_snapshot()`
- **说明**：清除所有已保存的快照。
- **返回**：无。

### 撤销与重做

#### `save_undo()`
- **说明**：记录当前覆盖物与地形的撤销点。通常在进行大量地形修改前调用。
- **返回**：无。

#### `save_undo_objects()`
- **说明**：记录游戏对象（建筑、车辆、飞行器、步兵、地形对象、污染、基地节点、隧道、路径点、单元标记）、地图注释、测距工具、可视区域大小的撤销点。通常在进行大量游戏对象修改前调用。
- **返回**：无。

### 其他

#### `in_map(x, y)`
- **说明**：判断坐标是否在地图范围内。
- **参数**：
  - `x, y` (`number`) — 坐标。
- **返回** (`boolean`)：是否在地图内。

#### `move_to(x, y)` / `move_to(waypoint_number)`
- **说明**：移动视野居中到指定坐标或数字路径点位置。若只传一个参数，则视为路径点数字。
- **参数**：
  - 若两个参数：`x, y` (`number`) — 目标坐标。
  - 若一个参数：`waypoint_number` (`number`) — 路径点数字。
- **返回**：无。

#### `running_lua_brush()`
- **说明**：返回当前脚本是否通过“脚本刷”模式执行。
- **返回** (`boolean`)：是否脚本刷模式。

### 脚本刷全局变量

仅在“脚本刷”运行时存在：

| 变量 | 类型 | 说明 |
|------|------|------|
| `X` | `number` | 鼠标点击的 X 坐标 |
| `Y` | `number` | 鼠标点击的 Y 坐标 |
| `first_run` | `boolean` | 是否是本次刷模式的首次点击 |
| `control_down` | `boolean` | 是否按下了 Ctrl 键 |
| `holding_click` | `boolean` | 是否由长按鼠标左键触发 |

**脚本刷示例**：
```lua
if running_lua_brush() then
    if not holding_click then
        local cell = get_cell(X, Y)
        -- 根据 first_run 等变量实现首次点击和后续点击的不同行为
    end
else
    message_box("本脚本仅能通过“脚本刷”运行！", "错误", 8)
end
```