
## 1.1 [Basic]单人任务设置

**代码和解释**

[Basic]

此条目内容存在于地编多个窗口,部分条目只存在于官方任务图，其中大部分都不用管。

(1)NextScenario下一主线剧情

**填写mapselmd.ini中定义的地图名称。（包括扩展名的完整名称）**

(2)AltNextScenario下一支线剧情

**填写mapselmd.ini中定义的地图名称。（包括扩展名的完整名称）（进入支线任务时游戏不会自动存档）**

(3)SkipMapSelect跳过地图选择

**填yes或no**。另外如果对关卡结构没有改动，此三条可不改动。详见[http://pkuit.com/forum.php?mod=viewthread&tid=205309](http://pkuit.com/forum.php?mod=viewthread&tid=205309&highlight=%BD%CC%B3%CC)

(4)NewINIFormat新ini格式

在红警2中，此部分固定为4，不过在pb扩展了此部分。

(5)CarryOverCap继承金钱

填数字。从前一关获得的最大资金。例如填5000就最多只能获得5000。（已在modenc上修改表述）

(6)CarryOverMoney继承上关的资金

填数字。继承上关的资金的百分比。比如填0.100000为10%。（以上两者取最小值）（已在modenc上修改表述）

(7)Percent百分比（金钱）

一代残留，玩家在上一个任务结束时剩余的钱将有多少百分比被带入下一个任务。实际功能由CarryOverMoney实现。

(8)EndOfGame最后一关

**填yes或no**。注意当此任务为最后一关时应勾选，以避免评分屏后的弹窗。

(9)SkipScore跳过评分屏

**填yes或no**。游戏结束后直接进入下一关。

(10)OneTimeOnly只一次

**填yes或no。打完这关就回主界面，不进入下一关。

(11)Official为官方地图

**填yes或no**。若为no，则联机时会主动向其他玩家发送此地图，不联机没啥用。

(12)IgnoreGlobalAITriggers忽略遭遇战AI控制

**填yes或no**。设置为no时，允许ai触发则ai会像遭遇战模式一样造兵。（按照aimd.ini）

(13)TruckCrate车辆木箱

**填yes或no**。为yes时，**打爆**IsTrain=no**且CarriesCrate=yes的载具，在载具所在位置生成随机箱子。若遭遇战页面关闭升级工具箱（ra2md.ini中CratesAppear=no），则不会生成随机箱子。

(14)TrainCrate列车木箱

**填yes或no**。为yes时，**打爆**IsTrain=yes**且CarriesCrate=yes的载具，在载具所在位置生成随机箱子。若遭遇战页面关闭升级工具箱（ra2md.ini中CratesAppear=no），则不会生成随机箱子。

(15)Theme本关播放的游戏背景音乐

填写thememd.ini注册的音乐名称。游戏开始时自动播放该BGM，如果想要单曲循环，就在thememd.ini里改。当然，如果之前是停止播放状态，bgm自然不会播放。

(16)Player玩家所属方

填写所属方。（仅限单人任务时。多人地图不应出现此条目）

以下部分代码为影片资料，在artmd.ini里注册影片，序号=影片文件名字。

(17)Intro介绍影片

填写在artmd.ini里注册的影片。这个是进入载入界面前播放的影片。

(18)Brief简报影片

填写在artmd.ini里注册的影片。效果同上，若两者都有则只播放`Intro介绍的影片`。

(19)Win胜利影片

填写在artmd.ini里注册的影片。玩家胜利后，在进入评分屏前播放的影片。

(20)Lose失败影片

填写在artmd.ini里注册的影片。玩家失败后，在选择是否重新开始前播放的影片。

(21)Action行为参数

填写在artmd.ini里注册的影片。载入界面后游戏开始前播放的影片，但只在从上一关进入时（或选关进入时）有效，也就是说在这关`重新开始`，影片不会播放。

(22)PostScore评分屏之后播放的影片

填写在artmd.ini里注册的影片。这个是评分屏之后播放的影片，例如尤复盟7评分屏后 尤里被关押 的影片。

(23)PreMapSelect预地图选择

**TS残留，并且并非选择地图，而是TS战役有一个地图定位的图像效果，用这个语句定义一个影片在该图像效果之前播放。**

(24)TimerInherit继承全局计时器。

**填写yes或no**。上一个任务的全局计时器是否会延续到这个任务，本任务开始时全局定时器被设置为在前一个任务结束时剩余的值。需要用触发行为23-27和103来实现全局计时器的设置。值得注意的是，梦幻世界盟军的a11到a13的全局计时器不用这个，是用了大量触发配合全局变量来实现倒计时的记录和传递。这是因为无法预估或用触发(14 计时器时间已到)判断剩余时间，也就不能实现`在剩余XX时做XX事`等更多效果。

(25)FillSilos无效 ~~tc2中可能使用~~

(26)StartingDropships存在大量bug，不应使用。

详见<https://modenc.renegadeprojects.com/StartingDropships>

<https://modenc.renegadeprojects.com/Dropship_loadout>

(27)HomeCell出生点

填写路径点。在任务开始时，玩家的视野以这个路径点为中心，无MCV或建造厂时按home键（或空格）返回这点。如果所填路径点不存在，则进入游戏时视野默认在地图可见区域中心，即右上小地图中心位置。

(28)AltHomeCell填路径点

当设置了全局变量0（即0=<Alternate Start Location>），那在之后的关里，此路径点将代替HomeCell所设置路径点。详见<https://modenc.renegadeprojects.com/AltHomeCell>。（没啥卵用）

(29)MultiplayerOnly是否多人游戏

填写数字。多人填1，单人填0。

不写Player=XXX且MultiplayerOnly=1的地图会被当成多人地图。（当然，还需要改后缀）

(30)TiberiumGrowthEnabled启用矿石生长

**填写yes或no**。设置为no时，[SpecialFlags]中的TiberiumGrows和TiberiumSpreads均无效，但矿柱依然可以生产黄金矿。

(31)VeinGrowthEnabled无效。

(32)IceGrowthEnabled冰层生长，无效。

(33)TiberiumDeathToVisceroid步兵能否变成器官兽，无效，ares重新启用。

(34)FreeRadar无偿雷达

**填写yes或no。**即使玩家没有雷达，也有小地图。

(35)InitTime初始化时间

无效。初始化时间只和电脑性能和地图文件大小有关。