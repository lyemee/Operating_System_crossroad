#ifndef __PROJECTS_CROSSROADS_PRIORITY_SYNC_H__
#define __PROJECTS_CROSSROADS_PRIORITY_SYNC_H__

#include "threads/thread.h"
#include "lib/kernel/list.h"
#include "projects/crossroads/vehicle.h"

// 우선순위 세마포어 구조체
struct priority_sema
{
    unsigned value;
    struct list waiters; // vehicle_info 대기 리스트
};

// 함수 선언
void priority_sema_init(struct priority_sema *psema, unsigned value);
void priority_sema_down(struct priority_sema *psema, struct vehicle_info *vi);
void priority_sema_up(struct priority_sema *psema);

#endif /* __PROJECTS_CROSSROADS_PRIORITY_SYNC_H__ */
