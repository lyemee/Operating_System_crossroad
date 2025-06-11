
#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/blinker.h"
#include "projects/crossroads/crossroads.h" // for CROSSROADS_UNIT_TIME_MS

enum blinker_dir current_blinker_state;
struct condition blinker_cond;
struct lock blinker_lock;

static int last_step = -1;

void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info)
{
    // 블링커 정보 초기화
    for (int i = 0; i < NUM_BLINKER; i++)
    {
        blinkers[i].map_locks = map_locks;
        blinkers[i].vehicles = vehicle_info;
    }

    // 조건 변수 및 락 초기화
    lock_init(&blinker_lock);
    cond_init(&blinker_cond);
    current_blinker_state = BLINKER_EW; // 초기 상태: 동서방향 허용
}

static void blinker_thread(void *aux UNUSED)
{
    lock_acquire(&blinker_lock);

    last_step = crossroads_step;

    while (true)
    {
        // unit step이 바뀔 때까지 기다림
        while (last_step == crossroads_step)
            cond_wait(&blinker_cond, &blinker_lock);

        last_step = crossroads_step;

        // 신호 상태를 전환
        current_blinker_state = (current_blinker_state == BLINKER_EW) ? BLINKER_NS : BLINKER_EW;

        // 차량들에게 신호 변경 알림
        cond_broadcast(&blinker_cond, &blinker_lock);
    }

    lock_release(&blinker_lock); // 도달할 수 없지만 형식상 필요
}

void blinker_loop(void *aux UNUSED)
{
    while (true)
    {
        // timer_sleep(TIMER_FREQ * 3); // 3초마다 전환

        lock_acquire(&blinker_lock);
        current_blinker_state = (current_blinker_state == BLINKER_NS) ? BLINKER_EW : BLINKER_NS;
        printf("[BLINKER CHANGE] -> %s\n", current_blinker_state == BLINKER_NS ? "NS" : "EW");
        cond_broadcast(&blinker_cond, &blinker_lock);
        lock_release(&blinker_lock);
    }
}

void start_blinker()
{
    // thread_create("blinker", PRI_DEFAULT + 1, blinker_thread, NULL);
    thread_create("blinker", PRI_DEFAULT + 1, blinker_thread, NULL);
}
