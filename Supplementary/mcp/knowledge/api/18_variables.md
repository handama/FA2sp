## 十七、变量 
#### `get_variable_value(index, [is_global = false])`
- **说明**：返回变量的默认值。
- **参数**：
  - `index` (`int`) — 从 0 开始的变量索引。
  - `is_global` (`bool`, 可选) — 若为 `true` 则返回对应的全局变量，否则返回局部变量。默认为 `false`。
- **返回** (`int`)：变量的默认值；若变量不存在则返回 `-1`。

#### `get_variable_name(index, [is_global = false])`
- **说明**：返回变量的名称。
- **参数**：
  - `index` (`int`) — 从 0 开始的变量索引。
  - `is_global` (`bool`, 可选) — 若为 `true` 则返回对应的全局变量，否则返回局部变量。默认为 `false`。
- **返回** (`string`)：变量的名称；若变量不存在则返回 `"MISSING"`。

#### `set_variable_value(index, value)`
- **说明**：设置局部变量的默认值。
- **参数**：
  - `index` (`int`) — 从 0 开始的变量索引。
  - `value` (`int`) — 新的默认值。
- **返回**：无。

#### `set_variable_name(index, name)`
- **说明**：设置局部变量的名称。
- **参数**：
  - `index` (`int`) — 从 0 开始的变量索引。
  - `name` (`string`) — 新的名称。
- **返回**：无。

#### `add_variable(name, value)`
- **说明**：新增一个局部变量。
- **参数**：
  - `name` (`string`) — 变量名。
  - `value` (`int`) — 默认值。
- **返回** (`int`)：从 0 开始的新变量的索引。