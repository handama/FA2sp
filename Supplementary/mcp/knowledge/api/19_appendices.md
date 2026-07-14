## 附录

### A. `message_box` 格式码

| 数值 | 样式 |
|------|------|
| 0 | 确认 |
| 1 | 确认 / 取消 |
| 2 | 是 / 否 |
| 3 | 是 / 否 / 取消 |
| 4 | 重试 / 取消 |
| 5 | 终止 / 重试 / 忽略 |
| 6 | 取消 / 重试 / 继续 |
| 7 | 带信息图标的确认 |
| 8 | 带警告图标的确认 |
| 9 | 带错误图标的确认 |
| 10 | 带询问图标的是 / 否 |
| 11 | 带信息图标的确认 / 取消 |
| 12 | 带警告图标的确认 / 取消 |
| 13 | 带错误图标的确认 / 取消 |
| 14 | 带询问图标的确认 / 取消 |
| 15 | 带错误图标的重试 / 取消 |
| 16 | 带错误图标的终止 / 重试 / 忽略 |

### B. `tile.land_type` 返回值

| 地表原始数值 | 返回字符串 |
|-------------|----------|
| 0x0, 0xD | `"clear"` |
| 0x1, 0x2, 0x3, 0x4 | `"ice"` |
| 0x5 | `"tunnel"` |
| 0x6 | `"railroad"` |
| 0x7, 0x8, 0xF | `"rock"` |
| 0x9 | `"water"` |
| 0xA | `"beach"` |
| 0xB, 0xC | `"road"` |
| 0xE | `"rough"` |

### C. `load_from` 参数取值

| 值 | 读取文件 |
|----|----------|
| `"map"` | 当前地图文件 |
| `"fadata"` | FAData.ini（地编目录下） |
| `"rules"` | rulesmd.ini |
| `"rules+map"` | rulesmd.ini + 地图文件 |
| `"art"` | artmd.ini |
| `"sound"` | soundmd.ini |
| `"theme"` | thememd.ini |
| `"ai"` | aimd.ini |
| `"ai+map"` | aimd.ini + 地图文件 |
| `"eva"` | evamd.ini |
| `"theater"` | 当前地形文件 |
| `"xxx.ini"` | 游戏目录下相对路径的 INI 文件（支持 mix 内读取） |

### D. 参数类型返回值 (`get_event_type` / `get_action_type` / `get_script_type`)

| 返回字符串 | 含义 |
|-----------|------|
| `"WAYPOINT_STR"` | 字母格式路径点 |
| `"WAYPOINT_NUM"` | 数字格式路径点 |
| `"COUNTRY"` | 国家索引 |
| `"TECHNO"` | 科技类型 ID |
| `"TRIGGER"` | 触发 ID |
| `"CSF"` | CSF 标签 |
| `"TAG"` | 标签 ID |
| `"FLOAT"` | 浮点数 |
| `"TEAM"` | 小队 ID |
| `"VARIABLE_GLOBAL"` | 全局变量索引 |
| `"VARIABLE_LOCAL"` | 局部变量索引 |
| `"AIRCRAFT"` | 地图内飞机索引 |
| `"INFANTRY"` | 地图内步兵索引 |
| `"UNIT"` | 地图内车辆索引 |
| `"STRUCTURE"` | 地图内建筑索引 |
| 其他数字字符串 | FAData.ini 中 `[NewParamTypes]` 的键名 |