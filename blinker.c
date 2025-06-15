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

/* 허용되는 경로 조합 테이블 */
static const int allowed_route_combinations[][2] = {
    /* 서로 직진 조합 */
    {ROUTE_AC, ROUTE_CA},
    {ROUTE_CA, ROUTE_AC},
    {ROUTE_BD, ROUTE_DB},
    {ROUTE_DB, ROUTE_BD},

    /* 이웃끼리 */
    {ROUTE_AB, ROUTE_BA},
    {ROUTE_BA, ROUTE_AB},
    {ROUTE_AD, ROUTE_DA},
    {ROUTE_DA, ROUTE_AD},
    {ROUTE_CD, ROUTE_DC},
    {ROUTE_DC, ROUTE_CD},
    {ROUTE_BC, ROUTE_CB},
    {ROUTE_CB, ROUTE_BC},
    {ROUTE_BD, ROUTE_DA},
    {ROUTE_DA, ROUTE_BD},
    {ROUTE_BD, ROUTE_AB},
    {ROUTE_AB, ROUTE_BD},
    {ROUTE_DB, ROUTE_CD},
    {ROUTE_CD, ROUTE_DB},
    {ROUTE_DB, ROUTE_BC},
    {ROUTE_BC, ROUTE_DB},
    {ROUTE_AC, ROUTE_CD},
    {ROUTE_CD, ROUTE_AC},
    {ROUTE_AC, ROUTE_DA},
    {ROUTE_DA, ROUTE_AC},
    {ROUTE_CA, ROUTE_AB},
    {ROUTE_AB, ROUTE_CA},
    {ROUTE_CA, ROUTE_BC},
    {ROUTE_BC, ROUTE_CA},

    /* 유턴 차량 조합 */
    {ROUTE_AA, ROUTE_AB},
    {ROUTE_AB, ROUTE_AA},
    {ROUTE_AA, ROUTE_AC},
    {ROUTE_AC, ROUTE_AA},
    {ROUTE_AA, ROUTE_AD},
    {ROUTE_AD, ROUTE_AA},
    {ROUTE_BB, ROUTE_BA},
    {ROUTE_BA, ROUTE_BB},
    {ROUTE_BB, ROUTE_BC},
    {ROUTE_BC, ROUTE_BB},
    {ROUTE_BB, ROUTE_BD},
    {ROUTE_BD, ROUTE_BB},
    {ROUTE_CC, ROUTE_CA},
    {ROUTE_CA, ROUTE_CC},
    {ROUTE_CC, ROUTE_CB},
    {ROUTE_CB, ROUTE_CC},
    {ROUTE_CC, ROUTE_CD},
    {ROUTE_CD, ROUTE_CC},
    {ROUTE_DD, ROUTE_DA},
    {ROUTE_DA, ROUTE_DD},
    {ROUTE_DD, ROUTE_DB},
    {ROUTE_DB, ROUTE_DD},
    {ROUTE_DD, ROUTE_DC},
    {ROUTE_DC, ROUTE_DD},

    /* in and out */
    {ROUTE_AB, ROUTE_BC},
    {ROUTE_BC, ROUTE_AB},
    {ROUTE_BC, ROUTE_CD},
    {ROUTE_CD, ROUTE_BC},
    {ROUTE_CD, ROUTE_DA},
    {ROUTE_DA, ROUTE_CD},
    {ROUTE_DA, ROUTE_AB},
    {ROUTE_AB, ROUTE_DA},

    {-1, -1}};

/* 🚑 앰뷸런스 예측 시스템 */
typedef struct
{
    bool is_active;                      // 앰뷸런스 예측 활성 여부
    route_id_t ambulance_route;          // 앰뷸런스 경로
    int arrival_step;                    // 앰뷸런스 도착 예정 스텝
    int current_step;                    // 현재 시스템 스텝
    bool priority_vehicles[ROUTE_COUNT]; // 우선권 부여된 일반차량들
} ambulance_prediction_t;

static ambulance_prediction_t ambulance_prediction;

/* 활성 경로 정보 - 개선된 버전 */
typedef struct
{
    bool active;
    int step_count;
    char vehicle_id;
    int vehicle_type;         // 차량 타입 추가
    int distance_from_entry;  // 교차로 진입점으로부터의 거리
    bool has_priority_signal; // 우선 신호 보유 여부
} active_route_info_t;

static bool route_conflict[ROUTE_COUNT][ROUTE_COUNT];
static struct priority_semaphore intersection_semaphore;
static active_route_info_t active_routes[ROUTE_COUNT];
static struct lock route_tracking_lock;
static struct lock ambulance_prediction_lock;

static route_id_t get_route_id(char start, char dest)
{
    int s = start - 'A';
    int d = dest - 'A';
    return (route_id_t)(s * 4 + d);
}

/* 허용 조합 확인 */
static bool is_allowed_combination(route_id_t route1, route_id_t route2)
{
    if (route1 == route2)
        return false;

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

/* 공간 기반 충돌 검사 */
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

/* 개선된 충돌 검사 */
static bool check_route_conflict(route_id_t route1, route_id_t route2)
{
    if (route1 == route2)
        return false;

    if (is_allowed_combination(route1, route2))
    {
        return false;
    }

    return check_route_conflict_spatial(route1, route2);
}

/* 🚑 앰뷸런스 예측 시스템 초기화 */
static void init_ambulance_prediction(void)
{
    lock_init(&ambulance_prediction_lock);
    ambulance_prediction.is_active = false;
    ambulance_prediction.ambulance_route = -1;
    ambulance_prediction.arrival_step = -1;
    ambulance_prediction.current_step = 0;

    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        ambulance_prediction.priority_vehicles[i] = false;
    }
}

/* 🚑 앰뷸런스 경로 등록 */
void register_ambulance_route(char start, char dest, int arrival_step)
{
    lock_acquire(&ambulance_prediction_lock);

    ambulance_prediction.is_active = true;
    ambulance_prediction.ambulance_route = get_route_id(start, dest);
    ambulance_prediction.arrival_step = arrival_step;

    lock_release(&ambulance_prediction_lock);
}

/* 🚑 경로상 일반차량에게 우선 신호 부여 */
static bool should_give_priority_signal(route_id_t vehicle_route, char vehicle_id)
{
    if (!ambulance_prediction.is_active)
        return false;

    lock_acquire(&ambulance_prediction_lock);

    route_id_t amb_route = ambulance_prediction.ambulance_route;
    int steps_until_ambulance = ambulance_prediction.arrival_step - ambulance_prediction.current_step;

    bool give_priority = false;

    // 앰뷸런스가 3스텝 이내에 올 예정이고, 같은 경로에 있는 일반차량
    if (steps_until_ambulance <= 3 && steps_until_ambulance > 0)
    {

        // 1. 앰뷸런스와 같은 경로의 차량
        if (vehicle_route == amb_route)
        {
            give_priority = true;
        }

        // 2. 앰뷸런스 경로와 교차하는 차량 중 먼저 비워야 할 차량
        else if (check_route_conflict_spatial(vehicle_route, amb_route))
        {
            const struct position *amb_path = route_intersection_path[amb_route];
            const struct position *vehicle_path = route_intersection_path[vehicle_route];

            // 앰뷸런스 진입점에 가까운 차량부터 우선권 부여
            for (int i = 0; i < 3 && amb_path[i].row != -1; i++)
            {
                for (int j = 0; vehicle_path[j].row != -1; j++)
                {
                    if (amb_path[i].row == vehicle_path[j].row &&
                        amb_path[i].col == vehicle_path[j].col)
                    {
                        give_priority = true;
                        goto priority_decided;
                    }
                }
            }
        priority_decided:;
        }
    }

    if (give_priority)
    {
        ambulance_prediction.priority_vehicles[vehicle_route] = true;
    }

    lock_release(&ambulance_prediction_lock);
    return give_priority;
}

/* 스텝 업데이트 */
void update_ambulance_prediction_step(int current_step)
{
    lock_acquire(&ambulance_prediction_lock);
    ambulance_prediction.current_step = current_step;

    // 앰뷸런스가 도착했으면 예측 시스템 비활성화
    if (ambulance_prediction.is_active &&
        current_step >= ambulance_prediction.arrival_step)
    {
        ambulance_prediction.is_active = false;

        // 우선권 부여 초기화
        for (int i = 0; i < ROUTE_COUNT; i++)
        {
            ambulance_prediction.priority_vehicles[i] = false;
        }
    }

    lock_release(&ambulance_prediction_lock);
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

/* 개선된 교차로 진입 가능 여부 확인 */
static bool can_enter_intersection(route_id_t my_route, char vehicle_id, int vehicle_type)
{
    lock_acquire(&route_tracking_lock);

    // 🚑 우선 신호 확인 (일반차량만)
    bool has_priority = false;
    if (vehicle_type == VEHICL_TYPE_NORMAL)
    {
        has_priority = should_give_priority_signal(my_route, vehicle_id);
    }

    /* 현재 활성화된 경로들과 충돌 검사 */
    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        if (active_routes[i].active && route_conflict[my_route][i])
        {
            // 우선 신호가 있으면 일반 차량도 통과 가능
            if (!has_priority)
            {
                lock_release(&route_tracking_lock);
                return false;
            }
        }
    }

    /* 진입 가능 - 내 경로를 활성화 */
    active_routes[my_route].active = true;
    active_routes[my_route].step_count = 0;
    active_routes[my_route].vehicle_id = vehicle_id;
    active_routes[my_route].vehicle_type = vehicle_type;
    active_routes[my_route].distance_from_entry = 0;
    active_routes[my_route].has_priority_signal = has_priority;

    lock_release(&route_tracking_lock);
    return true;
}

/* 교차로 퇴장 처리 */
static void exit_intersection(route_id_t my_route)
{
    lock_acquire(&route_tracking_lock);
    active_routes[my_route].active = false;
    active_routes[my_route].step_count = 0;
    active_routes[my_route].vehicle_id = 0;
    active_routes[my_route].has_priority_signal = false;
    lock_release(&route_tracking_lock);
}

/* 스텝 카운트 업데이트 */
static void update_route_step(route_id_t my_route)
{
    lock_acquire(&route_tracking_lock);
    if (active_routes[my_route].active)
    {
        active_routes[my_route].step_count++;
        active_routes[my_route].distance_from_entry++;
    }
    lock_release(&route_tracking_lock);
}

/* 신호등 시스템 초기화 */
void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info)
{
    /* 스마트 우선순위 세마포어 */
    priority_sema_init(&intersection_semaphore, 8); // 예측 시스템으로 더 많은 동시 진입 가능

    /* 경로 추적 락 초기화 */
    lock_init(&route_tracking_lock);

    /* 모든 경로 비활성화 */
    for (int i = 0; i < ROUTE_COUNT; i++)
    {
        active_routes[i].active = false;
        active_routes[i].step_count = 0;
        active_routes[i].vehicle_id = 0;
        active_routes[i].vehicle_type = VEHICL_TYPE_NORMAL;
        active_routes[i].distance_from_entry = 0;
        active_routes[i].has_priority_signal = false;
    }

    /* 충돌 테이블 및 앰뷸런스 예측 시스템 초기화 */
    init_conflict_table();
    init_ambulance_prediction();

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

    /* 앰뷸런스인 경우 경로 등록 */
    if (vi->type == VEHICL_TYPE_AMBULANCE)
    {
        register_ambulance_route(vi->start, vi->dest, vi->arrival);
    }

    /* 개선된 진입 가능 여부 확인 */
    if (can_enter_intersection(my_route, vi->id, vi->type))
    {

        /* 차량 타입에 따른 우선순위 세마포어 처리 */
        if (vi->type == VEHICL_TYPE_AMBULANCE)
        {
            priority_sema_down_ambulance(&intersection_semaphore, vi->arrival, vi->golden_time);
        }
        else
        {
            // 우선 신호가 있는 일반차량도 빠른 처리
            if (active_routes[my_route].has_priority_signal)
            {
                // 세마포어 수정사항!!!
                intersection_semaphore.value--; // 직접 값 감소

                // priority_sema_down_normal(&intersection_semaphore); // 수정 필요!
            }
            else
            {
                priority_sema_down_normal(&intersection_semaphore);
            }
        }
        return true;
    }
    else
    {
        return false;
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

        priority_sema_up(&intersection_semaphore);
    }
}

/* 스텝 업데이트 */
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
            const char *type_icon = (active_routes[i].vehicle_type == VEHICL_TYPE_AMBULANCE) ? "🚑" : (active_routes[i].has_priority_signal) ? "🚨"
                                                                                                                                             : "🚗";
            const char *priority_text = active_routes[i].has_priority_signal ? "(우선신호)" : "";

            any_active = true;
        }
    }

    // 앰뷸런스 예측 정보 출력
    lock_acquire(&ambulance_prediction_lock);
    if (ambulance_prediction.is_active)
    {
        char amb_start = 'A' + (ambulance_prediction.ambulance_route / 4);
        char amb_dest = 'A' + (ambulance_prediction.ambulance_route % 4);
        int steps_remaining = ambulance_prediction.arrival_step - ambulance_prediction.current_step;
    }
    lock_release(&ambulance_prediction_lock);

    lock_release(&route_tracking_lock);
}