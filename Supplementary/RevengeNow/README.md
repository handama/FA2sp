# Revenge Now (RN) 配置套件 / Revenge Now Configuration Suite

本目录提供面向 **Revenge Now (复仇时刻)** 项目的专属配置套件，旨在与 [`YR_RN_Mission_Editor`](file:///D:/Developments/RevengeNow/Codes/YR_RN_Mission_Editor) 地图编辑器行为全面对齐。

---

## 包含文件清单

1. **`FAData.ini`**：
   - 基于 FA2sp 原生默认的 `Supplementary/FAData.ini` 构建，完整保留了所有现代组件、包含引用（`[Include]`）与物件浏览器配置（`[ExtConfigs]`）。
   - 默认启用了面向 RN 对齐保存所需的 3 项保序配置：
     - `SaveMap.PreserveINISorting=true`：保存地图时保持所有 INI 小节原有的出现顺序。
     - `SaveMap.PreserveINIKeySorting=true`：保存地图时保持小节内部键值对（Key=Value）原有的出现顺序（不再按字典/字符串长度排序）。
     - `SaveMap.KeepComments=true`：保存地图时保留注释。

---

## 使用方法

将本目录中的 `FAData.ini` 放置到 FA2 程序运行根目录（或覆盖使用），启动后保存地图即可保证 INI 文件结构、小节顺序与键值顺序与 `YR_RN_Mission_Editor` 完全一致。
