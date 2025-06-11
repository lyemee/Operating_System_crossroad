// #ifndef __PROJECTS_CROSSROADS_PRIORITY_SYNC_H__
// #define __PROJECTS_CROSSROADS_PRIORITY_SYNC_H__

// #include "threads/thread.h"
// #include "lib/kernel/list.h"
// #include "projects/crossroads/vehicle.h"

// // 우선순위 세마포어 구조체
// struct priority_sema
// {
//     unsigned value;
//     struct list waiters; // vehicle_info 대기 리스트
// };

// // 앰뷸런스 우선 정렬 함수
// bool ambulance_first(const struct list_elem *a, const struct list_elem *b, void *aux);

// // 함수 선언
// void priority_sema_init(struct priority_sema *psema, unsigned value);
// void priority_sema_down(struct priority_sema *psema, struct vehicle_info *vi);
// void priority_sema_up(struct priority_sema *psema);

// #endif /* __PROJECTS_CROSSROADS_PRIORITY_SYNC_H__ */
#ifndef __PROJECTS_CROSSROADS_PRIORITY_SYNC_H__
#define __PROJECTS_CROSSROADS_PRIORITY_SYNC_H__

#include "threads/thread.h"
#include "lib/kernel/list.h"
#include "projects/crossroads/vehicle.h"

// 우선순위 세마포어 구조체
struct priority_sema
{
    unsigned value;
    struct list waiters; // vehicle_info 대기 리스트 (우선순위 정렬)
};

// 기본 우선순위 함수
bool ambulance_first(const struct list_elem *a, const struct list_elem *b, void *aux);

// 기본 세마포어 함수들
void priority_sema_init(struct priority_sema *psema, unsigned value);
void priority_sema_down(struct priority_sema *psema, struct vehicle_info *vi);
void priority_sema_up(struct priority_sema *psema);

// 응급상황 처리를 위한 확장 함수들
void emergency_priority_boost(struct priority_sema *psema, struct vehicle_info *ambulance);
int get_waiting_ambulance_count(struct priority_sema *psema);
void print_priority_queue_status(struct priority_sema *psema);

#endif /* __PROJECTS_CROSSROADS_PRIORITY_SYNC_H__ */
