#include "projects/crossroads/priority_sync.h"
#include "threads/interrupt.h"
#include <stdio.h>

// 앰뷸런스 우선순위 비교 함수 (향상된 버전)
bool ambulance_first(const struct list_elem *a, const struct list_elem *b, void *aux)
{
    struct vehicle_info *va = list_entry(a, struct vehicle_info, elem);
    struct vehicle_info *vb = list_entry(b, struct vehicle_info, elem);

    // 1차 우선순위: 차량 타입 (앰뷸런스 > 일반차량)
    if (va->type != vb->type)
    {
        return va->type > vb->type; // ambulance(1) > normal(0)
    }

    // 2차 우선순위: 앰뷸런스끼리는 골든타임이 짧은 순서
    if (va->type == VEHICL_TYPE_AMBULANCE && vb->type == VEHICL_TYPE_AMBULANCE)
    {
        // 골든타임이 짧을수록 우선순위 높음
        if (va->golden_time != vb->golden_time)
        {
            return va->golden_time < vb->golden_time;
        }
        // 골든타임이 같으면 도착시간이 빠른 순
        return va->arrival < vb->arrival;
    }

    // 3차 우선순위: 일반차량끼리는 도착시간 순
    return va->arrival < vb->arrival;
}

void priority_sema_init(struct priority_sema *psema, unsigned value)
{
    psema->value = value;
    list_init(&psema->waiters);
    printf("[PRIORITY] 🚦 Priority semaphore initialized with value %u\n", value);
}

void priority_sema_down(struct priority_sema *psema, struct vehicle_info *vi)
{
    enum intr_level old_level = intr_disable();

    while (psema->value == 0)
    {
        // 우선순위에 따라 정렬된 위치에 삽입
        list_insert_ordered(&psema->waiters, &vi->elem, ambulance_first, NULL);

        if (vi->type == VEHICL_TYPE_AMBULANCE)
        {
            printf("[PRIORITY] 🚑 Ambulance %c waiting for road entry (golden_time: %d)\n",
                   vi->id, vi->golden_time);
        }
        else
        {
            printf("[PRIORITY] 🚗 Vehicle %c waiting for road entry\n", vi->id);
        }

        thread_block(); // vehicle thread block
    }
    psema->value--;

    if (vi->type == VEHICL_TYPE_AMBULANCE)
    {
        printf("[PRIORITY] 🚑 Ambulance %c acquired road entry permit\n", vi->id);
    }
    else
    {
        printf("[PRIORITY] 🚗 Vehicle %c acquired road entry permit\n", vi->id);
    }

    intr_set_level(old_level);
}

void priority_sema_up(struct priority_sema *psema)
{
    enum intr_level old_level = intr_disable();

    if (!list_empty(&psema->waiters))
    {
        // 우선순위가 가장 높은 차량을 깨움
        struct vehicle_info *vi = list_entry(list_pop_front(&psema->waiters), struct vehicle_info, elem);

        if (vi->type == VEHICL_TYPE_AMBULANCE)
        {
            printf("[PRIORITY] 🚑 Ambulance %c unblocked for road entry\n", vi->id);
        }
        else
        {
            printf("[PRIORITY] 🚗 Vehicle %c unblocked for road entry\n", vi->id);
        }

        thread_unblock(vi->t); // t는 vehicle_info에 연결된 thread
    }
    psema->value++;

    intr_set_level(old_level);
}

// 응급 상황을 위한 추가 함수들
void emergency_priority_boost(struct priority_sema *psema, struct vehicle_info *ambulance)
{
    enum intr_level old_level = intr_disable();

    // 앰뷸런스를 대기열 맨 앞으로 이동
    if (!list_empty(&psema->waiters))
    {
        // 이미 대기중인 앰뷸런스가 있는지 확인
        struct list_elem *e;
        for (e = list_begin(&psema->waiters); e != list_end(&psema->waiters); e = list_next(e))
        {
            struct vehicle_info *vi = list_entry(e, struct vehicle_info, elem);
            if (vi == ambulance)
            {
                // 찾았으면 맨 앞으로 이동
                list_remove(e);
                list_push_front(&psema->waiters, e);
                printf("[EMERGENCY] 🚨 Ambulance %c priority boosted to front of queue\n", ambulance->id);
                break;
            }
        }
    }

    intr_set_level(old_level);
}

int get_waiting_ambulance_count(struct priority_sema *psema)
{
    enum intr_level old_level = intr_disable();

    int count = 0;
    struct list_elem *e;
    for (e = list_begin(&psema->waiters); e != list_end(&psema->waiters); e = list_next(e))
    {
        struct vehicle_info *vi = list_entry(e, struct vehicle_info, elem);
        if (vi->type == VEHICL_TYPE_AMBULANCE)
        {
            count++;
        }
    }

    intr_set_level(old_level);
    return count;
}

void print_priority_queue_status(struct priority_sema *psema)
{
    enum intr_level old_level = intr_disable();

    printf("[PRIORITY] Queue status (available permits: %u):\n", psema->value);

    if (list_empty(&psema->waiters))
    {
        printf("[PRIORITY]   - No vehicles waiting\n");
    }
    else
    {
        int pos = 1;
        struct list_elem *e;
        for (e = list_begin(&psema->waiters); e != list_end(&psema->waiters); e = list_next(e))
        {
            struct vehicle_info *vi = list_entry(e, struct vehicle_info, elem);
            if (vi->type == VEHICL_TYPE_AMBULANCE)
            {
                printf("[PRIORITY]   %d. 🚑 Ambulance %c (golden_time: %d, arrival: %d)\n",
                       pos++, vi->id, vi->golden_time, vi->arrival);
            }
            else
            {
                printf("[PRIORITY]   %d. 🚗 Vehicle %c (arrival: %d)\n",
                       pos++, vi->id, vi->arrival);
            }
        }
    }

    intr_set_level(old_level);
}
