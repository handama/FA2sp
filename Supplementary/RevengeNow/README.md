# Revenge Now (RN) 配置套件 / Revenge Now Configuration Suite

本目录提供面向 **Revenge Now (复仇时刻)** 项目的专属配置套件，旨在与 [`YR_RN_Mission_Editor`](file:///D:/Developments/RevengeNow/Codes/YR_RN_Mission_Editor) 地图编辑器行为全面对齐。

---

## 包含文件清单

1. **`FAData.ini`**：
   - 直接移植自 `YR_RN_Mission_Editor/MissionEditor/data/FinalAlert2/FAData.ini`（保留了原作者 Zero Fanker 的全套 RN 专属定制）。
   - 包含 RN 专属小节与模板：
     - `[Sides]`（安塔列星等 12 个阵营分类定义）
     - `[GameModes]`（标准作战、混战、淘金热、攻城拔寨等 RN 模式）
     - `[ForceUnitPalettePrefix]`
     - `[ScriptTemplates]`（巡逻、卸载攻击等脚本模板）
     - `[TeamTemplates]`（空降部队、AI 生产小队、突击小队等队伍模板）
     - `[Customizations]` 与 `[Debug]`（单向隧道支持、轨道逻辑开启等）
   - 仅增加了与目标编辑器保存对齐所需的 3 项保序配置：
     - `SaveMap.PreserveINISorting=true`：保存地图时保持所有 INI 小节原有的出现顺序。
     - `SaveMap.PreserveINIKeySorting=true`：保存地图时保持小节内部键值对（Key=Value）原有的出现顺序（不再按字典/字符串长度排序）。
     - `SaveMap.KeepComments=true`：保存地图时保留注释。
   - **不包含任何 Phobos 扩展特性**。
2. **`TileGroups.ini`**：
   - 移植自 `YR_RN_Mission_Editor/dist/FinalRevenge/TileGroups.ini`，包含温和气候、雪地、城市等地形的分组与快速选择配置。
3. **`FinalRevengeDefaults.ini`**：
   - 移植自 `YR_RN_Mission_Editor/dist/FinalRevenge/FinalRevengeDefaults.ini`，包含预设界面参数（缩放、显示网格等）。

---

## 使用方法

将本目录中的 `FAData.ini`、`TileGroups.ini`（及所需辅助文件）放置到 FA2 程序运行根目录（或覆盖使用），启动后保存地图即可保证 INI 文件结构、小节顺序与键值顺序与 `YR_RN_Mission_Editor` 完全一致。
