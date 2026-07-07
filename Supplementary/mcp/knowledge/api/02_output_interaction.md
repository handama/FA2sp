## 二、输出与交互

### `print(...)`
- **说明**：在输出窗口打印文本。可以接受任意数量和类型的参数，每个参数用空格分隔。
- **参数**：`...` — 可变参数，支持 `nil`, `string`, `number`, `boolean`。
- **返回**：无。
- **示例**：`print("Hello", 123, true)`

### `clear()`
- **说明**：清空输出窗口中的文本。
- **返回**：无。

### `avoid_time_out()`
- **说明**：向主程序分发一次消息，防止脚本执行时间过长导致界面卡死（“未响应”）。在长时间循环中应适时调用。
- **返回**：无。
- **使用示例**：
```lua
for i = 1, 10000 do
    -- 大量操作
    if i % 1000 == 0 then
        avoid_time_out()
    end
end
```

### `sleep(ms)`
- **说明**：暂停脚本执行指定的毫秒数。
- **参数**：`ms` (`number`) — 毫秒数。
- **返回**：无。

### `end_script()`
- **说明**：立即终止当前 Lua 脚本的执行。
- **返回**：无。

### `exec(command, [options])`
- **说明**：执行外部程序/命令、打开 URL 或文件，支持同步/异步模式和输出捕获。
- **参数**：
  - `command` (`string`) — 要执行的命令、程序路径、URL 或文件路径。
  - `options` (`table`, 可选) — 配置选项表，支持以下键：
    - `async` (`boolean`, 默认 `false`) — 异步执行，不等待程序完成即返回。
    - `url` (`boolean`, 默认 `false`) — 当作 URL 用默认浏览器打开（不可与 `file` 同时使用）。
    - `file` (`boolean`, 默认 `false`) — 用系统关联的默认程序打开文件（不可与 `url` 同时使用）。
    - `capture` (`boolean`, 默认 `true`) — 捕获 stdout/stderr 输出并作为返回值。异步模式下自动设为 `false`。
    - `stream` (`boolean`, 默认 `false`) — 将命令输出实时写入 Lua Console 输出窗口（需 `capture=true`）。
    - `show` (`boolean`, 默认 `false`) — 显示进程窗口。
    - `cwd` (`string`, 默认 `""`) — 工作目录路径，为空则使用程序默认目录。
    - `timeout` (`number`, 默认 `0`) — 超时毫秒数，`0` 表示无限等待。
- **返回**：
  - 同步+capture 模式：返回命令输出内容的字符串。
  - 其他模式：成功返回 `true`，失败返回 `nil`。
- **错误处理**：失败时自动在 Lua Console 输出错误信息，同时返回 `nil`。
- **典型用法**：
```lua
-- 同步执行 ping 并捕获输出
local result = exec("ping 127.0.0.1 -n 3")
print("Ping result:")
print(result)

-- 实时查看 ping 输出
exec("ping 127.0.0.1 -n 5", { stream = true })

-- 在默认浏览器中打开网页
exec("https://github.com/handama/FA2sp", { url = true })

-- 用默认程序打开文件
exec("FA2sp.log", { file = true })

-- 带超时控制
local out = exec("ping 127.0.0.1 -n 10", { timeout = 5000 })
if out then
    print(out)
else
    print("Command timed out or failed")
end

-- 指定工作目录
exec("debug.log", { cwd = game_path().."debug\\", file = true })
```
