# 地图 Spec 工作流（Map Spec Workflow）

> 定位：把"模糊需求 → 具体地图代码"的细化过程持久化为随地图走的 spec 文件，
> 使设计意图可以跨会话、跨 agent 接续。**任何地图项目的设计工作都应先走本技能。**

## 适用场景

- 用户提出一个地图需求（"做个五角大楼防守图""限时防守任务"…），需要设计剧情与触发；
- 接续之前未完成的地图工作（本会话开始、或新会话接手）；
- 修改已有地图的设计（改剧情、加关卡线、调整触发归属）；
- 任何"设计意图需要留档"的地图工作。

## 核心概念

- **spec 文件**：与地图同址同名，`mymap.map` ↔ `mymap.spec.md`。三层结构：
  1. **故事**（Story）——一段话概括全流程，最顶级设计。**整个故事只有一个主线**，
     由用户提供（agent 可协助润色/扩写，最终以用户为准）；
  2. **剧本**（Screenplay）——由故事拆解出的演出条目，条目 ID 形如 `L01S02`
     （流水线 ID + 线内序号，**工具分配，永不复用**）。条目按**触发流水线**分组：
     - **触发流水线**（line）= 并行推进的触发链条，如"苏军进攻流水线"、
       "AI 盟友增援流水线"、"对手防线强化流水线"；
     - 流水线**不是**剧情线——剧情只有一个主线，流水线是驱动剧情演出的并行机制；
     - 流水线按**逻辑先后**推进：条目顺序由前置 DAG 决定，绑定的是逻辑条件
       （触发条件达成），而非时间流逝——虽然逻辑在先的条目在时间上也必然先发生，
       但流水线的推进不依赖计时器本身；
     - 条目间用 `depends_on`（前置）构成依赖 DAG：串行 = 前置链，并行 = 无互相依赖，
       **禁止成环**；跨流水线的前置 = 同步点；
  3. **实现**（Implementation）——条目 → 触发器映射表，含状态机：
     `designing`（设计中）→ `implemented`（已实现）→ `verified`（已验证），另有 `deprecated`（已废弃）。
- **访问方式**：**禁止直接读改写 spec 文件**。一切通过 MCP `spec` 工具，
  由 `.spec_lib.lua`（`Scripts/.spec_lib.lua`）保证结构、校验与不变量。

## 需求拆解清单（开始设计前向用户澄清）

- [ ] 地图主题/胜利条件是什么？（防守 / 进攻 / 限时 / 多线）
- [ ] 故事主线是否明确？（主线只有一个，由用户确认）
- [ ] 有哪些**触发流水线**需要并行推进？（每条流水线 = 一个 Lxx 分组，
      如"AI 盟友随时间抵达""对手随时间巩固防线"）
- [ ] 玩家阵营与 AI 阵营？（决定触发 house 与小队归属）
- [ ] 关键演出节点（增援、台词、爆炸演出等）的先后依赖？

## 工具用法（MCP `spec` 工具，11 个 action）

所有 action 都需传 `map_path`（地图完整路径，正斜杠）。返回 JSON：
`{"ok":true,...}` 或 `{"ok":false,"error":"..."}`。

| action | 必需参数 | 说明 |
|--------|---------|------|
| `init` | `title?` | 创建 spec 文件；已存在则报错（**不会覆盖**） |
| `read` | — | 读取完整 spec；文件缺失时降级返回 `exists:false`（非错误） |
| `update_story` | `text` | 整体替换故事部分 |
| `add_line` | `name` | 新增触发流水线，返回分配的流水线 ID（L01/L02…） |
| `add_entry` | `line`, `summary`, `depends_on?` | 新增剧本条目，返回分配的 ID；校验前置存在且不成环 |
| `update_entry` | `entry_id`, `summary?`, `depends_on?` | 修改条目；前置变更同样做环检测 |
| `deprecate_entry` | `entry_id` | 标记已废弃；仍有触发映射时给出提示 |
| `link_trigger` | `entry_id`, `trigger_type`, `display_name?` | 建立映射；**校验触发器真实存在于地图 `[Triggers]`**；跨条目复用给出警告 |
| `unlink_trigger` | `entry_id`, `trigger_type` | 解除映射 |
| `set_status` | `entry_id`, `status` | `designing / implemented / verified / deprecated` |
| `validate` | — | 全量体检：引用完整性、环、悬空映射、孤儿触发器 |

### 会话示例

```
spec(init, map_path="C:/maps/pentagon.map", title="五角大楼防御战")
spec(update_story, text="苏联准备进攻并摧毁五角大楼…")
spec(add_line, name="苏军进攻流水线")        → L01
spec(add_line, name="AI盟友增援流水线")      → L02
spec(add_line, name="对手防线强化流水线")    → L03
spec(add_entry, line="L01", summary="苏军登陆")            → L01S01
spec(add_entry, line="L01", summary="建立基地", depends_on=["L01S01"]) → L01S02
spec(add_entry, line="L02", summary="增援分批抵达", depends_on=["L01S02"]) → L02S01
spec(link_trigger, entry_id="L01S01", trigger_type="SovietLanding",
     display_name="苏军登陆触发")
spec(set_status, entry_id="L01S01", status="implemented")
spec(validate)
```

## 标准工作流

**会话开始**：先 `read`，三种分支——
- 有 spec → 汇报当前进度（各条目状态），接续未完成部分；
- 无 spec 但地图已有内容 → 询问用户：补全 spec，还是仅局部工作；
- 无 spec 新图 → 走下方新建流程。

**新建/大改（逐层确认，逐场景推进）**：

```
① 故事       用户写故事（agent 可协助润色），直至用户确认
     ↓
② 剧本       agent 把故事拆解为剧本（流水线 + 条目 + 前置 DAG），
             拆解完向用户询问 → 多轮修改直至用户确认
     ↓
③ 实现       逐场景循环：
             实现剧本第一个场景（建触发器 → link_trigger → set_status）
             → 向用户询问 → 多轮修改直至用户确认 → 继续实现下一个场景
```

要点：
- **逐层确认**：故事、剧本、每个场景的实现，都要经过"产出 → 用户确认 → 修改 → 再确认"的循环，
  不允许一口气实现完整个剧本再交差。
- **变更随时发生**：用户在任何一步提出的修改都正常处理——没有"修改阶段"这个固定位置。
  修改设计时自顶向下同步：先改故事/剧本，再改实现映射，**禁止只改触发器不更新 spec**。
- **实现顺序**：每个场景先在地图里创建触发器，再 `link_trigger`（顺序不可反）；
  完成后 `set_status(implemented)`；用户实测通过后 `set_status(verified)`。

**会话结束前**：`validate`，向用户报告 error / warning / info 三项清单。

## 常见坑

1. **绝不手写 spec 文件**——哪怕改一个字也走工具；格式漂移是跨会话协作的头号杀手。
2. **流水线 ≠ 剧情线**：流水线是并行的触发链条（驱动演出的机制），不是多条故事线；
   剧情只有一个主线，别把"支线剧情"塞成一条新流水线——支线剧情是主线里的条目，
   流水线是它的触发机制归属。
3. **触发器先建后链**：`link_trigger` 会校验存在性，流程上必须"先在地图建触发，再链映射"。
4. **依赖只表达"必须发生在前"**：别把并行关系写进 depends_on 强行串行；
   并行就是无依赖，由 DAG 自然表达。
5. **读 spec 用工具，不要用 run_lua 去解析文件**：spec 文件内部有机读 JSON 块，
   但解析逻辑在 `.spec_lib.lua` 中，重复实现容易与格式定义失步。
6. **编码约定**：地编内部是 GBK，外部是 UTF-8，注意转换。
