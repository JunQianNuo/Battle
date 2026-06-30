# Battle Arena - TODO

## 已完成
- [x] 敌人波次系统（3种类型，随机生成）
- [x] 敌人 AI 移动 + 攻击（C++ Tick 驱动）
- [x] 玩家射击 + 武器拾取 + F键近战
- [x] 计分系统 + 死亡惩罚 + HUD 显示
- [x] 关卡 GameMode 绑定（Lvl_Shooter）
- [x] NavMesh 生成
- [x] 弹簧→斜坡替换
- [x] 死亡扣分：-50 → -500（PS和GS统一扣500）
- [x] 虚空掉落死亡：敌人 Z < -1000 自动死亡，防止波次卡关
- [x] 胜利条件：存活3波→结算（取代原来分数制）
- [x] 出生点验证：刷怪位置通过 ProjectPointToNavigation 投影到 NavMesh 上
- [x] **敌人跑步动画**：创建 C++ UEnemyAnimInstance 暴露 Speed/IsMoving/HP 给动画蓝图。需在编辑器中把 ABP_Unarmed/ABP_TP_Pistol 重新父类到 UEnemyAnimInstance 并在状态机中使用 Speed 变量。
- [x] **主菜单系统**：完善 BattlHUD 菜单（START GAME / HOST GAME / JOIN GAME / QUIT），支持 IP 输入、多人 Lobby、胜利画面
- [x] **多人联机（Listen Server）**：AEnemyBase 网络复制（HP/类型/死亡状态），GameState 同步菜单状态，Host/Join 通过 ServerTravel/ClientTravel，支持局域网联机

## 编辑器手动步骤
1. 重新父类 ABP_Unarmed → UEnemyAnimInstance（打开蓝图 → Class Settings → Parent Class）
2. 重新父类 ABP_TP_Pistol → UEnemyAnimInstance
3. 在动画蓝图状态机中使用 Speed / bIsMoving 变量驱动 locomotion
4. 重新编译 C++ 代码后测试

## 联机测试
- 主机：打开菜单点 HOST GAME，或控制台输入 `host Lvl_Shooter`
- 客户端：打开菜单点 JOIN GAME 输入主机 IP，或控制台输入 `join <ip>`
