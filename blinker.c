#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/blinker.h"
#include "projects/crossroads/priority_semaphore.h"

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

/* 허용되는 경로 조합 테이블 - 사용자 요구사항 기반 */
static const int allowed_route_combinations[][2] = {
    /* 서로 직진 조합 */
    {ROUTE_AC, ROUTE_CA},
    {ROUTE_CA, ROUTE_AC}, // A->C + C->A
    {ROUTE_BD, ROUTE_DB},
    {ROUTE_DB, ROUTE_BD}, // B->D + D->B

    /* 이웃끼리 */
    {ROUTE_AB, ROUTE_BA},
    {ROUTE_BA, ROUTE_AB}, // A->B + B->A
    {ROUTE_AD, ROUTE_DA},
    {ROUTE_DA, ROUTE_AD}, // A->D + D->A
    {ROUTE_CD, ROUTE_DC},
    {ROUTE_DC, ROUTE_CD}, // C->D + D->C
    {ROUTE_BC, ROUTE_CB},
    {ROUTE_CB, ROUTE_BC}, // B->C + C->B
    {ROUTE_BD, ROUTE_DA},
    {ROUTE_DA, ROUTE_BD}, // B->D + D->A
    {ROUTE_BD, ROUTE_AB},
    {ROUTE_AB, ROUTE_BD}, // B->D + A->B
    {ROUTE_DB, ROUTE_CD},
    {ROUTE_CD, ROUTE_DB}, // D->B + C->D
    {ROUTE_DB, ROUTE_BC},
    {ROUTE_BC, ROUTE_DB}, // D->B + B->C
    {ROUTE_AC, ROUTE_CD},
    {ROUTE_CD, ROUTE_AC}, // A->C + C->D
    {ROUTE_AC, ROUTE_DA},
    {ROUTE_DA, ROUTE_AC}, // A->C + D->A
    {ROUTE_CA, ROUTE_AB},
    {ROUTE_AB, ROUTE_CA}, // C->A + A->B
    {ROUTE_CA, ROUTE_BC},
    {ROUTE_BC, ROUTE_CA}, // C->A + B->C

    /* 유턴 차량 조합 */
    {ROUTE_AA, ROUTE_AB},
    {ROUTE_AB, ROUTE_AA}, // A->A + A->B
    {ROUTE_AA, ROUTE_AC},
    {ROUTE_AC, ROUTE_AA}, // A->A + A->C
    {ROUTE_AA, ROUTE_AD},
    {ROUTE_AD, ROUTE_AA}, // A->A + A->D
    {ROUTE_BB, ROUTE_BA},
    {ROUTE_BA, ROUTE_BB}, // B->B + B->A
    {ROUTE_BB, ROUTE_BC},
    {ROUTE_BC, ROUTE_BB}, // B->B + B->C
    {ROUTE_BB, ROUTE_BD},
    {ROUTE_BD, ROUTE_BB}, // B->B + B->D
    {ROUTE_CC, ROUTE_CA},
    {ROUTE_CA, ROUTE_CC}, // C->C + C->A
    {ROUTE_CC, ROUTE_CB},
    {ROUTE_CB, ROUTE_CC}, // C->C + C->B
    {ROUTE_CC, ROUTE_CD},
    {ROUTE_CD, ROUTE_CC}, // C->C + C->D
    {ROUTE_DD, ROUTE_DA},
    {ROUTE_DA, ROUTE_DD}, // D->D + D->A
    {ROUTE_DD, ROUTE_DB},
    {ROUTE_DB, ROUTE_DD}, // D->D + D->B
    {ROUTE_DD, ROUTE_DC},
    {ROUTE_DC, ROUTE_DD}, // D->D + D->C

    /* in and out */
    {ROUTE_AB, ROUTE_BC},
    {ROUTE_BC, ROUTE_AB}, // A->B + B->C
    {ROUTE_BC, ROUTE_CD},
    {ROUTE_CD, ROUTE_BC}, // B->C + C->D
    {ROUTE_CD, ROUTE_DA},
    {ROUTE_DA, ROUTE_CD}, // C->D + D->A
    {ROUTE_DA, ROUTE_AB},
    {ROUTE_AB, ROUTE_DA}, // D->A + A->B

    /* 종료 마커 */
    {-1, -1}};

static bool route_conflict[ROUTE_COUNT][ROUTE_COUNT];

static struct priority_semaphore intersection_semaphore;

/* 활성 경로 정보 - 시간 정보 포함 */
typedef struct
{
    bool active;
    int step_count; /* 해당 경로를 사용한 스텝 수 */
    int vehicle_id; /* 차량 ID (디버깅용) */
} active_route_info_t;

static active_route_info_t active_routes[ROUTE_COUNT];
static struct lock route_tracking_lock;

static route_id_t get_route_id(char start, char dest)
{
    int s = start - 'A';
    int d = dest - 'A';
    return (route_id_t)(s * 4 + d);
}

/* 허용 조합 테이블에서 두 경로가 허용되는지 확인 */
static bool is_allowed_combination(route_id_t route1, route_id_t route2)
{
    if (route1 == route2)
        return false; // 같은 경로는 불가

    for (int i = 0; allowed_route_combinations[i][0] != -1; i++)
    {
        if ((allowed_route_combinations[i][0] == route1 && allowed_route_combinations[i][1] == route2) ||
            (allowed_route_combinations[i][0] == route2 && allowed_route_combinations[i][1] == route1))
        {
            return true;
        }
    }
    return false;
}

/* 기존 충돌 검사 함수 (백업용) */
static bool check_route_conflict_spatial(route_id_t route1, route_id_t route2)
{
    if (route1 == route2)
        return false;

    const struct position *path1 = route_intersection_path[route1];
    const struct position *path2 = route_intersection_path[route2];

    if (path1[0].row == -1 || path2[0].row == -1)
        return false;

    for (int i = 0; path1[i].row != -1; i++)
    {
        for (int j = 0; path2[j].row != -1; j++)
        {
            if (path1[i].row == path2[j].row && path1[i].col == path2[j].col)
            {
                return true;
            }
        }
    }
    return false;
}

/* 개선된 충돌 검사 - 허용 조합 테이블 우선, 그 다음 기존 로직 */
static bool check_route_conflict(route_id_t route1, route_id_t route2)
{
    if (route1 == route2)
        return false;

    /* 1. 먼저 허용 조합 테이블 확인 */
    if (is_allowed_combination(route1, route2))
    {
        return false; /* 허용 조합이므로 충돌 없음 */
    }

    /* 2. 허용 조합에 없으면 기존 공간 기반 충돌 검사 */
    return check_route_conflict_spatial(route1, route2);
}

static void init_conflict_table(void)
{
    int conflict_count = 0;
    int allowed_count = 0;

    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        for (int j = 0; j < ROUTE_COUNT; j++)
        {
            route_conflict[i][j] = check_route_conflict((route_id_t)i, (route_id_t)j);
            if (i < j)
            {
                if (route_conflict[i][j])
                {
                    conflict_count++;
                }
                else
                {
                    allowed_count++;
                }
            }
        }
    }
}

/* 교차로 진입 가능 여부 확인 - 개선된 버전 */
static bool can_enter_intersection(route_id_t my_route, char vehicle_id)
{
    lock_acquire(&route_tracking_lock);

    /* 현재 활성화된 경로들과 충돌 검사 */
    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        if (active_routes[i].active && route_conflict[my_route][i])
        {
            lock_release(&route_tracking_lock);
            return false; // 충돌하는 활성 경로 존재
        }
    }

    /* 진입 가능 - 내 경로를 활성화 */
    active_routes[my_route].active = true;
    active_routes[my_route].step_count = 0;
    active_routes[my_route].vehicle_id = vehicle_id;

    lock_release(&route_tracking_lock);
    return true;
}

/* 교차로 퇴장 처리 - 개선된 버전 */
static void exit_intersection(route_id_t my_route)
{
    lock_acquire(&route_tracking_lock);
    active_routes[my_route].active = false;
    active_routes[my_route].step_count = 0;
    active_routes[my_route].vehicle_id = 0;
    lock_release(&route_tracking_lock);
}

/* 단계별 스텝 카운트 업데이트 */
static void update_route_step(route_id_t my_route)
{
    lock_acquire(&route_tracking_lock);
    if (active_routes[my_route].active)
    {
        active_routes[my_route].step_count++;
    }
    lock_release(&route_tracking_lock);
}

/* 신호등 시스템 초기화 */
void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info)
{
    /* 교차로 전체 제어용 우선순위 세마포어 - 앰뷸런스 우선 처리 */
    priority_sema_init(&intersection_semaphore, 6); // 허용 조합 증가로 인해 확대

    /* 경로 추적을 위한 락 초기화 */
    lock_init(&route_tracking_lock);

    /* 모든 경로 비활성화 */
    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        active_routes[i].active = false;
        active_routes[i].step_count = 0;
        active_routes[i].vehicle_id = 0;
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

    /* 경로 충돌 체크 - 개선된 로직 사용 */
    if (can_enter_intersection(my_route, vi->id))
    {
        /* 우선순위 세마포어 획득 - 차량 타입에 따라 다른 처리 */
        if (vi->type == VEHICL_TYPE_AMBULANCE)
        {
            priority_sema_down_ambulance(&intersection_semaphore, vi->arrival, vi->golden_time);
        }
        else
        {
            priority_sema_down_normal(&intersection_semaphore);
        }
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

        /* 우선순위 세마포어 해제 */
        priority_sema_up(&intersection_semaphore);
    }
}

/* 스텝 업데이트 함수 (교차로 내에서 호출) */
void update_intersection_step(struct vehicle_info *vi, struct position pos)
{
    /* 교차로 내부에서만 스텝 카운트 업데이트 */
    if (pos.row >= 2 && pos.row <= 4 && pos.col >= 2 && pos.col <= 4)
    {
        route_id_t my_route = get_route_id(vi->start, vi->dest);
        update_route_step(my_route);
    }
}

/* 현재 교차로 상태 출력 */
void print_intersection_status(void)
{
    lock_acquire(&route_tracking_lock);

    bool any_active = false;

    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        if (active_routes[i].active)
        {
            char start = 'A' + (i / 4);
            char dest = 'A' + (i % 4);

            any_active = true;
        }
    }

    lock_release(&route_tracking_lock);
}