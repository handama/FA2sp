# Revenge Now (RN) 配置套件 / Revenge Now Configuration Suite

本目录提供面向 **Revenge Now (复仇时刻)** 项目的专属配置套件，旨在与 [`YR_RN_Mission_Editor`](file:///D:/Developments/RevengeNow/Codes/YR_RN_Mission_Editor) 地图编辑器行为全面对齐。

---

## 包含文件清单

1. **`FAData.ini`**：
   - 融合了 `YR_RN_Mission_Editor` 的完整 RN 定制内容与 FA2sp 现代化框架体系：
     - **RN 专属触发系统**：完整保留 RN 的触发动作（`[ActionsRA2]` / `[Chinese-ActionsRA2]` 共 157 项，含 Ares 动作 150~154 及参数微调）、事件（`[EventsRA2]` / `[Chinese-EventsRA2]`）、脚本（`[ScriptsRA2]` / `[Chinese-ScriptsRA2]`）、参数定义（`[ParamTypes]`、`[ScriptParams]`、`[PT_CrateTypes]` 等）以及 `[ScriptTemplates]`、`[TeamTemplates]`。
     - **RN 阵营与模式**：保留 `[Sides]`（12 个作战方定义）、`[GameModes]`、`[ForceUnitPalettePrefix]` 等。
     - **FA2sp 现代兼容层**：保留 `[Include]` 引用链（加载 `FAData_ObjectBrowser.ini` 保证物件浏览器中文分类与完整条目正常显示）与 `[ExtConfigs]`、现代剧场与图块管理定义。
   - 启用了面向 RN 对齐保存所需的保序配置：
     - `SaveMap.PreserveINISorting=true`：保存地图时保持所有 INI 小节原有的出现顺序。
     - `SaveMap.AdaptiveSorting=true`：保存地图时自适应保持键值排布（数字索引小节自然升序，常规属性小节维持原序）。
     - `SaveMap.KeepComments=false`：不保留注释（符合 RN 要求）。

---

## 使用方法

将本目录中的 `FAData.ini` 放置到 FA2 程序运行根目录（或覆盖使用），启动后保存地图即可保证 INI 文件结构、小节顺序与键值顺序与 `YR_RN_Mission_Editor` 完全一致。
