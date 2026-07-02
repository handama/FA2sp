**关于地图1.3.9**

## 前言：

本节的主要内容是归类各个map内置ini的条目，并给出一般性介绍。地图文件.map的代码就其起作用的部分一般分为三类：(1) 对于地图属性基础设置。(2) 对于直接用地编摆放的建筑单位和地形对象等的设置。(3)作战小队和触发。我们将按照这一顺序介绍本文主要内容。

一. 地图基础属性设置

[Header]可以防读取地图时弹窗，需设置NumberStartingPoints=0，就这一行有用，其他的都没用，但建议把前面的也复制一份。
[Basic]基础设置。
[Map]地图，如大小尺寸、气候等。
[Lighting]光照设置。
[SpecialFlags]特殊设置。
[Ranking]评分页面内容，包括用时、标题、内容。
[Countries]国家注册表。
[Houses]所属方注册表。

下面是常规Country和House。

| 国家 | 所属方 |
| --- | --- |
| [Africans] | [Africans House] |
| [Alliance] | [Alliance House] |
| [Americans] | [Americans House] |
| [Arabs] | [Arabs House] |
| [British] | [British House] |
| [Confederation] | [Confederation House] |
| [French] | [French House] |
| [Germans] | [Germans House] |
| [Neutral] | [Neutral House] |
| [Russians] | [Russians House] |
| [Special] | [Special House] |
| [YuriCountry] | [YuriCountry House] |

注：基地节点写在具体house下如[Alliance House]。具体个数与其节点数NodeCount对应，否则在载入时就会弹窗。添加新所属方时一般要勾选派生国家。

[Digest]（地图结尾，地图的唯一标识。在多人游戏中用于检测哈希值来判断地图是否一致，不一致则向其他玩家发送地图。所以单人图用不到）

二. 建筑单位和地形对象等设置

[Infantry]步兵选项。
[Aircraft]飞行器选项。
[Structures]建筑选项。
[Units]载具选项。
[CellTags]单元标记。
[Waypoints]路径点。
[Terrain]地形对象，树、路灯什么的。
[Smudge]污染。

三. 作战小队和触发设置

[VariableNames]局部变量，在rules里则表示全局变量，不能在地图里新建全局变量。
[TaskForces]特遣部队的注册表。
[ScriptTypes]脚本的注册表。
[TeamTypes]作战小队的注册表。
[Triggers]触发选项，即触发第一项。
[Events]触发事件，即触发第二项。
[Actions]触发行为，即触发第三项。
[Tags]标签。
[AITriggerTypes]AI触发编辑。
[AITriggerTypesEnable]AI触发允许。

四. 其他内容

~~[FA2spVersionControl]使用sp版地编保存的次数。~~
[IsoMapPack5]具体地形。（经过加密）
[OverlayDataPack]覆盖图。（经过加密）
[OverlayPack] （同上）
[Preview]地图缩略图大小。
[PreviewPack]地图缩略图。

建立单人地图时，绝大多数情况地编会在保存时删掉其中的缩略图内容，所以你看到下面是空的。这两块一般会很大，建议检查一下地编有没有删，没删就自己删一下。

建立多人地图时，地图需要有这两个来展示预览图，[Preview]的Size所框定的尺寸至少为1，否则在游戏绘制该预览图时会冻结。此外Ares的`地图快照（抓取地图）`功能保存的地图就没有这条，需要手动添加至少一个Size=0,0,1,1。缩略图部分应在文件整体的前25%。
