
## 1.4 [SpecialFlags]特定标识

**代码和解释**

[SpecialFlags]

~~此部分似乎是rule里[MultiplayerDialogSettings]部分在地图中的存在形式~~

(1)DestroyableBridges能否破坏桥梁

填写yes或no。即使设置为no但有桥梁维修小屋时，伊文和谭雅海豹也能炸断桥。

(2)MCVDeploy基地车部署

**无效**，填写yes或no。建造厂(ConstructionYard=yes)是否能变回基地车。Pb中可用MCVRedeploys实现基地车重新部署。

(3)TiberiumGrows矿石生长（上图的`Ore grows`）

填写yes或no。黄金矿是否可以自我生长。设置为no时，黄金矿不能自我生长，也就是价值不变。受TiberiumGrowthEnabled制约。

在rules文件中，矿石在[Tiberiums]下注册，每一种矿都可以设置自己的GrowthPercentage和SpreadPercentage。宝石矿的这两个值都为0，所以不会生长或蔓延。

(4)TiberiumSpreads矿石蔓延（上图的`Ore spreads`）

填写yes或no。黄金矿是否可以自我蔓延。设置为no时，黄金矿不能自我蔓延。受TiberiumGrowthEnabled制约。另外存读档次数过多，会导致黄金矿更快自我蔓延。开启后若一局游戏时间过长，会导致游戏崩溃。

(5)TiberiumExplosive矿车被摧毁时随身矿石是否会爆炸

无效。Ares实现了这一效果。

(6)InitialVeteran初始为老兵

填写yes或no。所有初始单位为精英~~（明明是精英非说是老兵）~~，遭遇战有效。

(7)HarvesterImmune矿车免疫伤害（无效）

(8)FogOfWar战争迷雾（上图的`shroud`）

填写yes或no。即战争迷雾，半透明的那层，不需要额外素材支持。原版和尤复的效果不同，原版效果更好但可能弹窗。使用时应设置迷雾扩散速度（FogRate）以避免扩散过快。存在大量bug，如不取消迷雾中建筑的有电动画、建筑炮台缺失、部分非玩家单位的视野丢失、飞行类单位移动过程中视野不断重置、黑雾中单位丢失选中效果等（千万不要开**持续**的全图，指间谍卫星那种，触发的显示全图能用），其他见<https://modenc.renegadeprojects.com/FogOfWar>。实际效果见mo`苏军特殊行动：原型`。

(9)Inert惰性效果

填写yes或no。设置为yes时，所有常规伤害作废。（Is’t wonderful?）

WallAbsoluteDestroyer=yes也不能摧毁围墙。除了碾压，尤里心控，飞碟吸，真C4爆破建筑，乌贼的斩杀，寄生武器秒杀步兵，超时空兵攻击，地形杀（包括磁电吸载具坠毁、断桥、超时空传送等），铁幕、超时空传送对步兵的秒杀等`伤害`之外，其他都无效。神经突击车的混乱（这个打步兵，步兵会匍匐）无效。一般情况下，只有尤里的控制能触发`被特定所属方攻击`。

(10)Meteorites是否随机产生陨石

无效，多人游戏地图是否出现随机陨石。

(11)Visceroids无效。

(12)FixedAlliance是否能更改结盟状态

填写yes或no。用于多人游戏，是否能更改结盟状态。

(13)IonStorms是否随机产生离子风暴

无效。
