
## 1.6 [Countries]国家&[Houses]所属方

[Countries] [Houses] ;此为标准注册表

[Countries]为国家，[Houses]为所属方。

**在地编的表述中，国家和所属方往往混为一谈，这是不正确的。**在作战小队、触发等中，`所属方`实际为**国家**，只是由于国家和所属方的序号恰好相同，才不会出现问题。但大多mapper都习惯使用`所属方`的叫法而非`国家`，本文只在需要区别时使用`国家`代替`所属方`，其他地方依然延用`所属方`的叫法。

触发里和所属方有关的事件、行为所需参数要按注册顺序来填，不能因为地图注册表里没有写10就把12 Neutral认为是10，10实际上是rule(md).ini里的GDI。任何新增加的国家都应该从14（即rule注册表后）开始排。填-1为任意国家。具体规则见：<https://bbs.ra2diy.com/forum.php?mod=viewthread&tid=16046>

注：国家和所属方的名字可以更改，甚至国家名为`A`，所属方名为`B`都可以,这只是一个名字。

本节以`[Russians]国家苏联`和`[Russians House]苏联所属方`为例，来说明具体的所属方相关的功能设置。

[Russians]

(1)Name名字= Russians

此部分不能有重名，不然会出现一个国家对应多个所属方的情况。

(2)Side阵营=Nod

填写对应rulesmd.ini的[Sides]中定义的的阵营，GDI表示盟军，Nod表示苏军，ThirdSide表示尤里，Civilian表示平民，Mutant表示变种人/治安。国家属于盟军阵营，其伞兵和残兵（crew）出的就是大兵，其他同理。一般习惯上认为：平民是不归玩家管的无武装单位；`变种人`是不归玩家管的地方武装部队，其名字来源于泰伯利亚。

(3)Color颜色=DarkRed

填写rulesmd.ini中定义的颜色。注意此处设置的颜色无效，具体由对应的House的颜色决定。

(4)Prefix=B

未知效果，盟军为A，苏军和尤里为B，平民为C。

(5)Suffix=Soviet

未知效果，盟军为Allied，苏军和尤里为Soviet，平民为CIV。

(6)SmartAI=yes聪明AI，无效。

(7)CostUnitsMult=1单位造价折扣百分比

填数字。影响VehicleTypes单位造价，其造价=原造价\*这个数。矿场因为会赠送矿车，其造价也会变动。

(8)ParentCountry伴生国家

填写对应rules(md).ini中的国家。当ai的IQ满足自动补充矿车时，ai可生产Owner为ParentCountry的矿车。

(9)MultiplayPassive是否平民阵营

填true或false。真正用于区分平民阵营和其他阵营。其对应的国家将被视为**平民**。因此将具有以下平民阵营的特色：~~①遭遇战中不会出现在可选国家列表中(Multiplay=no（默认no）的效果)；~~②无视AI和基地节点；③即使玩家与之双向结盟，也不会获得其视野；④其拥有的超级武器，不暴露视野，不会在右下角显示倒计时；⑤RevealToAll=yes的建筑由平民所属方转为非平民所属方时不会显示视野。

注：①在rulesmd.ini中，平民和治安都有这一条。如果用地编新建平民所属方，还需要在对应的[Countries]下加上这个，使其具有平民阵营的特色，详见：[http://pkuit.com/forum.php?mod=viewthread&tid=205376](http://pkuit.com/forum.php?mod=viewthread&tid=205376&highlight=%BD%CC%B3%CC)

②[Countries]并不是很重要，当使用标准所属方时甚至可以不用在地图里写常规的Country（常见于尤复官方任务图）。当然为了避免误删而报错，别乱改地编安排好的国家。

接下来我们讨论所属方即[xxx House]设置。

[Russians House]

(1)IQ智商=5

填数字。只填0-5，对应rulesmd.ini中的[IQ]

000=TESLA,6,82

001=NAPOWER,7,80

xxx=AAAAA，X，Y

…

序号=建筑注册名，X，Y

对应所属方的建筑的基地节点。左边的序号代表建造的先后顺序。

注：①如果所属方和国家的名称相同，你将不能用地编给其建筑加上基地节点，但可以用记事本加，例如官方原版苏军第二关。

②如果这个建筑没有在rules中注册而是在地图中注册，给其加基地节点会导致地编报错。

(2)NodeCount建筑节点总数=2

填数字。建筑节点总数。

(3)Edge地图边界=North

填写东西南北。地图边界指东南西北四个方位。对于下述①情况，北为上方向；对于下述②③情况，北为右上方向。①伞兵运输机、鲍里斯召唤的米格默认从这个方位飞来。②对应所属方刷出的步兵InfantryTypes和载具VehicleTypes初始方向，但与飞行器AircraftTypes的方向相反。③触发行为108刷箱子但路径点被挡住时，箱子有更大可能出现在与触发所属方地图边界方向相同的位置。（这是一个概率问题）

附：有Dock的飞行器会在弹药耗尽或攻击目标消失后返回Dock（机场）；无Dock的飞行器在攻击完毕且弹药耗尽时飞向所属方地图边界并离开地图，在攻击完毕且弹药未耗尽时飞上地图上方并离开地图。

(4)Color颜色=DarkRed

填写rulesmd.ini中定义的颜色。

(5)Allies盟友=Russians House,…

填具体所属方的House，用英文逗号隔开。此行内容应少于128字符，多出部分的无效。ares将内容上限提高到了512字符。

注：在地编的表述中，不结盟的即为敌对，但实际上双方只是不结盟，并非真正敌对，例如超武可能不会对其释放，作战小队勾选[`OnlyTargetHouseEnemy](#OnlyTargetHouseEnemy)`时可能不会攻击对方，因为此时**双方没有仇恨**。想实现真正的敌对，可以用触发行为38，或用其他方法制造仇恨。仇恨的获得方式有多种，可实现某些特殊效果（如超武只打固定所属方），详见<https://bbs.ra2diy.com/forum.php?mod=viewthread&tid=23550#lastpost>。

(6)Country对应国家=Russians

填写对应的国家。优先对应地图中的Country，若没有找到，则找rulesmd.ini里的Country。每个House必须有对应的Country，不然会报错。一个国家只能对应一个所属方，一对多会出现所属方问题。

(7)Credits开局资金=0

填数字。游戏内获得的实际资金=此数值×100。如果想要资金不为100的整数，可以设法在开局给玩家一个具有油井效果的建筑来实现资金细节变化；或是修改CampaignMoneyDeltaEasy（简单难度额外的钱，可以是负数）和CampaignMoneyDeltaHard（困难难度额外的钱，可以是负数）来实现不同难度不同资金（以上两条只影响PlayerControl=yes的阵营）。

(8)TechLevel科技等级=10

填写数字。玩家所属方无法建造高于其科技等级的任何单位和建筑，ai所属方无法建造高于其科技等级的任何单位。（ai基地节点无视建筑的科技等级限制，例如TechLevel=-1或100，ai可以建造科技等级为-1的单位和建筑）

(9)PercentBuilt=100

可能是无效的。在官方任务中，无节点的均为0，有节点的均为100。modenc的解释是：为0则必须按照其节点来进行建造，为100则不必。但是实际上值为0或100，ai都会严格按照基地节点来建造；若没有节点，在开启自动建设基地模式后，ai会自动建造基地，无论值是0还是100，这说明ai没有按照其节点来建造。反复测试各种情况也没能复现modenc所说的情况，所以笔者认为这个是无效的。

注：`ai自动建造基地`内容见`[基地节点指南1.0](#基地节点指南)`的`4-自动建设基地模式`。

(10)PlayerControl=yes玩家能否操作该阵营

填写yes或no。玩家是否能控制该阵营的单位。

PlayerControl=yes，当玩家没有该阵营的某建筑视野（例如可控制的敌对阵营建筑），游戏不会渲染出完整的建筑图像，该建筑也会**丧失部分功能**（如耗电量、雷达效果等）；如果首次发现该建筑是通过间谍卫星这种持续性的全图视野（如修改大师的全图），会导致发现时游戏依然不会渲染出完整的建筑图像；只有非间谍卫星发现才会正常渲染。若PlayerControl=yes但不为玩家所属方，玩家不能变卖该所属方的建筑，但能维修，花费对应所属方的资金，其建筑受到攻击时副官也会提示；可以控制其单位进出建筑；不能使用该所属方的科技和超武，也不能进行生产。

如果该阵营PlayerControl=no但是玩家阵营，那玩家依然可以操控该阵营单位，进行生产、维修、变卖等活动。Rule中难度设置带来的攻速增减益分别影响PlayerControl=yes和PlayerControl=no的所属方。

注：放在地图可见范围外的建筑是不提供视野的。所以想让PlayerControl=yes或玩家阵营的建筑在可见范围外也能生效，需要提供额外的视野，即用触发行为17显示路径点周围区域。（由于地图高度等原因，建筑不一定能在显示区域内或地图内，需要按实际情况调整路径点或建筑位置）

Some more logics will start to fail after 24 houses, for example if a house with an index >= 24 has some disguise sensing unit or structure. If this happens, the memory corruption will shut down your game pretty quickly.一些更多的逻辑将在第24个所属方之后（即要有24个所属方）开始失效，例如，一个索引>=24的所属方的单位（包括建筑）能发现伪装单位（如间谍）。~~如果发生这种情况，内存泄露会使游戏崩溃。（并不一定会崩溃）~~
