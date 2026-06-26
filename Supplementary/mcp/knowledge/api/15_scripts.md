## 十四、动作脚本 (`script`)

#### `script([id])` （构造函数）
- **说明**：构造脚本对象。不提供 `id` 则自动分配。
- **参数**：`id` (`string`, 可选) — 脚本 ID。
- **返回** (`script`)：脚本实例。

**成员**：

| 成员 | 类型 | 读写 | 说明 |
|------|------|------|------|
| `id` | `string` | 只读 | 脚本 ID |
| `name` | `string` | 读/写 | 名称 |
| `actions` | `table<int>` | 只读 | 脚本行为数组（整数） |
| `params` | `table<int>` | 只读 | 对应行为的参数数组（整数） |

**方法**：

#### `add_action(action_number, param_number)`
- **说明**：添加一行脚本行为。
- **参数**：
  - `action_number` (`number`) — 行为编号。
  - `param_number` (`number`) — 参数编号。
- **返回**：无。

#### `delete_action(index)`
- **说明**：删除第 `index` 行（1 基）。
- **参数**：`index` (`number`) — 行索引。
- **返回**：无。

#### `replace_action(index, action_number, param_number)`
- **说明**：替换第 `index` 行行为（1 基）。
- **参数**：
	- `index` (`number`) — 行索引。
	-  `action_number` (`number`) — 行为编号。
	- `param_number` (`number`) — 参数编号。
- **返回**：无。

#### `apply()`
- **说明**：写入 INI。
- **返回**：无。

#### `change_id(new_id)`
- **说明**：修改 ID。
- **参数**：`new_id` (`string`) — 新 ID。
- **返回**：无。

#### `release_id()`
- **说明**：释放当前 ID。
- **返回**：无。

#### `delete()`
- **说明**：删除此脚本。
- **返回**：无。

**静态函数**：

#### `delete_script(id)`
- **说明**：删除指定脚本。
- **参数**：`id` (`string`) — 脚本 ID。
- **返回**：无。

#### `get_script(id)`
- **说明**：获取脚本对象。
- **参数**：`id` (`string`) — ID。
- **返回** (`script` 或 `nil`)：对象或 `nil`。

#### `get_scripts()`
- **说明**：获取所有脚本的 ID。
- **返回** (`table<int, string>`)：值数组。

#### `get_script_type(action_type, [extra = false])`
- **说明**：获取指定脚本行为的参数类型（`extra=true` 时获取额外参数类型）。详见附录 D。
- **参数**：
  - `action_type` (`number`) — 脚本行为类型。
  - `extra` (`boolean`, 可选) — 是否获取额外参数类型，默认为 `false`。
- **返回** (`string`)：类型字符串。

#### `script_has_extra(action_type)`
- **说明**：获取指定脚本行为的是否有额外参数。
- **参数**：`action_type` (`number`) — 脚本行为类型。
- **返回** (`boolean`)：是否有额外参数。

#### `get_script_extra(param, [get_extra = false])`
-   **说明**：获取行为参数的额外或基本部分。行为参数由低 16 位（基本参数）和高 16 位（额外参数）组合而成。
-   **参数**：
    -   `param`  (`number`) — 组合的行为参数值。
    -   `get_extra`  (`bool`, 可选) — 若为  `true`  则返回额外参数（高 16 位），否则返回基本参数（低 16 位）。默认为  `false`。
-   **返回**  (`number`)：基本参数或额外参数的值。

#### `combine_script_extra(param, extra_param)`
-   **说明**：将基本参数和额外参数组合成一个完整的行为参数（32 位整数）。
-   **参数**：
    -   `param`  (`number`) — 基本参数（将被置于低 16 位）。
    -   `extra_param`  (`number`) — 额外参数（将被置于高 16 位）。
-   **返回**  (`number`)：组合后的行为参数值。

**使用提醒**：所有修改必须在 `apply()` 后生效。