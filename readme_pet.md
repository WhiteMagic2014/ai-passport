# 换宠物 / 给图命名

宠物资源是**数据驱动**的：美术把 PNG 丢进 `pets/`，跑一条脚本，固件里就多了宠物。
支持**多宠物同时运动**：每只宠物在 `pets/` 中用不同的宠物名前缀区分，自动生成到各自子目录。
正常情况下**不用改任何 C 代码**（唯一例外见下文「重新构建」）。

## 目录

```
pets/                          ← 只动这里（美术原图，AI 出的像素图）
  cat_walk_1_r_moveforward.png ← 第一段是宠物名 (cat)
  cat_jump_2_l_sprintup.png
  cat_sleep_1_r_idle.png
  devon_walk_1_r_moveforward.png  ← 另一只宠物 (devon)
  devon_sleep_2_r_idle.png
  background.png                ← 可选：整屏背景（240x320），所有宠物共享

tools/
  png2lvgl.py                   ← 单张 PNG → LVGL9 C 数组（抠背景 / 方向归一 / 整图缩放，不裁切）
  prep_pet.py                   ← 批量调用上面，按宠物名分组，汇总生成 pet_manifest.h

main/pet/                       ← 自动生成，勿手改
  cat/                          ← 每只宠物一个子目录
    pet_cat_walk_1.c/.h ...
    pet_cat_jump_1.c/.h ...
  devon/
    pet_devon_walk_1.c/.h ...
    pet_devon_sleep_1.c/.h ...
  pet_bg.c/.h                   ← 仅当 pets/background.png 存在时生成
  pet_manifest.h                ← 行为总表 + 宠物注册表 (demo 运行时只读这张)
```

换宠物 = 替换 `pets/` 里的图 → 跑脚本 → 重新编译。

## 命名

```
<宠物名>_<动作>_<第几帧>_<朝向>_<运动状态>.png
```

- **宠物名**：第一段，决定宠物身份。同名前缀的帧归为同一只宠物，输出到 `main/pet/{宠物名}/` 子目录。
- **动作**：分组和显示名，不决定行为。`walk` `jump` `sleep` `groom` `sit` `peck` `fly` 都行。
- **第几帧**：从 1 开始的整数。
- **朝向**：表示这个图面向哪边 `r` 朝右，`l` 朝左（**l 会自动水平镜像，不用画两张**）。
- **运动状态**：必填，只描述"画面怎么动"，跟动物无关：
  `idle` 不动 / `moveforward` 慢走 / `sprintforward` 快走 / `moveup` 小跳 / `sprintup` 大跳。
  **文件名缺这个字段会直接报错退出。**

示例：

```
cat_walk_1_r_moveforward.png       黑猫走路第1帧，朝右，水平慢移
cat_jump_3_l_sprintup.png          黑猫跳跃第3帧，朝左(自动镜像)，垂直大弧
devon_sleep_2_r_idle.png           德文猫睡觉第2帧，原地循环
devon_groom_1_r_idle.png           德文猫舔毛第1帧，原地循环
```

**为什么动作名不决定行为**：猫"舔毛"标 `idle`、小鸡"啄米"也标 `idle`，代码只认 `idle`（原地）。
加小熊时 `bear_sit_1_r_idle.png` 自然就是原地，零改动。

运动状态的物理参数（想调速度 / 跳多高，改 `tools/png2lvgl.py` 的 `MOTION_PRESETS` 一处即可）：

| 字段 | 怎么动 | 水平/拍 | 垂直 | 换帧 | 静止 |
|---|---|---|---|---|---|
| `idle` | 不动 | 0 | 0 | 350ms | 3000ms 后转身 |
| `moveforward` | 水平慢移 | 1px | 0 | 130ms | 走到撞墙 |
| `sprintforward` | 水平快移 | 3px | 0 | 90ms | 走到撞墙 |
| `moveup` | 小跳 | 1px | 6px | 120ms | 走到撞墙 |
| `sprintup` | 大跳 | 2px | 12px | 110ms | 走到撞墙 |

## 多宠物

`pets/` 里放多个宠物名前缀的图，`prep_pet.py` 自动按宠物名分组，生成各自的子目录和动作表。
`pet_manifest.h` 顶层 `pet_defs[]` 注册表列出所有宠物，`demo_pet.c` 启动时遍历注册表创建实例。

- 每只宠物有独立的 LVGL 图片对象、状态机、翻转缓冲区。
- 多只宠物均匀分布在屏幕水平范围，交替朝右/朝左。
- 短按按键：所有宠物同时切换到下一个动作（方便逐个查看美术资源）。
- 最大同时运动宠物数由 `MAX_PETS`（`demo_pet.c`，默认 4）控制。

**内存约束**：每只宠物的翻转缓冲区占用 `.bss` 静态 RAM，大小 = `PET_SPRITE_W × PET_SPRITE_H × 4` 字节。
64×64 下每只 16KB，4 只 = 64KB。128×128 下每只 64KB，需减少 `MAX_PETS` 或改用 64×64。
ESP32-C3 无 PSRAM，总 DRAM 约 314KB，请控制宠物数量和 sprite 尺寸。

## 抠背景

AI 出的图一般是纯色底，脚本按色键自动抠成透明，你不用自己擦。
默认抠除色为蓝 `#1900FF`（见 `tools/prep_pet.py` 的 `--key-color`），换底色就传 `--key-color`：

```bash
python3 tools/prep_pet.py --key-color "#00FF00"   # 绿底就用绿
```

## 不裁切：小猫位置由你画的位置决定

**工具不再做任何裁切或自动居中**——整张源图直接等比缩放到基准分辨率（默认 64×64）。
因此小猫在 PNG 里的位置会被 1:1 保留到宠物里：

- 脚画在源图**底部** → 烧录后脚踩草地（落地）。
- 脚画在源图**中间** → 悬空。
- 浮空还是落地，**完全看你把猫画在哪**。

⚠️ 重要约束：既然工具不自动对齐，请**保证所有帧的小猫脚都画在源图同一条水平线上**（建议贴 128 画布底边）。
否则 walk 猫落地、sleep 猫悬空会不一致——这是「浮空/落地由图像决定」的代价，也是你想要的美术控制权。

## 背景图（可选）

`pets/` 放一张 `background.png`（建议 240×320，像素风）就会被生成成 `pet_bg`，
画在标题牌和草地下面，所有宠物共享。 没有 `background.png` 就不生成、不占空间，页面用默认天空+草地。

## 添加新宠物

0. 清理旧资源（可选，如果要重新生成全部）：
   把 `main/pet/` 中宠物子目录删除（由脚本生成，**勿手改**）。

1. 往 `pets/` 放图，文件名第一段为宠物名：
   ```
   dog_walk_1_r_moveforward.png   dog_walk_2_r_moveforward.png
   dog_peck_1_r_idle.png          dog_peck_2_r_idle.png
   dog_fly_1_r_sprintup.png       dog_fly_2_r_sprintup.png
   background.png                 # 可选
   ```
2. 跑脚本（项目 Python 环境需已装 Pillow）：
   ```bash
   python3 tools/prep_pet.py
   ```
3. **新增了 `.c` 文件要重新扫描构建缓存**（CMake 用 `file(GLOB)` 收集源文件，缓存了旧列表）：
   ```bash
   source "$IDF_PATH/export.sh"        # 先 export IDF_PATH=/path/to/esp-idf
   idf.py reconfigure
   idf.py build
   ```
   如果只是**覆盖**已有文件（不改数量），直接 `idf.py build` 即可。
4. 烧录：`idf.py flash monitor`

页面自动出现新宠物，与已有宠物同时在屏幕上运动，`demo_pet.c` 一行都不用动。

## 调大小：生成尺寸 vs 显示尺寸

- **生成尺寸** `--size`（默认 64）：宠物的基准分辨率，决定资源体积与清晰度。
- **显示尺寸** `PET_SCALE`（`main/demo_pet.c`，当前 460 ≈ 1.8x）：屏上实际放大倍数，
  64×1.8≈115px。想整体变大变小，**只改 `PET_SCALE` 一个数即可**，无需重生成资源。

```bash
python3 tools/prep_pet.py --size 80          # 想要更清晰的基准（重生成后才生效）
```

## 命令速查

```bash
# 生成资源（默认读 pets/ → 写 main/pet/{宠物名}/；基准 64x64，格式 rgb565a8）
python3 tools/prep_pet.py
python3 tools/prep_pet.py --size 80 --key-color "#00FF00"   # 自定义基准尺寸 / 抠色

# 进入 ESP-IDF 环境并构建（先 export IDF_PATH=/path/to/esp-idf）
source "$IDF_PATH/export.sh"
idf.py set-target esp32c3
idf.py reconfigure && idf.py build
idf.py flash monitor
```
