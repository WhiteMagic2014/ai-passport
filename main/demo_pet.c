// main/demo_pet.c —— 数据驱动的多宠物 demo
//
// 行为 (动作/帧/速度/跳跃) 全部来自 main/pet/pet_manifest.h,
// 由 tools/prep_pet.py 从 pets/ 的 PNG 自动汇总生成, 本文件不含任何动作硬编码。
// 支持多宠物同时运动: 每只宠物有独立的 LVGL 图片对象、状态机和翻转缓冲区。
// 命名: <pet>_<action>_<frame>_<dir>_<motion>.png → 输出到 main/pet/{pet}/
#include "demo.h"
#include "bsp_display.h"
#include "ui_pixel.h"
#include "pet_manifest.h"
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

/* ===== 显示 ===== */
#define PET_SCALE      460                                   /* 256=100% → 460 ≈ 1.8x */
#define PET_DRAW_W     ((PET_SPRITE_W * PET_SCALE) / 256)    /* 64*1.8 ≈ 115 */
#define PET_DRAW_H     ((PET_SPRITE_H * PET_SCALE) / 256)
#define GROUND_Y       286                                   /* 草地顶面 */
#define PET_Y_BASE     (GROUND_Y - PET_DRAW_H)
#define WALK_MIN       2
#define WALK_MAX       (240 - PET_DRAW_W - 2)

#define TICK_MS        40                                    /* 主循环节拍 */

/* ===== 多宠物配置 ===== */
#ifndef MAX_PETS
#define MAX_PETS       4                                     /* 同时运动的最大宠物数 */
#endif

#if PET_SPRITE_W * PET_SPRITE_H * 4 * MAX_PETS > 200000
#warning "宠物翻转缓冲区可能超出 .bss 预算, 考虑减小 sprite 尺寸或减少宠物数"
#endif

/* ===== 运行时实例 ===== */
typedef struct {
    lv_obj_t         *img;           /* LVGL 图片对象 */
    const pet_def_t  *def;          /* 宠物定义 (动作表/池) */
    uint8_t           act;          /* 当前动作下标 */
    uint8_t           frame;        /* 当前帧下标 */
    uint16_t          frame_acc;    /* 换帧累加器 (ms) */
    uint16_t          hold_acc;     /* 静止动作持续累加器 (ms) */
    int16_t           x;            /* 视觉 x 坐标 */
    int8_t            dir;          /* +1 右 / -1 左 */
    bool              face_right;
    uint8_t           drawn_act;    /* 上次绘制的动作 (避免重复) */
    uint8_t           drawn_frame;
    bool              drawn_face;
    uint8_t           flip_idx;     /* 翻转缓冲区下标 */
} pet_instance_t;

static lv_obj_t   *s_scr;
static lv_timer_t *s_tick;
static pet_instance_t s_pets[MAX_PETS];
static uint8_t s_pet_count = 0;

/* 朝左时按行水平镜像到静态 RAM 缓冲 (零 malloc, 适配无 PSRAM 设备)。
 * 每只宠物一个缓冲区, timer 顺序处理不会并发。 */
static uint8_t       s_flip_data[MAX_PETS][PET_SPRITE_W * PET_SPRITE_H * 4] __attribute__((aligned(4)));
static lv_draw_buf_t s_flip_dbs[MAX_PETS];

/* ===== 图片翻转 ===== */
static const lv_image_dsc_t *pet_resolve(pet_instance_t *p,
                                          const lv_image_dsc_t *frm, bool right)
{
    if (right) return frm;
    uint32_t w   = frm->header.w;
    uint32_t h   = frm->header.h;
    uint32_t cf  = frm->header.cf;
    const uint8_t *s = frm->data;
    uint8_t *d = s_flip_dbs[p->flip_idx].data;
    if (cf == LV_COLOR_FORMAT_RGB565A8) {
        uint32_t cstride = w * 2;
        uint32_t aoff    = cstride * h;
        for (uint32_t y = 0; y < h; y++) {
            for (uint32_t x = 0; x < w; x++) {
                const uint8_t *sc = s + y * cstride + x * 2;
                uint8_t *dc = d + y * cstride + (w - 1 - x) * 2;
                dc[0] = sc[0]; dc[1] = sc[1];
                const uint8_t *sa = s + aoff + y * w + x;
                uint8_t *da = d + aoff + y * w + (w - 1 - x);
                *da = *sa;
            }
        }
    } else {
        uint32_t bpp = (cf == LV_COLOR_FORMAT_RGB565) ? 2u : 4u;
        for (uint32_t y = 0; y < h; y++) {
            for (uint32_t x = 0; x < w; x++) {
                const uint8_t *sp = s + (y * w + x) * bpp;
                uint8_t *dp = d + (y * w + (w - 1 - x)) * bpp;
                for (uint32_t c = 0; c < bpp; c++) dp[c] = sp[c];
            }
        }
    }
    return (const lv_image_dsc_t *)&s_flip_dbs[p->flip_idx];
}

static void pet_draw(pet_instance_t *p)
{
    if (p->act == p->drawn_act && p->frame == p->drawn_frame &&
        p->face_right == p->drawn_face) {
        return;
    }
    lv_image_set_src(p->img, pet_resolve(p, p->def->actions[p->act].frames[p->frame],
                                          p->face_right));
    p->drawn_act   = p->act;
    p->drawn_frame = p->frame;
    p->drawn_face  = p->face_right;
}

/* 取一个随机静止动作 (休息用); 没有则返回 0xFF */
static uint8_t pick_rest(const pet_def_t *def)
{
    if (def->rest_n == 0) return 0xFF;
    return def->rest_pool[lv_rand(0, def->rest_n - 1)];
}

/* 取一个随机移动动作, 尽量不同于 cur; 没有则返回 0xFF */
static uint8_t pick_move(const pet_def_t *def, uint8_t cur)
{
    if (def->move_n == 0) return 0xFF;
    if (def->move_n == 1) return def->move_pool[0];
    uint8_t i;
    do { i = def->move_pool[lv_rand(0, def->move_n - 1)]; } while (i == cur);
    return i;
}

static void enter_act(pet_instance_t *p, uint8_t act)
{
    p->act       = act;
    p->frame     = 0;
    p->frame_acc = 0;
    p->hold_acc  = 0;
    lv_obj_set_y(p->img, PET_Y_BASE);
    pet_draw(p);
}

/* 起点: 优先移动动作, 否则第 0 个 */
static uint8_t start_act(const pet_def_t *def)
{
    return (def->move_n > 0) ? def->move_pool[0] : 0;
}

/* ===== 主节拍: 遍历所有宠物 ===== */
static void tick_cb(lv_timer_t *t)
{
    (void)t;
    for (uint8_t i = 0; i < s_pet_count; i++) {
        pet_instance_t *p = &s_pets[i];
        const pet_action_t *def = &p->def->actions[p->act];

        /* 1) 换帧 */
        p->frame_acc += TICK_MS;
        if (p->frame_acc >= def->frame_ms) {
            p->frame_acc = 0;
            p->frame = (uint8_t)((p->frame + 1) % def->n_frames);
            pet_draw(p);
        }

        /* 2) 移动 / 静止 */
        if (!def->stationary) {
            p->x += (int16_t)(p->dir * (int8_t)def->step_px);
            if (p->x <= WALK_MIN)      { p->x = WALK_MIN;  uint8_t r = pick_rest(p->def); if (r != 0xFF) enter_act(p, r); else { p->dir = 1;  pet_draw(p); } }
            else if (p->x >= WALK_MAX) { p->x = WALK_MAX; uint8_t r = pick_rest(p->def); if (r != 0xFF) enter_act(p, r); else { p->dir = -1; pet_draw(p); } }
            lv_obj_set_x(p->img, p->x);

            if (p->def->move_n > 1 && lv_rand(0, 299) == 0) {
                uint8_t m = pick_move(p->def, p->act);
                if (m != 0xFF) enter_act(p, m);
            }
        } else {
            p->hold_acc += TICK_MS;
            if (p->hold_acc >= def->hold_ms) {
                p->dir = (int8_t)-p->dir;
                p->face_right = (p->dir > 0);
                pet_draw(p);
                uint8_t m = pick_move(p->def, p->act);
                enter_act(p, (m != 0xFF) ? m : p->act);
                continue;
            }
        }

        /* 3) 垂直跳跃 (通用抛物线) */
        if (def->hop_max > 0) {
            uint8_t n = def->n_frames;
            int32_t dy;
            if (n > 1) {
                int32_t f   = p->frame;
                int32_t num = f * (n - 1 - f);
                int32_t den = (int32_t)(n - 1) * (n - 1);
                dy = -((int32_t)def->hop_max * 4 * num) / den;
            } else {
                dy = 0;
            }
            lv_obj_set_y(p->img, PET_Y_BASE + (int16_t)dy);
        }
    }
}

/* ===== 生命周期 ===== */
void demo_pet_enter(void)
{
    s_scr = ui_pixel_screen_create("PET");

#if PET_HAS_BG
    lv_obj_t *bg = lv_image_create(s_scr);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_size(bg, 240, 320);
    lv_image_set_src(bg, &pet_bg);
    lv_obj_move_to_index(bg, 0);
#endif

    s_pet_count = (PET_DEFS_COUNT < MAX_PETS) ? PET_DEFS_COUNT : MAX_PETS;

    /* 将宠物均匀分布在屏幕水平范围 */
    int16_t range = WALK_MAX - WALK_MIN;
    int16_t step = (s_pet_count > 1) ? (range / (s_pet_count)) : 0;

    for (uint8_t i = 0; i < s_pet_count; i++) {
        pet_instance_t *p = &s_pets[i];
        p->def = &pet_defs[i];
        p->flip_idx = i;

        /* 初始化翻转缓冲 (格式/stride 跟随宠物) */
        lv_color_format_t flip_cf = p->def->actions[0].frames[0]->header.cf;
        uint32_t flip_stride = lv_draw_buf_width_to_stride(PET_SPRITE_W, flip_cf);
        lv_draw_buf_init(&s_flip_dbs[i], PET_SPRITE_W, PET_SPRITE_H,
                         flip_cf, flip_stride,
                         s_flip_data[i], sizeof(s_flip_data[i]));

        /* 创建 LVGL 图片对象 */
        p->img = lv_image_create(s_scr);
        lv_obj_set_size(p->img, PET_DRAW_W, PET_DRAW_H);
        lv_image_set_inner_align(p->img, LV_IMAGE_ALIGN_CENTER);
        lv_image_set_scale(p->img, PET_SCALE);

        /* 初始位置和方向: 交替朝右/朝左, 均匀分布 */
        p->x = WALK_MIN + (int16_t)(i * step);
        if (i % 2 == 0) {
            p->dir = 1;
            p->face_right = true;
        } else {
            p->dir = -1;
            p->face_right = false;
        }
        p->drawn_act   = 0xFF;
        p->drawn_frame = 0xFF;
        p->drawn_face  = true;
        lv_obj_set_pos(p->img, p->x, PET_Y_BASE);

        enter_act(p, start_act(p->def));
    }

    s_tick = lv_timer_create(tick_cb, TICK_MS, NULL);
    lv_screen_load(s_scr);
}

void demo_pet_exit(void)
{
    if (s_tick) { lv_timer_delete(s_tick); s_tick = NULL; }
    if (s_scr)  { lv_obj_delete(s_scr);    s_scr  = NULL; }
    for (uint8_t i = 0; i < s_pet_count; i++) {
        s_pets[i].img = NULL;
    }
    s_pet_count = 0;
}

void demo_pet_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    /* 短按任意键: 所有宠物切换到下一个动作, 方便逐个查看美术资源 */
    if (ev != BSP_BTN_CLICK) return;
    (void)btn;
    for (uint8_t i = 0; i < s_pet_count; i++) {
        pet_instance_t *p = &s_pets[i];
        uint8_t next = (uint8_t)((p->act + 1) % p->def->act_count);
        enter_act(p, next);
    }
}
