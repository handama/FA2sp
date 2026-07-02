# 额外注册表
以下小节内的具体内容仅为参考, 可能具有时效性问题, 使用时优先依据 `load_from` 和小节名寻找内容   
如果要给玩家作为选项框显示, 则记得把名字一起打印出来提升可读性   

## FAData.ini
以下小节 `load_from` 参数取值为 `fadata`, 从地编目录下读取   

### `[AITargetCategories]`

| ID | 选项 |
|---|---|
| 0 | <未使用> |
| 1 | 任意目标 |
| 2 | 建筑物 |
| 3 | 矿车矿场 |
| 4 | 步兵 |
| 5 | 车辆 |
| 6 | 生产建筑 |
| 7 | 防御建筑 |
| 8 | 小队威胁 |
| 9 | 电厂 |
| 10 | 驻军建筑 |
| 11 | 科技建筑 |
| 12 | 矿石精炼厂 |
| 13 | 心灵控制者 |
| 14 | 空中单位(包括着陆的) |
| 15 | 海上目标 |
| 16 | 干扰者 |
| 17 | 地面车辆 |
| 18 | 经济单位 |
| 19 | 兵营 |
| 20 | 战车工厂 |
| 21 | 机场 |
| 22 | 雷达 |
| 23 | 科技实验室 |
| 24 | 船坞 |
| 25 | 超级武器 |
| 26 | 建造厂 |
| 27 | 中立 |
| 28 | 发生器 |
| 29 | 雷达干扰器 |
| 30 | 屏蔽器 |
| 31 | 海军单位 |
| 32 | 机动单位 |
| 33 | 可占领目标 |
| 34 | 区域威胁 |
| 35 | 战车工厂与海军船坞 |
| 36 | 非防御建筑 |
| 37 | 桥梁维修小屋 |

---

### `[Operator]`

| ID | 选项 | 说明 |
|---|---|---|
| 0 | 赋值 (=)  |   A=B |
| 1 | 加 (+)    |   A=A+B |
| 2 | 减 (-)    |   A=A-B |
| 3 | 乘 (*)    |   A=A*B |
| 4 | 除 (/)    |   A=A/B |
| 5 | 求余 (%)  |   A=A%B |
| 6 | 左移 (<<) |   A=A<<B |
| 7 | 右移 (>>) |   A=A>>B |
| 8 | 反转 (~)  |   A=~A |
| 9 | 异或  |   A=A⊕B |
| 10 | 或   |   A=A\|B |
| 11 | 与   |   A=A&B |

---

### `[RadarEvents]`

| ID | 选项 |
|---|---|
| 0 | 战斗 |
| 1 | 非战斗 |
| 2 | 空降区 |
| 3 | 基地受击 |
| 4 | 矿车受击 |
| 5 | 发现敌军 |
| 6 | 单位生产 |
| 7 | 单位阵亡 |
| 8 | 单位维修 |
| 9 | 建筑被渗透 |
| 10 | 建筑被占领 |
| 11 | 信标放置 |
| 12 | 发现超武 |
| 13 | 超武启动 |
| 14 | 桥梁维修 |
| 15 | 放弃驻扎 |
| 16 | 友军受击 |

---

### `[EVAType]`

| ID | 选项 |
|---|---|
| -1 | 禁用 |
| 0 | 盟军(伊娃) |
| 1 | 苏军(索菲亚) |
| 2 | 尤里 |

---

### `[SebarNum]`

| ID | 选项 |
|---|---|
| 0 | 建筑栏 |
| 1 | 防御栏 |
| 2 | 步兵栏 |
| 3 | 战车栏 |

---


### `[CrateTypes]`

| ID | 选项 |
|---|---|
| 0 | 大量金钱(固定值) |
| 1 | 随机单位 |
| 2 | 医疗 |
| 3 | 隐形 |
| 4 | 爆炸 |
| 5 | 大型爆炸 |
| 6 | 金钱(随机) |
| 7 | 重置黑幕 |
| 8 | 显示全图 |
| 9 | 防御力强化 |
| 10 | 速度强化 |
| 11 | 火力强化 |
| 12 | 一次核弹 |
| 13 | 空 |
| 14 | 升级 |
| 15 | 空 |
| 16 | 毒气 |
| 17 | 一片矿 |

---

### `[SpotlightOptions]`

| ID | 选项 |
|---|---|
| 0 | 无聚光灯 |
| 1 | 使用rules设置 |
| 2 | 圆圈 |
| 3 | 跟随敌对目标 |

---

### `[CameraSpeed]`

| ID | 选项 |
|---|---|
| 0 | 非常慢 |
| 1 | 慢 |
| 2 | 正常 |
| 3 | 快 |
| 4 | 非常快 |

---

### `[Boolean]` Boolean(布尔值)

| ID | 选项 |
|---|---|
| 0 | 否 |
| 1 | 是 |

---

### `[EnterStatus]`

| ID | 选项 |
|---|---|
| 0 | ※Sleep (休眠，不还击) |
| 1 | Attack nearest enemy |
| 2 | Move |
| 3 | QMove |
| 4 | Retreat (撤离地图) |
| 5 | ※Guard (原地警戒) |
| 6 | Sticky (固守，不主动攻击) |
| 7 | Enter object |
| 8 | Capture object |
| 9 | Move into & get eaten |
| 10 | Harvest (采矿) |
| 11 | ※Area Guard (区域警戒) |
| 12 | Return (to refinery) |
| 13 | Stop |
| 14 | Ambush (wait until discovered) |
| 15 | ※Hunt (游猎) |
| 16 | ※Unload (卸载或部署) |
| 17 | Sabotage (move in & destroy) |
| 18 | Construction |
| 19 | Deconstruction |
| 20 | Repair |
| 21 | Rescue |
| 22 | Missile |
| 23 | ※Harmless (无威胁) |
| 24 | Open |
| 25 | Patrol |
| 26 | Paradrop approach drop zone |
| 27 | Paradrop overlay drop zone |
| 28 | Wait |
| 29 | Attack again |
| 30 | Spyplane approach (YR) |
| 31 | Spyplane overfly (YR) |

---

### `[Facing]`

| ID | 选项 |
|---|---|
| 0 | ↗ |
| 1 | → |
| 2 | ↘ |
| 3 | ↓ |
| 4 | ↙ |
| 5 | ← |
| 6 | ↖ |
| 7 | ↑ |

---

### `[UnloadOptions]`

| ID | 选项 |
|---|---|
| 0 | 保留载具，保留成员 |
| 1 | 保留载具，丢弃成员 |
| 2 | 丢弃载具，保留成员 |
| 3 | 丢弃载具，丢弃成员 |

---

### `[AttackTargets]`

| ID | 选项 |
|---|---|
| 0 | 任意目标 |
| 1 | 任意目标(同0) |
| 2 | 建筑物 |
| 3 | 矿车矿场 |
| 4 | 步兵 |
| 5 | 车辆 |
| 6 | 生产建筑 |
| 7 | 防御建筑 |
| 8 | 基地威胁 |
| 9 | 电厂 |
| 10 | 驻军建筑 |
| 11 | 科技建筑 |

---

### `[BuildingwithProperty]`

| ID | 选项 |
|---|---|
| 0 | 最小威胁 |
| 1 | 最大威胁 |
| 2 | 最近 |
| 3 | 最远 |

---

### `[BuildingwithPropertyID]`

| ID | 选项 | 说明 |
|---|---|---|
| 0 | LeastThreat | 最小威胁 |
| 1 | HighestThreat | 最大威胁 |
| 2 | Nearest | 最近 |
| 3 | Farthest | 最远 |

---

### `[TalkBubble]`

| ID | 选项 |
|---|---|
| 1 | 星号(*) |
| 2 | 问号(?) |
| 3 | 感叹号(!) |

---

### `[IdleAnim]`

| ID | 选项 |
|---|---|
| 0 | Idle1 |
| 1 | Idle2 |

---

### `[MoveCompletion]`

| ID | 选项 |
|---|---|
| 0 | 小队队长抵达 |
| 1 | 任一成员抵达 |
| 2 | 所有成员抵达 |

---

### `[PlayBuildup]`

| ID | 选项 |
|---|---|
| 0 | 播放 |
| 1 | 不播放 |

---

### `[RadarMode]`

| ID | 选项 |
|---|---|
| 0 | 默认，需要有足够电力供应与雷达建筑 |
| 1 | 无偿雷达 |
| 2 | 永久开启，无视雷达干扰效果 |
| 3 | 永久关闭 |

---

### `[Chinese-ForceOnlyTargetHouseEnemy]`

| ID | 选项 |
|---|---|
| -1 | 默认值，不强制更改 OnlyTargetHouseEnemy 标签全局值 |
| 0 | 强制 OnlyTargetHouseEnemy 为 false |
| 1 | 强制 OnlyTargetHouseEnemy 为 true |
| 2 | 强制随机布尔值 |

---

## rulesmd.ini + 地图文件
以下小节 `load_from` 参数取值为 `rules+map`, 从规则文件和地图文件下读取

### `[Countries]`

| 索引 | 所属方 | 说明 |
|---|---|---|
| 0 | Americans | 美国 |
| 1 | Alliance | 韩国 |
| 2 | French | 法国 |
| 3 | Germans | 德国 |
| 4 | British | 英国 |
| 5 | Africans | 利比亚 |
| 6 | Arabs | 伊拉克 |
| 7 | Confederation | 古巴 |
| 8 | Russians | 苏联 |
| 9 | YuriCountry | 尤里 |
| 10 | Neutral | 中立 |
| 11 | Special | 治安 |
| ... | ... | ... |

---

### `[BuildingTypes]`

| 索引 | 名称 |
|---|---|
| 0 | GAPOWR |
| 1 | GAREFN |
| 2 | GACNST |
| 3 | GAPILE |
| 4 | GASAND |
| 5 | GADEPT |
| 6 | GATECH |
| ... | ... |

---

### `[VehicleTypes]`

| 索引 | 名称 |
|---|---|
| 0 | AMCV |
| 1 | HARV |
| 2 | APOC |
| 3 | HTNK |
| 4 | SAPC |
| 5 | CAR |
| 6 | BUS |
| ... | ... |

---

### `[InfantryTypes]`

| 索引 | 名称 |
|---|---|
| 0 | E1 |
| 1 | E2 |
| 2 | SHK |
| 3 | ENGINEER |
| 4 | JUMPJET |
| 5 | GHOST |
| 6 | YURI |
| ... | ... |

---

### `[AircraftTypes]`

| 索引 | 名称 |
|---|---|
| 0 | APACHE |
| 1 | ORCA |
| 2 | HORNET |
| 3 | V3ROCKET |
| 4 | ASW |
| 5 | DMISL |
| 6 | PDPLANE |
| 7 | BEAG |
| 8 | CARGOPLANE |
| 9 | BPLN |
| 10 | SPYP |
| 11 | CMISL |
| ... | ... |

---

### 科技类型

指[BuildingTypes], [VehicleTypes], [InfantryTypes],  [AircraftTypes]的名称, 无索引, 使用时一般仅使用字符串

---

### `[SuperWeaponTypes]`

| ID | 名称 |
|---|---|
| 0 | NukeSpecial |
| 1 | IronCurtainSpecial |
| 2 | LightningStormSpecial |
| 3 | ChronoSphereSpecial |
| 4 | ChronoSphereSpecial |
| 5 | ChronoWarpSpecial |
| 6 | ParaDropSpecial |
| 7 | AmericanParaDropSpecial |
| 8 | PsychicDominatorSpecial |
| 9 | SpyPlaneSpecial |
| 10 | GeneticConverterSpecial |
| 11 | ForceShieldSpecial |
| 12 | PsychicRevealSpecial |
| ... | ... |

---

### `[VoxelAnims]`

| ID | 名称 |
|---|---|
| 1 | PIECE |
| 2 | TIRE |
| 3 | GASTANK |
| 4 | SONICTURRET |
| 5 | 4TNKTURRET |
| 6 | CRYSTAL01 |
| 7 | CRYSTAL02 |
| 8 | METEOR01 |
| 9 | METEOR02 |
| 10 | PEBBLE |
| ... | ... |

---

### `[ParticleSystems]`

| ID | 名称 |
|---|---|
| 1 | GasCloudSys |
| 2 | FireStreamSys |
| 3 | BigGreySmokeSys |
| 4 | SmallGreySSys |
| 5 | DebrisSmokeSys |
| 6 | SparkSys |
| 7 | FirestormSparkSys |
| 8 | TestSmokeSys |
| 9 | SmallRailgunSys |
| 10 | LargeRailgunSys |
| 11 | WeldingSys |
| 12 | LGSparkSys |
| 13 | PsychCloudSys |
| ... | ... |

---

### `[AttachEffect]`
完全由用户自定义, 必须手动遍历查找

---

### `[BannerType]`
完全由用户自定义, 必须手动遍历查找

---


## 地图文件
以下小节 `load_from` 参数取值为 `map`, 从当前地图文件下读取
以下内容必须实时读取, 该处仅为举例

### `[TeamTypes]`

| 索引 | 名称 |
|---|---|
| 0 | 01000001 |
| 1 | 01000003 |
| 2 | 01000008 |
| ... | ... |

---

### `[Triggers]`    
内容完全由用户自定义, 必须手动遍历查找

---

### `[Tags]`    
内容完全由用户自定义, 必须手动遍历查找

---

### `[VariableNames]` 局部变量
该小解如果在`rulesmd.ini`文件里面就是全局变量, `map`文件里面就是局部变量    
内容完全由用户自定义, 必须手动遍历查找

---

### `[Waypoints]`

RA2的路径点存储格式为:   

```ini
[Waypoints]
序号=YX
```

其中Y为前两（或三）位数, 而X为后三位数，例如:   

```ini
[Waypoints]
98=80080   
99=79080
```
例中99路径点对应的Y为79，99路径点对应的X为080，即80。右上角Y最小，为1；左上角X最小，为1。以此类推。

---
## rulesmd.ini
以下小节 `load_from` 参数取值为 `rules`, 从当前地图文件下读取

### `[VariableNames]` 全局变量
该小解如果在`rulesmd.ini`文件里面就是全局变量, `map`文件里面就是局部变量

| 索引 | 名称 |
|---|---|
| 0 | \<Alternate Start Location\> |
| 1 | \<Alternate Next Scenario\> |
| 2 | \<reserved2\> |
| 3 | Smithsonian Destroyed |
| 4 | Lincoln Destroyed |
| 5 | Jefferson Destroyed |
| 6 | Washington Destroyed |
| 7 | SmithCastle Destroyed |
| 8 | Completed 3B |
| 9 | Prisoners Freed |
| 10 | Train Stolen |
| 11 | Completed 9B |
| 12 | Machineshop |
| 13 | Hospital |
| ... | ... |

---

### `[AITargetTypes]`
内容完全由用户自定义, 必须手动遍历查找

---

### `[AIScriptsList]`
内容完全由用户自定义, 必须手动遍历查找

---

### `[Animations]`

| 索引 | 名称 |
|---|---|
| 0 | TWLT100 |
| 1 | ELECTRO |
| 2 | H2O_EXP1 |
| 3 | H2O_EXP2 |
| ... | ... |

---

### `[WeaponTypes]`
内容完全由用户自定义, 必须手动遍历查找

---


## 	soundmd.ini
以下小节 `load_from` 参数取值为 `sound`, 从声音文件文件中读取

### `[SoundList]`

| 索引 | 名称 |
|---|---|
| 0 | ChronoLegionAttack |
| 1 | ChronoLegionAttackCommand |
| 2 | ChronoLegionMove |
| 3 | ChronoLegionSelect |
| 4 | ChronoLegionFear |
| 5 | ChronoLegionDie |
| ... | ... |

---

## 	thememd.ini
以下小节 `load_from` 参数取值为 `theme`, 从声音文件文件中读取

### `[Themes]`

| 索引 | 名称 |
|---|---|
| 0 | INTRO |
| 1 | SCORE |
| 2 | LOADING |
| 3 | CREDITS |
| 4 | RA2Options |
| ... | ... |

---

## 	artmd.ini
以下小节 `load_from` 参数取值为 `art`, 从美术文件文件中读取

### `[Movies]`

| 索引 | 名称 |
|---|---|
| 0 | A00_F00e |
| 1 | A00_F01e |
| 2 | A00_F02e |
| 3 | A00_F03e |
| 4 | A00_F04e |
| 5 | A00_F05e |
| 6 | A00_F06e |
| 7 | A00_F07e |
| ... | ... |

---

## 	evamd.ini
以下小节 `load_from` 参数取值为 `eva`, 从副官语音文件文件中读取

### `[DialogList]`

| 索引 | 名称 |
|---|---|
| 0 | EVA_NuclearSiloDetected |
| 1 | EVA_NuclearMissileLaunched |
| 2 | EVA_NuclearMissileReady |
| 3 | EVA_IronCurtainDetected |
| 4 | EVA_IronCurtainActivated |
| 5 | EVA_IronCurtainReady |
| 6 | EVA_ChronosphereDetected |
| 7 | EVA_ChronosphereActivated |
| ... | ... |

---


