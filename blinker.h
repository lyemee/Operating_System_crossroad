#ifndef __PROJECTS_PROJECT2_BLINKER_H__
#define __PROJECTS_PROJECT2_BLINKER_H__

#include "threads/synch.h"
#include "projects/crossroads/vehicle.h"
#include "projects/crossroads/position.h"

/** you can change the number of blinkers */
#define NUM_BLINKER 4

// 기존 호환성을 위한 enum (하위 호환성)
enum blinker_dir
{
    BLINKER_EW,
    BLINKER_NS
};

// 새로운 고급 신호등 시스템
typedef enum
{
    SIGNAL_RED = 0,
    SIGNAL_GREEN = 1
} signal_state_t;

typedef enum
{
    DIR_NORTH = 0,
    DIR_SOUTH = 1,
    DIR_EAST = 2,
    DIR_WEST = 3,
    DIR_COUNT = 4
} direction_t;

// 교차로별 신호등 상태
struct intersection_signal
{
    signal_state_t directions[DIR_COUNT]; // 각 방향별 신호
    int phase;                            // 현재 신호 페이즈
};

// 고급 신호등 시스템
struct advanced_signal_system
{
    struct intersection_signal intersections[7][7]; // 각 위치별 신호등
    int current_phase;                              // 전역 신호 페이즈
    int phase_step_count;                           // 현재 페이즈 지속 스텝
    struct lock signal_lock;                        // 신호 시스템 락
    struct condition signal_cond;                   // 신호 변경 조건변수
};

// 신호 패턴 정의
struct signal_pattern
{
    int phase_id;
    int duration_steps;                      // 페이즈 지속 스텝 수
    signal_state_t pattern[7][7][DIR_COUNT]; // 위치별 방향별 신호
};

struct blinker_info
{
    struct lock **map_locks;
    struct vehicle_info *vehicles;
};

// 기존 함수들 (하위 호환성)
void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info);
void start_blinker();

// 새로운 고급 신호등 함수들
bool can_vehicle_pass_signal(struct position from, struct position to, struct vehicle_info *vehicle);
direction_t get_move_direction(struct position from, struct position to);
bool is_intersection_coord(struct position pos);

// 외부 참조용 전역 변수 (기존 호환성)
extern enum blinker_dir current_blinker_state;
extern struct lock blinker_lock;
extern struct condition blinker_cond;

// 새로운 고급 시스템
extern struct advanced_signal_system global_signal_system;

#endif /* __PROJECTS_PROJECT2_BLINKER_H__ */
