/* 由 prep_pet.py 自动生成 —— 勿手改 */
#ifndef PET_MANIFEST_H
#define PET_MANIFEST_H

#include "lvgl.h"

#define PET_HAS_BG  0

#define PET_SPRITE_W  64
#define PET_SPRITE_H  64

typedef enum { MOT_IDLE, MOT_MOVEFORWARD, MOT_SPRINTFORWARD, MOT_MOVEUP, MOT_SPRINTUP } pet_mot_t;

typedef struct {
    const char *name;                        /* 显示标签 (动作名大写) */
    const lv_image_dsc_t *const *frames;    /* 帧指针表 */
    uint8_t  n_frames;
    uint16_t frame_ms;     /* 换帧周期 (ms) */
    uint16_t hold_ms;      /* 静止持续; 0 = 移动到撞墙 */
    pet_mot_t motion;      /* 运动学类型 */
    uint8_t  step_px;      /* 每 tick 水平位移 */
    int8_t   hop_max;      /* 垂直跳跃幅度 (0 = 不跳) */
    bool     stationary;   /* true = 原地不动 */
} pet_action_t;

typedef struct {
    const char *name;            /* 宠物名 */
    const pet_action_t *actions; /* 动作表 */
    uint8_t  act_count;          /* 动作数 */
    const uint8_t *rest_pool;    /* 静止动作下标池 */
    uint8_t  rest_n;             /* 静止池大小 */
    const uint8_t *move_pool;    /* 移动动作下标池 */
    uint8_t  move_n;             /* 移动池大小 */
} pet_def_t;

/* ===== bao ===== */
#include "bao/pet_bao_groom_1.h"
#include "bao/pet_bao_groom_2.h"
#include "bao/pet_bao_groom_3.h"
#include "bao/pet_bao_jump_1.h"
#include "bao/pet_bao_jump_2.h"
#include "bao/pet_bao_jump_3.h"
#include "bao/pet_bao_jump_4.h"
#include "bao/pet_bao_sleep_1.h"
#include "bao/pet_bao_sleep_2.h"
#include "bao/pet_bao_walk_1.h"
#include "bao/pet_bao_walk_2.h"
#include "bao/pet_bao_walk_3.h"

static const lv_image_dsc_t *const bao_groom_frames[] = { &pet_bao_groom_1, &pet_bao_groom_2, &pet_bao_groom_3 };
static const lv_image_dsc_t *const bao_jump_frames[] = { &pet_bao_jump_1, &pet_bao_jump_2, &pet_bao_jump_3, &pet_bao_jump_4 };
static const lv_image_dsc_t *const bao_sleep_frames[] = { &pet_bao_sleep_1, &pet_bao_sleep_2 };
static const lv_image_dsc_t *const bao_walk_frames[] = { &pet_bao_walk_1, &pet_bao_walk_2, &pet_bao_walk_3 };

static const pet_action_t bao_actions[] = {
  /* GROOM */ { "GROOM", bao_groom_frames, 3, 350, 3000, MOT_IDLE, 0, 0, true },
  /* JUMP */ { "JUMP", bao_jump_frames, 4, 110, 0, MOT_SPRINTUP, 2, 12, false },
  /* SLEEP */ { "SLEEP", bao_sleep_frames, 2, 350, 3000, MOT_IDLE, 0, 0, true },
  /* WALK */ { "WALK", bao_walk_frames, 3, 130, 0, MOT_MOVEFORWARD, 1, 0, false },
};
#define BAO_ACT_COUNT  4

#define BAO_HAS_REST  1
static const uint8_t bao_rest_pool[] = { 0, 2 };
#define BAO_REST_N  (2)

#define BAO_HAS_MOVE  1
static const uint8_t bao_move_pool[] = { 1, 3 };
#define BAO_MOVE_N  (2)


/* ===== dage ===== */
#include "dage/pet_dage_groom_1.h"
#include "dage/pet_dage_groom_2.h"
#include "dage/pet_dage_groom_3.h"
#include "dage/pet_dage_jump_1.h"
#include "dage/pet_dage_jump_2.h"
#include "dage/pet_dage_jump_3.h"
#include "dage/pet_dage_jump_4.h"
#include "dage/pet_dage_sleep_1.h"
#include "dage/pet_dage_sleep_2.h"
#include "dage/pet_dage_sleep_3.h"
#include "dage/pet_dage_sleep_4.h"
#include "dage/pet_dage_sleep_5.h"
#include "dage/pet_dage_walk_1.h"
#include "dage/pet_dage_walk_2.h"
#include "dage/pet_dage_walk_3.h"
#include "dage/pet_dage_walk_4.h"

static const lv_image_dsc_t *const dage_groom_frames[] = { &pet_dage_groom_1, &pet_dage_groom_2, &pet_dage_groom_3 };
static const lv_image_dsc_t *const dage_jump_frames[] = { &pet_dage_jump_1, &pet_dage_jump_2, &pet_dage_jump_3, &pet_dage_jump_4 };
static const lv_image_dsc_t *const dage_sleep_frames[] = { &pet_dage_sleep_1, &pet_dage_sleep_2, &pet_dage_sleep_3, &pet_dage_sleep_4, &pet_dage_sleep_5 };
static const lv_image_dsc_t *const dage_walk_frames[] = { &pet_dage_walk_1, &pet_dage_walk_2, &pet_dage_walk_3, &pet_dage_walk_4 };

static const pet_action_t dage_actions[] = {
  /* GROOM */ { "GROOM", dage_groom_frames, 3, 350, 3000, MOT_IDLE, 0, 0, true },
  /* JUMP */ { "JUMP", dage_jump_frames, 4, 110, 0, MOT_SPRINTUP, 2, 12, false },
  /* SLEEP */ { "SLEEP", dage_sleep_frames, 5, 350, 3000, MOT_IDLE, 0, 0, true },
  /* WALK */ { "WALK", dage_walk_frames, 4, 130, 0, MOT_MOVEFORWARD, 1, 0, false },
};
#define DAGE_ACT_COUNT  4

#define DAGE_HAS_REST  1
static const uint8_t dage_rest_pool[] = { 0, 2 };
#define DAGE_REST_N  (2)

#define DAGE_HAS_MOVE  1
static const uint8_t dage_move_pool[] = { 1, 3 };
#define DAGE_MOVE_N  (2)


/* ===== 宠物注册表 ===== */
static const pet_def_t pet_defs[] = {
  { "bao", bao_actions, BAO_ACT_COUNT, bao_rest_pool, BAO_REST_N, bao_move_pool, BAO_MOVE_N },
  { "dage", dage_actions, DAGE_ACT_COUNT, dage_rest_pool, DAGE_REST_N, dage_move_pool, DAGE_MOVE_N },
};

#define PET_DEFS_COUNT  (sizeof(pet_defs) / sizeof(pet_defs[0]))

#endif /* PET_MANIFEST_H */
