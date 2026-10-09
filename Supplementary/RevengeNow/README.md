# Revenge Now (RN) 配置套件 / Revenge Now Configuration Suite

本目录提供面向 **Revenge Now (复仇时刻)** 项目的专属配置套件，旨在与 [`YR_RN_Mission_Editor`](file:///D:/Developments/RevengeNow/Codes/YR_RN_Mission_Editor) 地图编辑器行为全面对齐。

---

## 包含文件清单

1. **`FAData.ini`**：
   - 核心配置文件，通过模块化 `[Include]` 引用链加载配套子文件，并包含 RN 的阵营、模式、模板与剧场图块设置：
     - **RN 专属配置**：包含 `[Customizations]`、`[Sides]`（含第四阵营 `Antalian` / `安塔列`）、`[GameModes]`（13 种 RN 特色作战模式）、`[ForceUnitPalettePrefix]`、`[VehicleVoxelTurretsRA2Disabled]`、`[ScriptTemplates]`、`[TeamTemplates]` 与 `[ShoreTerrainTS]`。
     - **地图保存保序与编码配置**：
       - `SaveMap.PreserveINISorting=true`：保存地图时保持所有 INI 小节原有的出现顺序。
       - `SaveMap.AdaptiveSorting=true`：保存地图时自适应保持键值排布（数字索引小节自然升序，常规属性小节维持原序）。
       - `SaveMap.KeepComments=false`：不保留注释（符合 RN 要求）。
       - `UTF8Support.AlwaysSaveAsUTF8=true`：总是以 UTF-8 编码保存地图（符合 RN 要求）。

2. **`FAData_ObjectBrowser.ini`**：
   - 物件浏览器分类定义，注册了双语 `[Chinese-Sides]`（`3=安塔列`）与 `[English-Sides]`（`3=Antalian`），并融合了 RN 专属 `[IgnoreRA2]` 过滤列表。

3. **`FAData_TriggerAndScript.ini`**：
   - 现代化触发与动作脚本引擎：
     - 完整保留 FA2sp 原生 Phobos 与 Ares 现代触发扩展体系。
     - 整合 HAres 专属动作（146~154）与脚本（65~71），并提供中英双语规范化定义。
     - 注册 `[NewParamTypes]` 索引 `568=TeamTargetTechnoTypes,2,1,1,0`，实现脚本参数 22 动态读取规则文件中的科技类型列表与中文 UIName。
     - 配置 `[English-AITriggerSides]` 与 `[Chinese-AITriggerSides]`（`4=Antalian` / `4=安塔列`）。
     - 提供 `[SP_TargetList]` 与 `[ScriptExtras]` 兼容段。

4. **`FAData_RandomPlacement.ini`**：
   - 随机物件摆放预设配置。

5. **`FAData_bak.ini`**：
   - 旧版单体配置的原始备份，供对照与查验历史配置。

---

## 使用方法

在 RN 独立运行环境或打包发布中，将本目录内的 INI 配置文件放置于地编运行根目录即可。地编将通过 `[Include]` 链自动加载完整的 RN 定制系统与现代触发脚本引擎。
