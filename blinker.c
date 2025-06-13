
#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/blinker.h"

void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info)
{
    /* TODO: 신호등 시스템 초기화 구현 필요 */
    blinkers->map_locks = map_locks;
    blinkers->vehicles = vehicle_info;
}

void start_blinker()
{
}
