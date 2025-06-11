#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/vehicle.h"
#include "projects/crossroads/map.h"
#include "projects/crossroads/ats.h"
#include "projects/crossroads/priority_sync.h"
#include "projects/crossroads/blinker.h"

static struct lock step_lock;
static struct condition step_cond;

static int unfinished_vehicles = 0;
static int vehicles_reported_this_step = 0;
static int vehicles_moved_this_step = 0;
static struct vehicle_info *vehicle_list_ptr = NULL;
static int vehicle_count = 0;

// 앰뷸런스 우선순위를 위한 전역 상태
static bool emergency_mode = false;
static struct lock emergency_lock;

struct priority_sema road_entry_sema;

// 기존 blinker 시스템 (하위 호환성)
extern enum blinker_dir current_blinker_state;
extern struct lock blinker_lock;
extern struct condition blinker_cond;

/* path. A:0 B:1 C:2 D:3 */
const struct position vehicle_path[4][4][12] = {
	/* from A */ {
		/* to A */
		{
			{4, 0},
			{4, 1},
			{4, 2},
			{4, 3},
			{4, 4},
			{3, 4},
			{2, 4},
			{2, 3},
			{2, 2},
			{2, 1},
			{2, 0},
			{-1, -1},
		},
		/* to B */
		{
			{4, 0},
			{4, 1},
			{4, 2},
			{5, 2},
			{6, 2},
			{-1, -1},
		},
		/* to C */
		{
			{4, 0},
			{4, 1},
			{4, 2},
			{4, 3},
			{4, 4},
			{4, 5},
			{4, 6},
			{-1, -1},
		},
		/* to D */
		{
			{4, 0},
			{4, 1},
			{4, 2},
			{4, 3},
			{4, 4},
			{3, 4},
			{2, 4},
			{1, 4},
			{0, 4},
			{-1, -1},
		}},
	/* from B */ {/* to A */
				  {
					  {6, 4},
					  {5, 4},
					  {4, 4},
					  {3, 4},
					  {2, 4},
					  {2, 3},
					  {2, 2},
					  {2, 1},
					  {2, 0},
					  {-1, -1},
				  },
				  /* to B */
				  {
					  {6, 4},
					  {5, 4},
					  {4, 4},
					  {3, 4},
					  {2, 4},
					  {2, 3},
					  {2, 2},
					  {3, 2},
					  {4, 2},
					  {5, 2},
					  {6, 2},
					  {-1, -1},
				  },
				  /* to C */
				  {
					  {6, 4},
					  {5, 4},
					  {4, 4},
					  {4, 5},
					  {4, 6},
					  {-1, -1},
				  },
				  /* to D */
				  {
					  {6, 4},
					  {5, 4},
					  {4, 4},
					  {3, 4},
					  {2, 4},
					  {1, 4},
					  {0, 4},
					  {-1, -1},
				  }},
	/* from C */ {/* to A */
				  {
					  {2, 6},
					  {2, 5},
					  {2, 4},
					  {2, 3},
					  {2, 2},
					  {2, 1},
					  {2, 0},
					  {-1, -1},
				  },
				  /* to B */
				  {
					  {2, 6},
					  {2, 5},
					  {2, 4},
					  {2, 3},
					  {2, 2},
					  {3, 2},
					  {4, 2},
					  {5, 2},
					  {6, 2},
					  {-1, -1},
				  },
				  /* to C */
				  {
					  {2, 6},
					  {2, 5},
					  {2, 4},
					  {2, 3},
					  {2, 2},
					  {3, 2},
					  {4, 2},
					  {4, 3},
					  {4, 4},
					  {4, 5},
					  {4, 6},
					  {-1, -1},
				  },
				  /* to D */
				  {
					  {2, 6},
					  {2, 5},
					  {2, 4},
					  {1, 4},
					  {0, 4},
					  {-1, -1},
				  }},
	/* from D */ {/* to A */
				  {
					  {0, 2},
					  {1, 2},
					  {2, 2},
					  {2, 1},
					  {2, 0},
					  {-1, -1},
				  },
				  /* to B */
				  {
					  {0, 2},
					  {1, 2},
					  {2, 2},
					  {3, 2},
					  {4, 2},
					  {5, 2},
					  {6, 2},
					  {-1, -1},
				  },
				  /* to C */
				  {
					  {0, 2},
					  {1, 2},
					  {2, 2},
					  {3, 2},
					  {4, 2},
					  {4, 3},
					  {4, 4},
					  {4, 5},
					  {4, 6},
					  {-1, -1},
				  },
				  /* to D */
				  {
					  {0, 2},
					  {1, 2},
					  {2, 2},
					  {3, 2},
					  {4, 2},
					  {4, 3},
					  {4, 4},
					  {3, 4},
					  {2, 4},
					  {1, 4},
					  {0, 4},
					  {-1, -1},
				  }}};

void register_vehicle_list(struct vehicle_info *list, int count)
{
	vehicle_list_ptr = list;
	vehicle_count = count;
}

int is_position_outside(struct position pos)
{
	return (pos.row == -1 || pos.col == -1);
}

// 앰뷸런스를 위한 긴급 경로 확보
static bool try_emergency_clearance(struct vehicle_info *ambulance, struct position target_pos)
{
	for (int i = 0; i < vehicle_count; i++)
	{
		struct vehicle_info *blocking_vehicle = &vehicle_list_ptr[i];
		if (blocking_vehicle == ambulance || blocking_vehicle->state != VEHICLE_STATUS_RUNNING)
			continue;

		// 목표 위치에 있는 일반 차량 찾기
		if (blocking_vehicle->position.row == target_pos.row &&
			blocking_vehicle->position.col == target_pos.col &&
			blocking_vehicle->type == VEHICL_TYPE_NORMAL)
		{
			printf("[EMERGENCY] 🚨 Ambulance %c requesting emergency clearance from vehicle %c at (%d,%d)\n",
				   ambulance->id, blocking_vehicle->id, target_pos.row, target_pos.col);

			// 일반 차량을 안전한 인근 위치로 이동시킴
			struct position safe_positions[] = {
				{target_pos.row - 1, target_pos.col}, // 북쪽
				{target_pos.row + 1, target_pos.col}, // 남쪽
				{target_pos.row, target_pos.col - 1}, // 서쪽
				{target_pos.row, target_pos.col + 1}, // 동쪽
				{-1, -1}							  // 종료 마커
			};

			for (int j = 0; safe_positions[j].row != -1; j++)
			{
				struct position safe_pos = safe_positions[j];

				// 맵 범위 확인
				if (safe_pos.row < 0 || safe_pos.row >= 7 || safe_pos.col < 0 || safe_pos.col >= 7)
					continue;

				// 해당 위치가 비어있는지 확인
				bool position_free = true;
				for (int k = 0; k < vehicle_count; k++)
				{
					struct vehicle_info *other = &vehicle_list_ptr[k];
					if (other->state == VEHICLE_STATUS_RUNNING &&
						other->position.row == safe_pos.row && other->position.col == safe_pos.col)
					{
						position_free = false;
						break;
					}
				}

				if (position_free)
				{
					// 기존 위치 락 해제
					lock_release(&blocking_vehicle->map_locks[blocking_vehicle->position.row][blocking_vehicle->position.col]);

					// 새 위치 락 획득
					lock_acquire(&blocking_vehicle->map_locks[safe_pos.row][safe_pos.col]);

					// 차량 이동
					blocking_vehicle->position = safe_pos;

					printf("[EMERGENCY] ✅ Vehicle %c yielded to ambulance, moved to (%d,%d)\n",
						   blocking_vehicle->id, safe_pos.row, safe_pos.col);

					return true; // 성공적으로 길을 비움
				}
			}

			printf("[EMERGENCY] ⚠️ No safe position found for vehicle %c to yield\n", blocking_vehicle->id);
			return false;
		}
	}
	return false; // 차단 차량 없음
}

// 앰뷸런스 감지 및 응급 모드 설정
static void check_emergency_mode()
{
	lock_acquire(&emergency_lock);
	bool found_ambulance = false;

	for (int i = 0; i < vehicle_count; i++)
	{
		struct vehicle_info *vi = &vehicle_list_ptr[i];
		if (vi->state == VEHICLE_STATUS_RUNNING && vi->type == VEHICL_TYPE_AMBULANCE)
		{
			found_ambulance = true;
			break;
		}
	}

	if (found_ambulance != emergency_mode)
	{
		emergency_mode = found_ambulance;
		if (emergency_mode)
		{
			printf("[EMERGENCY] 🚨 Emergency mode activated - Ambulance detected!\n");
		}
		else
		{
			printf("[EMERGENCY] ✅ Emergency mode deactivated\n");
		}
	}

	lock_release(&emergency_lock);
}

// 현실적인 차량 이동 가능성 확인 (신호 + 교통 상황 + 앰뷸런스 우선순위 고려)
static bool can_realistically_move(struct vehicle_info *vi, struct position next_pos)
{
	// 1. 신호등 확인
	bool signal_ok = can_vehicle_pass_signal(vi->position, next_pos, vi);
	if (!signal_ok && vi->type != VEHICL_TYPE_AMBULANCE)
	{
		return false;
	}

	// 2. 앞차가 막혀있는지 확인
	bool path_blocked = false;
	struct vehicle_info *blocking_vehicle = NULL;

	for (int i = 0; i < vehicle_count; i++)
	{
		struct vehicle_info *other = &vehicle_list_ptr[i];
		if (other == vi || other->state != VEHICLE_STATUS_RUNNING)
			continue;

		// 다음 위치에 이미 다른 차량이 있으면 차단됨
		if (other->position.row == next_pos.row && other->position.col == next_pos.col)
		{
			path_blocked = true;
			blocking_vehicle = other;
			break;
		}
	}

	if (!path_blocked)
	{
		return true; // 길이 비어있음
	}

	// 3. 앰뷸런스인 경우 긴급 경로 확보 시도
	if (vi->type == VEHICL_TYPE_AMBULANCE && blocking_vehicle->type == VEHICL_TYPE_NORMAL)
	{
		return try_emergency_clearance(vi, next_pos);
	}

	// 4. 일반 차량은 차단되면 이동 불가
	return false;
}

// 강제 deadlock 해결 - 앰뷸런스 우선 처리
static void force_deadlock_resolution()
{
	printf("[DEADLOCK] 🔧 Attempting forced deadlock resolution...\n");

	// 1. 앰뷸런스 우선 처리
	for (int i = 0; i < vehicle_count; i++)
	{
		struct vehicle_info *ambulance = &vehicle_list_ptr[i];
		if (ambulance->state == VEHICLE_STATUS_RUNNING && ambulance->type == VEHICL_TYPE_AMBULANCE)
		{
			int start = ambulance->start - 'A';
			int dest = ambulance->dest - 'A';

			// 현재 step 찾기
			int current_step = 0;
			while (current_step < 12)
			{
				struct position path_pos = vehicle_path[start][dest][current_step];
				if (is_position_outside(path_pos))
					break;
				if (path_pos.row == ambulance->position.row && path_pos.col == ambulance->position.col)
					break;
				current_step++;
			}

			struct position next = vehicle_path[start][dest][current_step + 1];
			if (!is_position_outside(next))
			{
				// 앰뷸런스 강제 이동 시도
				if (try_emergency_clearance(ambulance, next))
				{
					printf("[DEADLOCK] ✅ Emergency clearance successful for ambulance %c\n", ambulance->id);
					return;
				}
			}
		}
	}

	// 2. 일반 차량들의 임시 위치 조정
	printf("[DEADLOCK] 🔄 Attempting vehicle position adjustment...\n");

	for (int i = 0; i < vehicle_count; i++)
	{
		struct vehicle_info *vi = &vehicle_list_ptr[i];
		if (vi->state == VEHICLE_STATUS_RUNNING)
		{
			// 차량을 임시로 안전한 위치로 이동
			struct position temp_positions[] = {
				{0, 0}, {0, 6}, {6, 0}, {6, 6}, // 맵 모서리
				{1, 1},
				{1, 5},
				{5, 1},
				{5, 5}, // 모서리 근처
				{-1, -1}};

			for (int j = 0; temp_positions[j].row != -1; j++)
			{
				struct position temp_pos = temp_positions[j];

				// 해당 위치가 비어있는지 확인
				bool position_free = true;
				for (int k = 0; k < vehicle_count; k++)
				{
					struct vehicle_info *other = &vehicle_list_ptr[k];
					if (other->state == VEHICLE_STATUS_RUNNING &&
						other->position.row == temp_pos.row && other->position.col == temp_pos.col)
					{
						position_free = false;
						break;
					}
				}

				if (position_free)
				{
					printf("[DEADLOCK] 🚗 Moving vehicle %c to temporary position (%d,%d)\n",
						   vi->id, temp_pos.row, temp_pos.col);

					// 기존 위치 락 해제
					if (!is_position_outside(vi->position))
					{
						lock_release(&vi->map_locks[vi->position.row][vi->position.col]);
					}

					// 새 위치 락 획득
					lock_acquire(&vi->map_locks[temp_pos.row][temp_pos.col]);

					// 차량 이동
					vi->position = temp_pos;
					return;
				}
			}
		}
	}

	printf("[DEADLOCK] ⚠️ Failed to resolve deadlock automatically\n");
}

// 강화된 deadlock 해결을 위한 차량 이동 가능성 확인
static bool no_movable_vehicles()
{
	printf("[DEBUG] Checking if any vehicles can move...\n");

	for (int i = 0; i < vehicle_count; i++)
	{
		struct vehicle_info *vi = &vehicle_list_ptr[i];
		if (vi->state == VEHICLE_STATUS_RUNNING)
		{
			int start = vi->start - 'A';
			int dest = vi->dest - 'A';

			// 현재 step 찾기
			int current_step = 0;
			while (current_step < 12)
			{
				struct position path_pos = vehicle_path[start][dest][current_step];
				if (is_position_outside(path_pos))
					break;
				if (path_pos.row == vi->position.row && path_pos.col == vi->position.col)
				{
					break;
				}
				current_step++;
			}

			struct position next = vehicle_path[start][dest][current_step + 1];
			if (!is_position_outside(next))
			{
				// 현실적인 이동 가능성 확인 (양보 메커니즘 포함)
				if (can_realistically_move(vi, next))
				{
					printf("[DEBUG] Vehicle %c can move from (%d,%d) to (%d,%d)\n",
						   vi->id, vi->position.row, vi->position.col, next.row, next.col);
					return false; // 움직일 수 있는 차량이 있음
				}
				else
				{
					printf("[DEBUG] Vehicle %c blocked at (%d,%d), cannot move to (%d,%d)\n",
						   vi->id, vi->position.row, vi->position.col, next.row, next.col);
				}
			}
			else
			{
				printf("[DEBUG] Vehicle %c at (%d,%d) has reached destination\n",
					   vi->id, vi->position.row, vi->position.col);
			}
		}
	}
	printf("[DEBUG] No vehicles can move - deadlock detected\n");
	return true; // 아무도 움직일 수 없음
}

// 향상된 우선순위 차량 확인 (앰뷸런스 고려)
bool is_blocking_higher_priority_vehicle(struct vehicle_info *me)
{
	for (int i = 0; i < vehicle_count; i++)
	{
		struct vehicle_info *other = &vehicle_list_ptr[i];

		if (other == me)
			continue;
		if (other->state != VEHICLE_STATUS_RUNNING)
			continue;

		// priority: ambulance(1) > normal(0)
		if (other->type <= me->type)
			continue;

		int s = other->start - 'A';
		int d = other->dest - 'A';
		int step = 0;

		// 다른 차량의 경로상에서 내가 막고 있는지 확인
		while (step < 12)
		{
			struct position next = vehicle_path[s][d][step];
			if (is_position_outside(next))
				break;

			// 우선순위 차량의 경로에 내가 있으면 양보해야 함
			if (next.row == me->position.row && next.col == me->position.col)
			{
				if (other->type == VEHICL_TYPE_AMBULANCE)
				{
					printf("[PRIORITY] 🚗 Vehicle %c yielding to ambulance %c at (%d,%d)\n",
						   me->id, other->id, me->position.row, me->position.col);
				}
				return true;
			}
			step++;
		}
	}
	return false;
}

static int try_move(int start, int dest, int step, struct vehicle_info *vi)
{
	struct position pos_cur = vi->position;
	struct position pos_next = vehicle_path[start][dest][step];

	if (is_position_outside(pos_next))
	{
		if (vi->state == VEHICLE_STATUS_RUNNING)
		{
			vi->position.row = vi->position.col = -1;
			if (!is_position_outside(pos_cur))
			{
				lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
			}

			if (vi->type == VEHICL_TYPE_AMBULANCE)
			{
				printf("[ARRIVAL] 🚑 Ambulance %c arrived at destination %c\n", vi->id, vi->dest);
			}
			else
			{
				printf("[ARRIVAL] 🚗 Vehicle %c arrived at destination %c\n", vi->id, vi->dest);
			}
			return 0; // 정상 종료
		}
		return -1; // 잘못된 경로
	}

	// 다음 위치 락 획득
	lock_acquire(&vi->map_locks[pos_next.row][pos_next.col]);

	if (vi->state == VEHICLE_STATUS_READY)
	{
		vi->state = VEHICLE_STATUS_RUNNING;
		if (vi->type == VEHICL_TYPE_AMBULANCE)
		{
			printf("[START] 🚑 Ambulance %c started journey %c→%c\n", vi->id, vi->start, vi->dest);
		}
		else
		{
			printf("[START] 🚗 Vehicle %c started journey %c→%c\n", vi->id, vi->start, vi->dest);
		}
	}
	else
	{
		// 이전 위치 락 해제
		if (!is_position_outside(pos_cur))
		{
			lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
		}
	}

	vi->position = pos_next;

	// 이동 로그 (교차로나 중요 지점에서만)
	if (is_intersection_coord(pos_next) || vi->type == VEHICL_TYPE_AMBULANCE)
	{
		if (vi->type == VEHICL_TYPE_AMBULANCE)
		{
			printf("[MOVE] 🚑 Ambulance %c → (%d,%d)\n", vi->id, pos_next.row, pos_next.col);
		}
		else if (is_intersection_coord(pos_next))
		{
			printf("[MOVE] 🚗 Vehicle %c → intersection (%d,%d)\n", vi->id, pos_next.row, pos_next.col);
		}
	}

	return 1; // 이동 성공
}

void parse_vehicles(struct vehicle_info *vehicle_info, char *input)
{
	int idx = 0;
	char *token, *save_ptr;

	for (token = strtok_r(input, ":", &save_ptr); token != NULL; token = strtok_r(NULL, ":", &save_ptr))
	{
		int len = strlen(token);
		char id = token[0];
		char start = token[1];
		char dest = token[2];

		int arrival = 0, golden_time = -1;
		int type = VEHICL_TYPE_NORMAL;

		if (len > 3)
		{
			char *time_part = &token[3];
			char *dot = strchr(time_part, '.');
			if (dot)
			{
				*dot = '\0';
				arrival = atoi(time_part);
				golden_time = atoi(dot + 1);
				type = VEHICL_TYPE_AMBULANCE;
			}
		}

		vehicle_info[idx].id = id;
		vehicle_info[idx].start = start;
		vehicle_info[idx].dest = dest;
		vehicle_info[idx].type = type;
		vehicle_info[idx].arrival = arrival;
		vehicle_info[idx].golden_time = golden_time;
		vehicle_info[idx].state = VEHICLE_STATUS_READY;
		vehicle_info[idx].position.row = -1;
		vehicle_info[idx].position.col = -1;
		idx++;
	}

	register_vehicle_list(vehicle_info, idx);
}

void init_on_mainthread(int thread_cnt)
{
	unfinished_vehicles = thread_cnt;
	lock_init(&step_lock);
	cond_init(&step_cond);
	lock_init(&emergency_lock);
	priority_sema_init(&road_entry_sema, thread_cnt);

	printf("[SYSTEM] 🚦 Realistic traffic simulation initialized\n");
	printf("[SYSTEM] 🚑 Emergency vehicle priority system enabled\n");
}

void vehicle_loop(void *_vi)
{
	struct vehicle_info *vi = _vi;
	vi->t = thread_current();

	int start = vi->start - 'A';
	int dest = vi->dest - 'A';
	int step = 0;
	vi->position.row = vi->position.col = -1;
	vi->state = VEHICLE_STATUS_READY;

	// 진입 전 우선순위 세마포어 획득
	priority_sema_down(&road_entry_sema, vi);

	while (1)
	{
		bool moved = false;

		// 응급 모드 확인
		check_emergency_mode();

		if (crossroads_step < vi->arrival)
			goto move_sync;

		if (vi->state == VEHICLE_STATUS_FINISHED)
			goto move_sync;

		// === 핵심: 현실적인 이동 가능성 확인 ===
		struct position next_pos = vehicle_path[start][dest][step];

		// 1. 신호등 및 교통 상황 확인
		bool can_move = can_realistically_move(vi, next_pos);

		if (!can_move)
		{
			goto move_sync;
		}

		// 2. 우선순위 차량 확인 (앰뷸런스 우선순위)
		if (is_blocking_higher_priority_vehicle(vi))
		{
			goto move_sync;
		}

		// 3. 실제 이동 시도
		int res = try_move(start, dest, step, vi);

		if (res == 1)
		{
			step++;
			moved = true;
		}
		else if (res == 0)
		{
			// 도착 완료
			vi->state = VEHICLE_STATUS_FINISHED;

			lock_acquire(&step_lock);
			unfinished_vehicles--;
			vehicles_reported_this_step++;

			if (vehicles_reported_this_step == unfinished_vehicles)
			{
				vehicles_reported_this_step = 0;
				vehicles_moved_this_step = 0;
				crossroads_step++;
				unitstep_changed();

				// 기존 blinker 시스템에 알림
				lock_acquire(&blinker_lock);
				cond_broadcast(&blinker_cond, &blinker_lock);
				lock_release(&blinker_lock);

				cond_broadcast(&step_cond, &step_lock);
			}
			else
			{
				cond_wait(&step_cond, &step_lock);
			}
			lock_release(&step_lock);

			priority_sema_up(&road_entry_sema); // 도착 후 해제
			break;
		}

	move_sync:
		lock_acquire(&step_lock);
		vehicles_reported_this_step++;
		if (moved)
			vehicles_moved_this_step++;

		if (vehicles_reported_this_step == unfinished_vehicles)
		{
			// Deadlock 방지: 아무도 움직일 수 없는 상황
			if (vehicles_moved_this_step == 0 && no_movable_vehicles())
			{
				printf("[DEADLOCK] ⚠️  No vehicles can move at step %d → attempting resolution\n", crossroads_step);

				// 응급 상황에서는 더 적극적인 해결책 시도
				if (emergency_mode)
				{
					printf("[EMERGENCY] 🚨 Emergency deadlock resolution activated\n");
				}

				// 실제 deadlock 해결 시도
				force_deadlock_resolution();
			}

			vehicles_reported_this_step = 0;
			vehicles_moved_this_step = 0;
			crossroads_step++;
			unitstep_changed();

			// 기존 blinker 시스템에 알림
			lock_acquire(&blinker_lock);
			cond_broadcast(&blinker_cond, &blinker_lock);
			lock_release(&blinker_lock);

			cond_broadcast(&step_cond, &step_lock);
		}
		else
		{
			cond_wait(&step_cond, &step_lock);
		}

		lock_release(&step_lock);
	}
}
