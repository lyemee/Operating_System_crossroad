#include "projects/crossroads/priority_semaphore.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"

/* 앰뷸런스 우선순위 비교 함수 - urgency score가 낮을수록 우선 */
static bool ambulance_urgency_less(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
    struct ambulance_waiter *wa = list_entry(a, struct ambulance_waiter, elem);
    struct ambulance_waiter *wb = list_entry(b, struct ambulance_waiter, elem);

    return wa->urgency_score < wb->urgency_score;
}

void priority_sema_init(struct priority_semaphore *psema, unsigned value)
{
    psema->value = value;
    list_init(&psema->waiters_ambulance);
    list_init(&psema->waiters_normal);
}

void priority_sema_down_ambulance(struct priority_semaphore *psema, int arrival, int golden)
{
    enum intr_level old_level = intr_disable();

    while (psema->value == 0)
    {
        struct ambulance_waiter *waiter = malloc(sizeof(struct ambulance_waiter));
        waiter->thread = thread_current();
        waiter->arrival_time = arrival;
        waiter->golden_time = golden;
        waiter->urgency_score = arrival + golden;

        list_insert_ordered(&psema->waiters_ambulance, &waiter->elem,
                            ambulance_urgency_less, NULL);
        thread_block();
    }
    psema->value--;

    intr_set_level(old_level);
}

void priority_sema_down_normal(struct priority_semaphore *psema)
{
    enum intr_level old_level = intr_disable();

    while (psema->value == 0)
    {
        struct normal_waiter *waiter = malloc(sizeof(struct normal_waiter));
        waiter->thread = thread_current();

        list_push_back(&psema->waiters_normal, &waiter->elem);
        thread_block();
    }
    psema->value--;

    intr_set_level(old_level);
}

void priority_sema_up(struct priority_semaphore *psema)
{
    enum intr_level old_level = intr_disable();

    /* 앰뷸런스가 대기 중이면 우선 처리 */
    if (!list_empty(&psema->waiters_ambulance))
    {
        struct ambulance_waiter *waiter = list_entry(
            list_pop_front(&psema->waiters_ambulance),
            struct ambulance_waiter, elem);

        thread_unblock(waiter->thread);
        free(waiter);
    }
    /* 일반 차량 처리 */
    else if (!list_empty(&psema->waiters_normal))
    {
        struct normal_waiter *waiter = list_entry(
            list_pop_front(&psema->waiters_normal),
            struct normal_waiter, elem);

        thread_unblock(waiter->thread);
        free(waiter);
    }

    psema->value++;
    intr_set_level(old_level);
}

/* 기존 함수 호환성 유지 (일반 차량으로 처리) */
void priority_sema_down(struct priority_semaphore *psema)
{
    priority_sema_down_normal(psema);
}