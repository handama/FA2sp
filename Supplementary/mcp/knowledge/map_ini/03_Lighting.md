
## 1.3 [Lighting]光照设置

**代码和解释**

[Lighting]

红绿蓝均为0，即0,0,0时为全黑。

`环境`为地图整体亮度；`等级`为地形高度每加1时增加的亮度；`地面`为高度为的0地形的额外**负**亮度。三者对亮度的影响程度非1:1:1。

一般做地图要避免地图过亮或者过暗，可适当进行自行调试达到最佳光照效果，必要的话可以在地图中添加适当灯光建筑或使用AlphaImage补充光照。

可用触发行为71、72、73来改变环境照明。但地编显示的数值和记事本中实际记录的不一致，详见<https://www.bilibili.com/read/cv13844311/>。还可用触发行为142、143、144重绘红绿蓝颜色。

值得注意的是，在[Lighting]中的颜色设置（RGB）只影响**地面**和**地形对象**（不影响矿柱），而环境光照设置和触发行为对颜色的改动还会影响地面、建筑、载具、步兵、飞行器等。因此，如果通过触发改变光照强度或颜色，可能导致**地面**亮度和单位亮度的差异明显。（例如，原本地板高亮但触发把光照调到底）

为避免地面亮度明显高于单位亮度，常规光照的红绿蓝（RGB）不应超过1.0。

载入存档不会重绘颜色，所以载入存档后会**保留**载入前游戏的重绘颜色效果。但重开后游戏会自动重绘颜色。例如在任务`黄金眼`苏军第六关中，触发重绘颜色后存档再读档，会导致颜色非常诡异。解决方法是，重开后再读档进入游戏。

注：如果游戏刚开始，任务作者就用触发重绘了颜色，那就只能在**游戏内颜色与存档的相同**时读档，来获取正常的游戏画面。

题外话，大多数人认为mo单位太黑，这一方面是因为素材本身偏暗，另一方面是因为mo将单位的额外光照减少了。[AudioVisual]中的ExtraUnitLight、ExtraInfantryLight、ExtraAircraftLight在尤复中均为0.2，但mo改为了0.08，单位看起来更暗了。

（If **AmbientChangeRate** is defined in map code, you must also redefine **ExtraUnitLight**, **ExtraInfantryLight** and **ExtraAircraftLight**, else these are reset to rules value without decimal component.）见<https://modenc.renegadeprojects.com/AmbientChangeRate>
