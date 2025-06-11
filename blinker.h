#ifndef __PROJECTS_PROJECT2_BLINKER_H__
#define __PROJECTS_PROJECT2_BLINKER_H__

#include "threads/synch.h"
#include "projects/crossroads/vehicle.h" // vehicle_info 사용을 위해 필요

/** you can change the number of blinkers */
#define NUM_BLINKER 4

enum blinker_dir
{
    BLINKER_EW,
    BLINKER_NS
};

struct blinker_info
{
    struct lock **map_locks;
    struct vehicle_info *vehicles;
};

void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info);
void start_blinker();

// 외부 참조용 전역 변수 선언
extern enum blinker_dir current_blinker_state;
extern struct lock blinker_lock;
extern struct condition blinker_cond;

#endif /* __PROJECTS_PROJECT2_BLINKER_H__ */
