#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/blinker.h"
#include "projects/crossroads/crossroads.h"
#include <stdio.h>
#include <string.h>

// 기존 변수들 (하위 호환성)
enum blinker_dir current_blinker_state;
struct condition blinker_cond;
struct lock blinker_lock;

// 새로운 고급 신호등 시스템
struct advanced_signal_system global_signal_system;

static int last_step = -1;

// 교차로 위치들 (이미지 기반 - 7x7 맵의 교차로 영역)
static const struct position intersection_coords[] = {
    {2, 3}, {3, 2}, {3, 3}, {3, 4}, {4, 3}, // 중앙 교차로 영역
    {-1, -1}                                // 종료 마커
};

// 신호 패턴들 (이미지의 6가지 시나리오)
static struct signal_pattern signal_patterns[] = {
    // Phase 0: 동서방향 직진 (그림 1) - 3스텝 지속
    {0, 3, {}},
    // Phase 1: 남북방향 직진 (그림 2) - 3스텝 지속
    {1, 3, {}},
    // Phase 2: 동서방향 좌회전 (그림 3) - 2스텝 지속
    {2, 2, {}},
    // Phase 3: 남북방향 좌회전 (그림 4) - 2스텝 지속
    {3, 2, {}},
    // Phase 4: 복합 이동 1 (그림 5) - 2스텝 지속
    {4, 2, {}},
    // Phase 5: 복합 이동 2 (그림 6) - 2스텝 지속
    {5, 2, {}}};

static const int NUM_SIGNAL_PHASES = sizeof(signal_patterns) / sizeof(signal_patterns[0]);

direction_t get_move_direction(struct position from, struct position to)
{
    if (to.row < from.row)
        return DIR_NORTH;
    if (to.row > from.row)
        return DIR_SOUTH;
    if (to.col > from.col)
        return DIR_EAST;
    if (to.col < from.col)
        return DIR_WEST;
    return DIR_EAST; // 기본값
}

bool is_intersection_coord(struct position pos)
{
    for (int i = 0; intersection_coords[i].row != -1; i++)
    {
        if (intersection_coords[i].row == pos.row &&
            intersection_coords[i].col == pos.col)
        {
            return true;
        }
    }
    return false;
}

static void init_signal_patterns()
{
    // 모든 신호를 RED로 초기화
    for (int p = 0; p < NUM_SIGNAL_PHASES; p++)
    {
        for (int r = 0; r < 7; r++)
        {
            for (int c = 0; c < 7; c++)
            {
                for (int d = 0; d < DIR_COUNT; d++)
                {
                    signal_patterns[p].pattern[r][c][d] = SIGNAL_RED;
                }
            }
        }
    }

    // Phase 0: 동서방향 직진 허용 (그림 1)
    signal_patterns[0].pattern[3][2][DIR_EAST] = SIGNAL_GREEN; // 서→동 진입
    signal_patterns[0].pattern[3][3][DIR_EAST] = SIGNAL_GREEN; // 중앙 통과 (동쪽으로)
    signal_patterns[0].pattern[3][4][DIR_WEST] = SIGNAL_GREEN; // 동→서 진입
    signal_patterns[0].pattern[3][3][DIR_WEST] = SIGNAL_GREEN; // 중앙 통과 (서쪽으로)
    signal_patterns[0].pattern[4][3][DIR_EAST] = SIGNAL_GREEN;
    signal_patterns[0].pattern[2][3][DIR_WEST] = SIGNAL_GREEN;

    // Phase 1: 남북방향 직진 허용 (그림 2)
    signal_patterns[1].pattern[2][3][DIR_SOUTH] = SIGNAL_GREEN; // 북→남 진입
    signal_patterns[1].pattern[3][3][DIR_SOUTH] = SIGNAL_GREEN; // 중앙 통과 (남쪽으로)
    signal_patterns[1].pattern[4][3][DIR_NORTH] = SIGNAL_GREEN; // 남→북 진입
    signal_patterns[1].pattern[3][3][DIR_NORTH] = SIGNAL_GREEN; // 중앙 통과 (북쪽으로)
    signal_patterns[1].pattern[3][4][DIR_NORTH] = SIGNAL_GREEN;

    // Phase 2: 동서방향에서 좌회전 (그림 3)
    signal_patterns[2].pattern[3][2][DIR_SOUTH] = SIGNAL_GREEN; // 서쪽에서 남쪽으로 좌회전
    signal_patterns[2].pattern[3][3][DIR_SOUTH] = SIGNAL_GREEN; // 중앙에서 남쪽으로
    signal_patterns[2].pattern[3][4][DIR_NORTH] = SIGNAL_GREEN; // 동쪽에서 북쪽으로 좌회전
    signal_patterns[2].pattern[3][3][DIR_NORTH] = SIGNAL_GREEN; // 중앙에서 북쪽으로

    // Phase 3: 남북방향에서 좌회전 (그림 4)
    signal_patterns[3].pattern[2][3][DIR_EAST] = SIGNAL_GREEN; // 북쪽에서 동쪽으로 좌회전
    signal_patterns[3].pattern[3][3][DIR_EAST] = SIGNAL_GREEN; // 중앙에서 동쪽으로
    signal_patterns[3].pattern[4][3][DIR_WEST] = SIGNAL_GREEN; // 남쪽에서 서쪽으로 좌회전
    signal_patterns[3].pattern[3][3][DIR_WEST] = SIGNAL_GREEN; // 중앙에서 서쪽으로

    // Phase 4: 복합 패턴 1 (그림 5) - 북→남 직진 + 동→서 직진
    signal_patterns[4].pattern[2][3][DIR_SOUTH] = SIGNAL_GREEN; // 북→남 직진
    signal_patterns[4].pattern[3][3][DIR_SOUTH] = SIGNAL_GREEN; // 중앙 통과
    signal_patterns[4].pattern[3][4][DIR_WEST] = SIGNAL_GREEN;  // 동→서 직진
    signal_patterns[4].pattern[3][3][DIR_WEST] = SIGNAL_GREEN;  // 중앙 통과

    // Phase 5: 복합 패턴 2 (그림 6) - 남→북 직진 + 서→동 직진
    signal_patterns[5].pattern[4][3][DIR_NORTH] = SIGNAL_GREEN; // 남→북 직진
    signal_patterns[5].pattern[3][3][DIR_NORTH] = SIGNAL_GREEN; // 중앙 통과
    signal_patterns[5].pattern[3][2][DIR_EAST] = SIGNAL_GREEN;  // 서→동 직진
    signal_patterns[5].pattern[3][3][DIR_EAST] = SIGNAL_GREEN;  // 중앙 통과

    signal_patterns[1].pattern[3][2][DIR_SOUTH] = SIGNAL_GREEN;
}

static void update_advanced_signals()
{
    lock_acquire(&global_signal_system.signal_lock);

    struct signal_pattern *current = &signal_patterns[global_signal_system.current_phase];

    // 현재 페이즈의 신호 패턴을 시스템에 적용
    for (int r = 0; r < 7; r++)
    {
        for (int c = 0; c < 7; c++)
        {
            if (is_intersection_coord((struct position){r, c}))
            {
                for (int d = 0; d < DIR_COUNT; d++)
                {
                    global_signal_system.intersections[r][c].directions[d] =
                        current->pattern[r][c][d];
                }
                global_signal_system.intersections[r][c].phase = global_signal_system.current_phase;
            }
        }
    }

    printf("[ADVANCED SIGNAL] Phase %d activated (duration: %d steps)\n",
           global_signal_system.current_phase, current->duration_steps);

    // 신호 변경 알림
    cond_broadcast(&global_signal_system.signal_cond, &global_signal_system.signal_lock);

    lock_release(&global_signal_system.signal_lock);
}

bool can_vehicle_pass_signal(struct position from, struct position to, struct vehicle_info *vehicle)
{
    // 시작 위치(-1, -1)에서의 이동은 항상 허용
    if (from.row == -1 || from.col == -1)
    {
        return true;
    }

    // 목적지가 맵 범위를 벗어나면 (도착) 허용
    if (to.row < 0 || to.row >= 7 || to.col < 0 || to.col >= 7)
    {
        return true;
    }

    // from 위치가 교차로가 아니면 자유 이동
    if (!is_intersection_coord(from))
    {
        return true;
    }

    lock_acquire(&global_signal_system.signal_lock);

    direction_t move_dir = get_move_direction(from, to);
    signal_state_t signal = global_signal_system.intersections[from.row][from.col].directions[move_dir];

    lock_release(&global_signal_system.signal_lock);

    bool can_pass = (signal == SIGNAL_GREEN);

    return can_pass;
}

void init_blinker(struct blinker_info *blinkers, struct lock **map_locks, struct vehicle_info *vehicle_info)
{
    // 기존 blinker 시스템 초기화 (하위 호환성)
    for (int i = 0; i < NUM_BLINKER; i++)
    {
        blinkers[i].map_locks = map_locks;
        blinkers[i].vehicles = vehicle_info;
    }

    lock_init(&blinker_lock);
    cond_init(&blinker_cond);
    current_blinker_state = BLINKER_EW; // 기본값

    // 새로운 고급 신호등 시스템 초기화
    lock_init(&global_signal_system.signal_lock);
    cond_init(&global_signal_system.signal_cond);
    global_signal_system.current_phase = 0;
    global_signal_system.phase_step_count = 0;

    // 모든 교차로 신호를 RED로 초기화
    for (int r = 0; r < 7; r++)
    {
        for (int c = 0; c < 7; c++)
        {
            for (int d = 0; d < DIR_COUNT; d++)
            {
                global_signal_system.intersections[r][c].directions[d] = SIGNAL_RED;
            }
            global_signal_system.intersections[r][c].phase = 0;
        }
    }

    // 신호 패턴 초기화 및 첫 페이즈 적용
    init_signal_patterns();
    update_advanced_signals();

    printf("[BLINKER] Advanced signal system initialized with %d phases\n", NUM_SIGNAL_PHASES);
}

static void blinker_thread(void *aux UNUSED)
{
    last_step = crossroads_step;

    while (true)
    {
        lock_acquire(&blinker_lock);

        // unit step이 바뀔 때까지 기다림
        while (last_step == crossroads_step)
            cond_wait(&blinker_cond, &blinker_lock);

        last_step = crossroads_step;

        // 기존 simple blinker 상태 업데이트 (하위 호환성)
        current_blinker_state = (current_blinker_state == BLINKER_EW) ? BLINKER_NS : BLINKER_EW;

        lock_release(&blinker_lock);

        // 고급 신호등 시스템 업데이트
        lock_acquire(&global_signal_system.signal_lock);
        global_signal_system.phase_step_count++;

        // 현재 페이즈 지속 시간이 끝났는지 확인
        if (global_signal_system.phase_step_count >=
            signal_patterns[global_signal_system.current_phase].duration_steps)
        {

            // 다음 페이즈로 전환
            global_signal_system.current_phase =
                (global_signal_system.current_phase + 1) % NUM_SIGNAL_PHASES;
            global_signal_system.phase_step_count = 0;

            lock_release(&global_signal_system.signal_lock);
            update_advanced_signals();
        }
        else
        {
            lock_release(&global_signal_system.signal_lock);
        }

        // 기존 호환성을 위한 브로드캐스트
        lock_acquire(&blinker_lock);
        cond_broadcast(&blinker_cond, &blinker_lock);
        lock_release(&blinker_lock);
    }
}

void start_blinker()
{
    thread_create("blinker", PRI_DEFAULT + 1, blinker_thread, NULL);
    printf("[BLINKER] Advanced blinker thread started\n");
}
