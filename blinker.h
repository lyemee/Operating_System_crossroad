#ifndef __PROJECTS_PROJECT2_BLINKER_H__
#define __PROJECTS_PROJECT2_BLINKER_H__

#include "projects/crossroads/position.h"
#include "projects/crossroads/vehicle.h"

/** you can change the number of blinkers */
#define NUM_BLINKER 4

struct blinker_info
{
    struct lock **map_locks;
    struct vehicle_info *vehicles;
};

/* 기존 함수들 */
void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info);
void start_blinker();

/* 교차로 진입/퇴장 제어 함수들 */
bool acquire_intersection_permission(struct vehicle_info *vi, struct position pos);
void release_intersection_permission(struct vehicle_info *vi, struct position pos);

/* 새로 추가된 함수들 */
void update_intersection_step(struct vehicle_info *vi, struct position pos);
void print_intersection_status(void);

#endif /* __PROJECTS_PROJECT2_BLINKER_H__ */