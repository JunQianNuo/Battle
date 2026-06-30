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

## 已知问题
- [ ] **敌人跑步动画**：动画蓝图（ABP_Unarmed/ABP_TP_Pistol）未正确响应 CharacterMovement 速度，敌人移动时播放 idle 而非 locomotion。可能原因：AnimBP 的 StateMachine 配置不完整，或需要在 AnimInstance 中设置 C++ 暴露的速度变量。

## 未开始
- [ ] 主菜单
- [ ] 多人联机（Listen Server）


