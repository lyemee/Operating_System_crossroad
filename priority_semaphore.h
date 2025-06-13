#ifndef __THREADS_PRIORITY_SEMAPHORE_H__
#define __THREADS_PRIORITY_SEMAPHORE_H__

#include <list.h>
#include "threads/thread.h"

/* 우선순위를 고려하는 세마포어 구조체 */
struct priority_semaphore
{
    unsigned value;                // 현재 세마포어 값
    struct list waiters_ambulance; // 앰뷸런스 대기 목록
    struct list waiters_normal;    // 일반 차량 대기 목록
};

/* 앰뷸런스 정보를 포함한 대기자 구조체 */
struct ambulance_waiter
{
    struct list_elem elem;
    struct thread *thread;
    int arrival_time;
    int golden_time;
    int urgency_score; // arrival_time + golden_time
};

/* 일반 차량 대기자 구조체 */
struct normal_waiter
{
    struct list_elem elem;
    struct thread *thread;
};

/* 초기화 */
void priority_sema_init(struct priority_semaphore *psema, unsigned value);

/* 다운(획득) - 앰뷸런스용 */
void priority_sema_down_ambulance(struct priority_semaphore *psema, int arrival, int golden);

/* 다운(획득) - 일반 차량용 */
void priority_sema_down_normal(struct priority_semaphore *psema);

/* 업(해제) */
void priority_sema_up(struct priority_semaphore *psema);

/* 기존 함수들 (호환성 유지) */
void priority_sema_down(struct priority_semaphore *psema);

#endif /* __THREADS_PRIORITY_SEMAPHORE_H__ */