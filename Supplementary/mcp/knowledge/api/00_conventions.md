# 基本约定

- **对象索引**：直接接受"索引"作为参数的函数（例如 `remove_unit(index)`）使用 **0 基索引**（C++ 内部数组下标）。返回的 Lua 数组表（如 `get_units()` 的结果）索引从 **1** 开始。具体约定以实际函数说明为准。
- **修改生效**：大部分地图对象的增删改不会立即刷新到界面，而是在 **脚本执行完毕后** 统一刷新。可以使用 `update_*` 系列函数触发刷新，也可以用 `redraw_window()` 立即重绘。触发、小队等需要通过对象的 `apply()` 方法写入 INI 文件。
  **重要**：对于通过 `get_infantry()`、`get_unit()` 等获取的对象，直接修改其属性（如 `facing`）后，必须将其从地图删除再重新放置（或调用 `place()` 方法），才能让修改生效并最终写入 INI。
- **默认值**：参数列表中以 `[可选参数 = 默认值]` 的形式标出。若不传递，则使用默认值。
- **类型**：`number` 为 Lua 整数或浮点数；`string` 为字符串；`boolean` 为 `true`/`false`；`table` 为 Lua 表；`object` 为特定类的实例；`nil` 表示无返回值或空。
- **文本编码**：程序内部均使用 ANSI（GBK） 编码，MCP 服务器输出的则为 UTF-8 编码。MCP 服务器会自动对编码进行转换，但 Lua 脚本直接读取/保存的文件则不会自动转换。
- **错误与警告**：部分操作会向输出窗口打印错误信息，本文档不再逐一说明。

---

## 文档目录索引

以下是各拆分文件的主要内容说明：

| 文件 | 主要内容 |
|------|----------|
| `01_global_state.md` | 全局只读状态变量：地图尺寸、各类对象数量、路径/阵营/地形信息、EXE/GAME/MAP 路径等 |
| `02_output_interaction.md` | 输出与交互函数：`print`、`message_box`、`input_box`、`open_file`/`save_file`、`end_script` 等 |
| `03_dialog_classes.md` | 对话框类：`select_box`（单选）、`multi_select_box`（多选）、`LuaDialog`（自定义对话框，支持 CheckBox/Edit/ComboBox/ListBox 等控件及事件回调） |
| `04_terrain_smudge.md` | 地形与覆盖物：地形对象（树木/石头）、污染（Smudge）、覆盖物（Overlay）、围墙的放置与删除，矿石平滑 |
| `05_waypoints.md` | 路径点：按坐标或 ID 放置、删除路径点 |
| `06_infantry.md` | 步兵类：构造函数、属性（所属方/类型/血量/坐标/子格/面向/经验等）、place/remove、全局 get/remove/place 函数 |
| `07_units.md` | 车辆类：构造函数、属性（含 follow）、place/remove、全局 get/remove/place 函数 |
| `08_aircraft.md` | 飞行器类：构造函数、属性、place/remove、全局 get/remove/place 函数 |
| `09_buildings_nodes.md` | 建筑与基地节点：building 类（含升级组件）、place_node、全局建筑操作函数 |
| `10_cells_tiles.md` | 单元格(cell)与地形块(tile)：cell 属性读写与 apply、tile 只读属性、地形放置/高度/隐藏/多选、海岸线/斜坡生成 |
| `11_ini_operations.md` | INI 读取与写入：基础读写(get_string/get_integer/get_bool 等)、枚举解析、参数操作(split_string/get_param/set_param)、特殊 ID 与转换(get_free_id/translate_house 等) |
| `12_fa2_extensions.md` | FA2 扩展功能：刷新函数(update_xxx)、界面更新(redraw_window/update_minimap)、快照(create_snapshot/restore)、撤销(save_undo)、脚本刷(running_lua_brush + X/Y 等全局变量) |
| `13_triggers_tags.md` | 触发与标签：trigger 类（构造函数、属性、事件/行为管理、标签管理、change_id/release_id/apply/delete）、单元格标记(place_celltag/remove_celltag) |
| `14_ai_triggers.md` | AI 触发：ai_trigger 类（构造函数、条件/对象/比较/权重/难度等属性、apply/change_id/delete） |
| `15_scripts.md` | 动作脚本：script 类（构造函数、行为/参数管理、add_action/delete_action/replace_action、get_script_type、参数组合 get_script_extra/combine_script_extra） |
| `16_taskforces.md` | 特遣部队：task_force 类（构造函数、成员管理 add_number/delete_number/replace_number、apply/delete） |
| `17_teams.md` | 作战小队：team 类（构造函数、所属方/特遣部队/脚本/路径点/经验/科技等级等属性、apply/delete），含典型创建流程示例 |
| `18_variables.md` | 变量操作：局部/全局变量的读写(get_variable_value/get_variable_name)、设置(set_variable_value/set_variable_name)、新增变量(add_variable) |
| `19_appendices.md` | 附录：message_box 格式码、tile.land_type 地表类型映射表、load_from 参数取值说明、参数类型返回值表 |