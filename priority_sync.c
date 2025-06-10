#include "projects/crossroads/priority_sync.h"
#include "threads/interrupt.h"

bool ambulance_first(const struct list_elem *a, const struct list_elem *b, void *aux)
{
    struct vehicle_info *va = list_entry(a, struct vehicle_info, elem);
    struct vehicle_info *vb = list_entry(b, struct vehicle_info, elem);
    return va->type > vb->type; // ambulance(1) > normal(0)
}

void priority_sema_init(struct priority_sema *psema, unsigned value)
{
    psema->value = value;
    list_init(&psema->waiters);
}

void priority_sema_down(struct priority_sema *psema, struct vehicle_info *vi)
{
    enum intr_level old_level = intr_disable();

    while (psema->value == 0)
    {
        list_insert_ordered(&psema->waiters, &vi->elem, ambulance_first, NULL);
        thread_block(); // vehicle thread block
    }
    psema->value--;

    intr_set_level(old_level);
}

void priority_sema_up(struct priority_sema *psema)
{
    enum intr_level old_level = intr_disable();

    if (!list_empty(&psema->waiters))
    {
        struct vehicle_info *vi = list_entry(list_pop_front(&psema->waiters), struct vehicle_info, elem);
        thread_unblock(vi->t); // t는 vehicle_info에 연결된 thread
    }
    psema->value++;

    intr_set_level(old_level);
}
