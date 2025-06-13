// /* blinker.c - 수정된 경로 분석 버전 */
// #include "threads/thread.h"
// #include "threads/synch.h"
// #include "projects/crossroads/blinker.h"

// /* 교차로 경로 충돌 분석 테이블 */
// typedef enum
// {
//     ROUTE_AA = 0,
//     ROUTE_AB,
//     ROUTE_AC,
//     ROUTE_AD,
//     ROUTE_BA,
//     ROUTE_BB,
//     ROUTE_BC,
//     ROUTE_BD,
//     ROUTE_CA,
//     ROUTE_CB,
//     ROUTE_CC,
//     ROUTE_CD,
//     ROUTE_DA,
//     ROUTE_DB,
//     ROUTE_DC,
//     ROUTE_DD,
//     ROUTE_COUNT
// } route_id_t;

// /* vehicle_path에서 실제 교차로 부분만 정확히 추출 */
// static const struct position route_intersection_path[ROUTE_COUNT][10] = {
//     /* A->A */ {{4, 2}, {4, 3}, {4, 4}, {3, 4}, {2, 4}, {2, 3}, {2, 2}, {-1, -1}},
//     /* A->B */ {{4, 2}, {-1, -1}},
//     /* A->C */ {{4, 2}, {4, 3}, {4, 4}, {-1, -1}},
//     /* A->D */ {{4, 2}, {4, 3}, {4, 4}, {3, 4}, {2, 4}, {-1, -1}},

//     /* B->A */ {{4, 4}, {3, 4}, {2, 4}, {2, 3}, {2, 2}, {-1, -1}},
//     /* B->B */ {{4, 4}, {3, 4}, {2, 4}, {2, 3}, {2, 2}, {3, 2}, {4, 2}, {-1, -1}},
//     /* B->C */ {{4, 4}, {-1, -1}},
//     /* B->D */ {{4, 4}, {3, 4}, {2, 4}, {-1, -1}},

//     /* C->A */ {{2, 4}, {2, 3}, {2, 2}, {-1, -1}},
//     /* C->B */ {{2, 4}, {2, 3}, {2, 2}, {3, 2}, {4, 2}, {-1, -1}},
//     /* C->C */ {{2, 4}, {2, 3}, {2, 2}, {3, 2}, {4, 2}, {4, 3}, {4, 4}, {-1, -1}},
//     /* C->D */ {{2, 4}, {-1, -1}},

//     /* D->A */ {{2, 2}, {-1, -1}},
//     /* D->B */ {{2, 2}, {3, 2}, {4, 2}, {-1, -1}},
//     /* D->C */ {{2, 2}, {3, 2}, {4, 2}, {4, 3}, {4, 4}, {-1, -1}},
//     /* D->D */ {{2, 2}, {3, 2}, {4, 2}, {4, 3}, {4, 4}, {3, 4}, {2, 4}, {-1, -1}}};

// /* 경로 간 충돌 여부 테이블 */
// static bool route_conflict[ROUTE_COUNT][ROUTE_COUNT];

// /* 기본 세마포어 - 교차로 전체 제어 */
// static struct semaphore intersection_semaphore;

// /* 현재 교차로 진입 중인 차량들의 경로 추적 */
// static bool active_routes[ROUTE_COUNT];
// static struct lock route_tracking_lock;

// /* 경로 ID 계산 */
// static route_id_t get_route_id(char start, char dest)
// {
//     int s = start - 'A';
//     int d = dest - 'A';
//     return (route_id_t)(s * 4 + d);
// }

// /* 두 경로가 실제로 충돌하는지 정확히 확인 */
// static bool check_route_conflict(route_id_t route1, route_id_t route2)
// {
//     if (route1 == route2)
//         return false; // 같은 경로는 충돌 안함

//     const struct position *path1 = route_intersection_path[route1];
//     const struct position *path2 = route_intersection_path[route2];

//     /* 경로가 교차로를 지나지 않으면 충돌 없음 */
//     if (path1[0].row == -1 || path2[0].row == -1)
//         return false;

//     for (int i = 0; path1[i].row != -1; i++)
//     {
//         for (int j = 0; path2[j].row != -1; j++)
//         {
//             if (path1[i].row == path2[j].row && path1[i].col == path2[j].col)
//             {
//                 return true; // 실제 충돌 지점 발견
//             }
//         }
//     }
//     return false; // 충돌 없음
// }

// /* 충돌 테이블 초기화 */
// static void init_conflict_table(void)
// {
//     int conflict_count = 0;

//     for (int i = 0; i < ROUTE_COUNT; i++)
//     {
//         for (int j = 0; j < ROUTE_COUNT; j++)
//         {
//             route_conflict[i][j] = check_route_conflict((route_id_t)i, (route_id_t)j);
//             if (i < j && route_conflict[i][j])
//                 conflict_count++;
//         }
//     }
// }

// /* 교차로 진입 가능 여부 확인 */
// static bool can_enter_intersection(route_id_t my_route)
// {
//     lock_acquire(&route_tracking_lock);

//     /* 현재 활성화된 경로들과 충돌 검사 */
//     for (int i = 0; i < ROUTE_COUNT; i++)
//     {
//         if (active_routes[i] && route_conflict[my_route][i])
//         {
//             lock_release(&route_tracking_lock);
//             return false; // 충돌하는 활성 경로 존재
//         }
//     }

//     /* 진입 가능 - 내 경로를 활성화 */
//     active_routes[my_route] = true;

//     lock_release(&route_tracking_lock);
//     return true;
// }

// /* 교차로 퇴장 처리 */
// static void exit_intersection(route_id_t my_route)
// {
//     lock_acquire(&route_tracking_lock);
//     active_routes[my_route] = false;
//     lock_release(&route_tracking_lock);
// }

// /* 신호등 시스템 초기화 */
// void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info)
// {
//     /* 교차로 전체 제어용 세마포어 - 충돌 방지만 목적 */
//     sema_init(&intersection_semaphore, 4); // 최대 4대 동시 진입 허용

//     /* 경로 추적을 위한 락 초기화 */
//     lock_init(&route_tracking_lock);

//     /* 모든 경로 비활성화 */
//     for (int i = 0; i < ROUTE_COUNT; i++)
//     {
//         active_routes[i] = false;
//     }

//     /* 충돌 테이블 초기화 */
//     init_conflict_table();

//     blinkers->map_locks = map_locks;
//     blinkers->vehicles = vehicle_info;
// }

// void start_blinker()
// {
//     printf("[신호등] 교차로 신호등 시작\n");
// }

// /* 교차로 진입 권한 요청 */
// bool acquire_intersection_permission(struct vehicle_info *vi, struct position pos)
// {
//     /* 교차로 외부면 즉시 허용 */
//     if (pos.row < 2 || pos.row > 4 || pos.col < 2 || pos.col > 4)
//     {
//         return true;
//     }

//     route_id_t my_route = get_route_id(vi->start, vi->dest);

//     /* 경로 충돌 체크 */
//     if (can_enter_intersection(my_route))
//     {

//         /* 세마포어 획득 */
//         sema_down(&intersection_semaphore);

//         return true;
//     }
//     else
//     {
//         return false; // 충돌로 인한 진입 실패
//     }
// }

// /* 교차로 퇴장 처리 */
// void release_intersection_permission(struct vehicle_info *vi, struct position pos)
// {
//     /* 교차로 외부로 나간 경우만 처리 */
//     if (pos.row < 2 || pos.row > 4 || pos.col < 2 || pos.col > 4)
//     {
//         route_id_t my_route = get_route_id(vi->start, vi->dest);
//         exit_intersection(my_route);

//         /* 세마포어 해제 */
//         sema_up(&intersection_semaphore);

//         printf("[차량 %c] 교차로 퇴장 완료\n", vi->id);
//     }
// }

// /* 현재 교차로 상태 출력 */
// void print_intersection_status(void)
// {

//     bool any_active = false;
//     for (int i = 0; i < ROUTE_COUNT; i++)
//     {
//         if (active_routes[i])
//         {
//             any_active = true;
//         }
//     }
// }
/* blinker.c - 수정된 경로 분석 버전 */
#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/blinker.h"

/* 교차로 경로 충돌 분석 테이블 */
typedef enum
{
    ROUTE_AA = 0,
    ROUTE_AB,
    ROUTE_AC,
    ROUTE_AD,
    ROUTE_BA,
    ROUTE_BB,
    ROUTE_BC,
    ROUTE_BD,
    ROUTE_CA,
    ROUTE_CB,
    ROUTE_CC,
    ROUTE_CD,
    ROUTE_DA,
    ROUTE_DB,
    ROUTE_DC,
    ROUTE_DD,
    ROUTE_COUNT
} route_id_t;

/* vehicle_path에서 실제 교차로 부분만 정확히 추출 */
static const struct position route_intersection_path[ROUTE_COUNT][10] = {
    /* A->A */ {{4, 2}, {4, 3}, {4, 4}, {3, 4}, {2, 4}, {2, 3}, {2, 2}, {-1, -1}},
    /* A->B */ {{4, 2}, {-1, -1}},
    /* A->C */ {{4, 2}, {4, 3}, {4, 4}, {-1, -1}},
    /* A->D */ {{4, 2}, {4, 3}, {4, 4}, {3, 4}, {2, 4}, {-1, -1}},

    /* B->A */ {{4, 4}, {3, 4}, {2, 4}, {2, 3}, {2, 2}, {-1, -1}},
    /* B->B */ {{4, 4}, {3, 4}, {2, 4}, {2, 3}, {2, 2}, {3, 2}, {4, 2}, {-1, -1}},
    /* B->C */ {{4, 4}, {-1, -1}},
    /* B->D */ {{4, 4}, {3, 4}, {2, 4}, {-1, -1}},

    /* C->A */ {{2, 4}, {2, 3}, {2, 2}, {-1, -1}},
    /* C->B */ {{2, 4}, {2, 3}, {2, 2}, {3, 2}, {4, 2}, {-1, -1}},
    /* C->C */ {{2, 4}, {2, 3}, {2, 2}, {3, 2}, {4, 2}, {4, 3}, {4, 4}, {-1, -1}},
    /* C->D */ {{2, 4}, {-1, -1}},

    /* D->A */ {{2, 2}, {-1, -1}},
    /* D->B */ {{2, 2}, {3, 2}, {4, 2}, {-1, -1}},
    /* D->C */ {{2, 2}, {3, 2}, {4, 2}, {4, 3}, {4, 4}, {-1, -1}},
    /* D->D */ {{2, 2}, {3, 2}, {4, 2}, {4, 3}, {4, 4}, {3, 4}, {2, 4}, {-1, -1}}};

/* 경로 간 충돌 여부 테이블 */
static bool route_conflict[ROUTE_COUNT][ROUTE_COUNT];

/* 기본 세마포어 - 교차로 전체 제어 */
static struct semaphore intersection_semaphore;

/* 현재 교차로 진입 중인 차량들의 경로 추적 */
static bool active_routes[ROUTE_COUNT];
static struct lock route_tracking_lock;

/* 경로 ID 계산 */
static route_id_t get_route_id(char start, char dest)
{
    int s = start - 'A';
    int d = dest - 'A';
    return (route_id_t)(s * 4 + d);
}

/* 두 경로가 실제로 충돌하는지 정확히 확인 */
static bool check_route_conflict(route_id_t route1, route_id_t route2)
{
    if (route1 == route2)
        return false; // 같은 경로는 충돌 안함

    const struct position *path1 = route_intersection_path[route1];
    const struct position *path2 = route_intersection_path[route2];

    /* 경로가 교차로를 지나지 않으면 충돌 없음 */
    if (path1[0].row == -1 || path2[0].row == -1)
        return false;

    for (int i = 0; path1[i].row != -1; i++)
    {
        for (int j = 0; path2[j].row != -1; j++)
        {
            if (path1[i].row == path2[j].row && path1[i].col == path2[j].col)
            {
                return true; // 실제 충돌 지점 발견
            }
        }
    }
    return false; // 충돌 없음
}

/* 충돌 테이블 초기화 */
static void init_conflict_table(void)
{
    int conflict_count = 0;

    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        for (int j = 0; j < ROUTE_COUNT; j++)
        {
            route_conflict[i][j] = check_route_conflict((route_id_t)i, (route_id_t)j);
            if (i < j && route_conflict[i][j])
                conflict_count++;
        }
    }

    /* 실제 충돌하는 경로들만 출력 */
    if (conflict_count > 0)
    {
    }
    else
    {
    }
}

/* 교차로 진입 가능 여부 확인 */
static bool can_enter_intersection(route_id_t my_route)
{
    lock_acquire(&route_tracking_lock);

    /* 현재 활성화된 경로들과 충돌 검사 */
    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        if (active_routes[i] && route_conflict[my_route][i])
        {
            lock_release(&route_tracking_lock);
            return false; // 충돌하는 활성 경로 존재
        }
    }

    /* 진입 가능 - 내 경로를 활성화 */
    active_routes[my_route] = true;

    lock_release(&route_tracking_lock);
    return true;
}

/* 교차로 퇴장 처리 */
static void exit_intersection(route_id_t my_route)
{
    lock_acquire(&route_tracking_lock);
    active_routes[my_route] = false;
    lock_release(&route_tracking_lock);
}

/* 신호등 시스템 초기화 */
void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info)
{
    /* 교차로 전체 제어용 세마포어 - 충돌 방지만 목적 */
    sema_init(&intersection_semaphore, 4); // 최대 4대 동시 진입 허용

    /* 경로 추적을 위한 락 초기화 */
    lock_init(&route_tracking_lock);

    /* 모든 경로 비활성화 */
    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        active_routes[i] = false;
    }

    /* 충돌 테이블 초기화 */
    init_conflict_table();

    blinkers->map_locks = map_locks;
    blinkers->vehicles = vehicle_info;
}

void start_blinker()
{
}

/* 교차로 진입 권한 요청 */
bool acquire_intersection_permission(struct vehicle_info *vi, struct position pos)
{
    /* 교차로 외부면 즉시 허용 */
    if (pos.row < 2 || pos.row > 4 || pos.col < 2 || pos.col > 4)
    {
        return true;
    }

    route_id_t my_route = get_route_id(vi->start, vi->dest);

    /* 경로 충돌 체크 */
    if (can_enter_intersection(my_route))
    {

        /* 세마포어 획득 */
        sema_down(&intersection_semaphore);

        return true;
    }
    else
    {
        return false; // 충돌로 인한 진입 실패
    }
}

/* 교차로 퇴장 처리 */
void release_intersection_permission(struct vehicle_info *vi, struct position pos)
{
    /* 교차로 외부로 나간 경우만 처리 */
    if (pos.row < 2 || pos.row > 4 || pos.col < 2 || pos.col > 4)
    {
        route_id_t my_route = get_route_id(vi->start, vi->dest);
        exit_intersection(my_route);

        /* 세마포어 해제 */
        sema_up(&intersection_semaphore);
    }
}

/* 현재 교차로 상태 출력 */
void print_intersection_status(void)
{

    bool any_active = false;
    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        if (active_routes[i])
        {
            any_active = true;
        }
    }
}