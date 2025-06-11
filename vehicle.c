// #include <stdio.h>
// #include <string.h>
// #include <ctype.h>
// #include <stdlib.h>
// #include <stdbool.h>

// #include "threads/thread.h"
// #include "threads/synch.h"
// #include "projects/crossroads/vehicle.h"
// #include "projects/crossroads/map.h"
// #include "projects/crossroads/ats.h"
// #include "projects/crossroads/priority_sync.h"
// #include "projects/crossroads/blinker.h"

// static struct lock step_lock;
// static struct condition step_cond;

// static int unfinished_vehicles = 0;
// static int vehicles_reported_this_step = 0;
// static int vehicles_moved_this_step = 0;
// static struct vehicle_info *vehicle_list_ptr = NULL;
// static int vehicle_count = 0;

// struct priority_sema road_entry_sema;

// extern enum blinker_dir current_blinker_state;
// extern struct lock blinker_lock;
// extern struct condition blinker_cond;

// /* path. A:0 B:1 C:2 D:3 */
// const struct position vehicle_path[4][4][12] = {
// 	/* from A */ {
// 		/* to A */
// 		{
// 			{4, 0},
// 			{4, 1},
// 			{4, 2},
// 			{4, 3},
// 			{4, 4},
// 			{3, 4},
// 			{2, 4},
// 			{2, 3},
// 			{2, 2},
// 			{2, 1},
// 			{2, 0},
// 			{-1, -1},
// 		},
// 		/* to B */
// 		{
// 			{4, 0},
// 			{4, 1},
// 			{4, 2},
// 			{5, 2},
// 			{6, 2},
// 			{-1, -1},
// 		},
// 		/* to C */
// 		{
// 			{4, 0},
// 			{4, 1},
// 			{4, 2},
// 			{4, 3},
// 			{4, 4},
// 			{4, 5},
// 			{4, 6},
// 			{-1, -1},
// 		},
// 		/* to D */
// 		{
// 			{4, 0},
// 			{4, 1},
// 			{4, 2},
// 			{4, 3},
// 			{4, 4},
// 			{3, 4},
// 			{2, 4},
// 			{1, 4},
// 			{0, 4},
// 			{-1, -1},
// 		}},
// 	/* from B */ {/* to A */
// 				  {
// 					  {6, 4},
// 					  {5, 4},
// 					  {4, 4},
// 					  {3, 4},
// 					  {2, 4},
// 					  {2, 3},
// 					  {2, 2},
// 					  {2, 1},
// 					  {2, 0},
// 					  {-1, -1},
// 				  },
// 				  /* to B */
// 				  {
// 					  {6, 4},
// 					  {5, 4},
// 					  {4, 4},
// 					  {3, 4},
// 					  {2, 4},
// 					  {2, 3},
// 					  {2, 2},
// 					  {3, 2},
// 					  {4, 2},
// 					  {5, 2},
// 					  {6, 2},
// 					  {-1, -1},
// 				  },
// 				  /* to C */
// 				  {
// 					  {6, 4},
// 					  {5, 4},
// 					  {4, 4},
// 					  {4, 5},
// 					  {4, 6},
// 					  {-1, -1},
// 				  },
// 				  /* to D */
// 				  {
// 					  {6, 4},
// 					  {5, 4},
// 					  {4, 4},
// 					  {3, 4},
// 					  {2, 4},
// 					  {1, 4},
// 					  {0, 4},
// 					  {-1, -1},
// 				  }},
// 	/* from C */ {/* to A */
// 				  {
// 					  {2, 6},
// 					  {2, 5},
// 					  {2, 4},
// 					  {2, 3},
// 					  {2, 2},
// 					  {2, 1},
// 					  {2, 0},
// 					  {-1, -1},
// 				  },
// 				  /* to B */
// 				  {
// 					  {2, 6},
// 					  {2, 5},
// 					  {2, 4},
// 					  {2, 3},
// 					  {2, 2},
// 					  {3, 2},
// 					  {4, 2},
// 					  {5, 2},
// 					  {6, 2},
// 					  {-1, -1},
// 				  },
// 				  /* to C */
// 				  {
// 					  {2, 6},
// 					  {2, 5},
// 					  {2, 4},
// 					  {2, 3},
// 					  {2, 2},
// 					  {3, 2},
// 					  {4, 2},
// 					  {4, 3},
// 					  {4, 4},
// 					  {4, 5},
// 					  {4, 6},
// 					  {-1, -1},
// 				  },
// 				  /* to D */
// 				  {
// 					  {2, 6},
// 					  {2, 5},
// 					  {2, 4},
// 					  {1, 4},
// 					  {0, 4},
// 					  {-1, -1},
// 				  }},
// 	/* from D */ {/* to A */
// 				  {
// 					  {0, 2},
// 					  {1, 2},
// 					  {2, 2},
// 					  {2, 1},
// 					  {2, 0},
// 					  {-1, -1},
// 				  },
// 				  /* to B */
// 				  {
// 					  {0, 2},
// 					  {1, 2},
// 					  {2, 2},
// 					  {3, 2},
// 					  {4, 2},
// 					  {5, 2},
// 					  {6, 2},
// 					  {-1, -1},
// 				  },
// 				  /* to C */
// 				  {
// 					  {0, 2},
// 					  {1, 2},
// 					  {2, 2},
// 					  {3, 2},
// 					  {4, 2},
// 					  {4, 3},
// 					  {4, 4},
// 					  {4, 5},
// 					  {4, 6},
// 					  {-1, -1},
// 				  },
// 				  /* to D */
// 				  {
// 					  {0, 2},
// 					  {1, 2},
// 					  {2, 2},
// 					  {3, 2},
// 					  {4, 2},
// 					  {4, 3},
// 					  {4, 4},
// 					  {3, 4},
// 					  {2, 4},
// 					  {1, 4},
// 					  {0, 4},
// 					  {-1, -1},
// 				  }}};

// void register_vehicle_list(struct vehicle_info *list, int count)
// {
// 	vehicle_list_ptr = list;
// 	vehicle_count = count;
// }

// // 현재 위치에서 다음 위치로의 이동 방향을 판단하는 함수
// static enum blinker_dir get_movement_direction(struct position current, struct position next)
// {
// 	// 동서방향 이동 (좌우)
// 	if (current.row == next.row && current.col != next.col)
// 	{
// 		return BLINKER_EW;
// 	}
// 	// 남북방향 이동 (상하)
// 	else if (current.row != next.row && current.col == next.col)
// 	{
// 		return BLINKER_NS;
// 	}
// 	// 대각선이나 같은 위치인 경우 (일반적으로 발생하지 않음)
// 	else
// 	{
// 		return BLINKER_EW; // 기본값
// 	}
// }

// // 수정된 can_pass 함수 - 실제 이동 방향 기반
// static bool can_pass_by_direction(enum blinker_dir current_signal,
// 								  struct position current_pos,
// 								  struct position next_pos)
// {
// 	// 차량이 아직 도로에 진입하지 않은 경우 (시작 위치)
// 	if (current_pos.row == -1 || current_pos.col == -1)
// 	{
// 		return true; // 첫 진입은 항상 허용
// 	}

// 	// 현재 이동하려는 방향 확인
// 	enum blinker_dir movement_dir = get_movement_direction(current_pos, next_pos);

// 	// 신호가 이동 방향과 일치하는지 확인
// 	return (current_signal == movement_dir);
// }

// int is_position_outside(struct position pos)
// {
// 	return (pos.row == -1 || pos.col == -1);
// }

// // 수정된 no_movable_vehicles 함수 - 실제 이동 방향 기반
// static bool no_movable_vehicles()
// {
// 	for (int i = 0; i < vehicle_count; i++)
// 	{
// 		struct vehicle_info *vi = &vehicle_list_ptr[i];
// 		if (vi->state == VEHICLE_STATUS_RUNNING)
// 		{
// 			int start = vi->start - 'A';
// 			int dest = vi->dest - 'A';

// 			// 현재 step 찾기
// 			int current_step = 0;
// 			while (current_step < 12)
// 			{
// 				struct position path_pos = vehicle_path[start][dest][current_step];
// 				if (is_position_outside(path_pos))
// 					break;
// 				if (path_pos.row == vi->position.row && path_pos.col == vi->position.col)
// 				{
// 					break;
// 				}
// 				current_step++;
// 			}

// 			struct position next = vehicle_path[start][dest][current_step + 1];
// 			if (!is_position_outside(next))
// 			{
// 				// 실제 이동 방향 기반으로 확인
// 				if (can_pass_by_direction(current_blinker_state, vi->position, next))
// 				{
// 					return false; // 움직일 수 있는 차량이 있음
// 				}
// 			}
// 		}
// 	}
// 	return true; // 아무도 움직일 수 없음
// }

// bool is_blocking_higher_priority_vehicle(struct vehicle_info *me)
// {
// 	for (int i = 0; i < vehicle_count; i++)
// 	{
// 		struct vehicle_info *other = &vehicle_list_ptr[i];

// 		if (other == me)
// 			continue;
// 		if (other->state != VEHICLE_STATUS_RUNNING)
// 			continue;

// 		// priority: ambulance(1) > normal(0)
// 		if (other->type <= me->type)
// 			continue;

// 		int s = other->start - 'A';
// 		int d = other->dest - 'A';
// 		int step = 0;

// 		// 다른 차량의 현재 step 계산
// 		struct position curr = other->position;
// 		struct position next;

// 		while (true)
// 		{
// 			next = vehicle_path[s][d][step];
// 			if (is_position_outside(next))
// 				break;

// 			// 아직 도달하지 않은 차량의 다음 위치 중 내가 막고 있는지 확인
// 			if (next.row == me->position.row && next.col == me->position.col)
// 				return true;

// 			step++;
// 		}
// 	}
// 	return false;
// }

// static int try_move(int start, int dest, int step, struct vehicle_info *vi)
// {
// 	struct position pos_cur = vi->position;
// 	struct position pos_next = vehicle_path[start][dest][step];

// 	if (is_position_outside(pos_next))
// 	{
// 		if (vi->state == VEHICLE_STATUS_RUNNING)
// 		{
// 			vi->position.row = vi->position.col = -1;
// 			lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
// 			return 0; // 정상 종료
// 		}
// 		return -1; // 잘못된 경로
// 	}

// 	lock_acquire(&vi->map_locks[pos_next.row][pos_next.col]);

// 	if (vi->state == VEHICLE_STATUS_READY)
// 	{
// 		vi->state = VEHICLE_STATUS_RUNNING;
// 	}
// 	else
// 	{
// 		lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
// 	}

// 	vi->position = pos_next;
// 	return 1; // 이동 성공
// }

// void parse_vehicles(struct vehicle_info *vehicle_info, char *input)
// {
// 	int idx = 0;
// 	char *token, *save_ptr;

// 	for (token = strtok_r(input, ":", &save_ptr); token != NULL; token = strtok_r(NULL, ":", &save_ptr))
// 	{
// 		int len = strlen(token);
// 		char id = token[0];
// 		char start = token[1];
// 		char dest = token[2];

// 		int arrival = 0, golden_time = -1;
// 		int type = VEHICL_TYPE_NORMAL;

// 		if (len > 3)
// 		{
// 			char *time_part = &token[3];
// 			char *dot = strchr(time_part, '.');
// 			if (dot)
// 			{
// 				*dot = '\0';
// 				arrival = atoi(time_part);
// 				golden_time = atoi(dot + 1);
// 				type = VEHICL_TYPE_AMBULANCE;
// 			}
// 		}

// 		vehicle_info[idx].id = id;
// 		vehicle_info[idx].start = start;
// 		vehicle_info[idx].dest = dest;
// 		vehicle_info[idx].type = type;
// 		vehicle_info[idx].arrival = arrival;
// 		vehicle_info[idx].golden_time = golden_time;
// 		vehicle_info[idx].state = VEHICLE_STATUS_READY;
// 		vehicle_info[idx].position.row = -1;
// 		vehicle_info[idx].position.col = -1;
// 		idx++;
// 	}

// 	register_vehicle_list(vehicle_info, idx);
// }

// void init_on_mainthread(int thread_cnt)
// {
// 	unfinished_vehicles = thread_cnt;
// 	lock_init(&step_lock);
// 	cond_init(&step_cond);
// 	priority_sema_init(&road_entry_sema, thread_cnt);
// }

// void vehicle_loop(void *_vi)
// {
// 	struct vehicle_info *vi = _vi;
// 	vi->t = thread_current();

// 	int start = vi->start - 'A';
// 	int dest = vi->dest - 'A';
// 	int step = 0;
// 	vi->position.row = vi->position.col = -1;
// 	vi->state = VEHICLE_STATUS_READY;

// 	// 진입 전 우선순위 세마포어 획득
// 	priority_sema_down(&road_entry_sema, vi);

// 	while (1)
// 	{
// 		bool moved = false;

// 		if (crossroads_step < vi->arrival)
// 			goto move_sync;

// 		if (vi->state == VEHICLE_STATUS_FINISHED)
// 			goto move_sync;

// 		// === 핵심 수정 부분: 실제 이동 방향 기반 신호 확인 ===
// 		struct position next_pos = vehicle_path[start][dest][step];

// 		lock_acquire(&blinker_lock);
// 		bool can_move = can_pass_by_direction(current_blinker_state, vi->position, next_pos);
// 		lock_release(&blinker_lock);

// 		if (!can_move)
// 		{
// 			// 디버그 출력 (원하면 주석 처리)
// 			if (vi->position.row != -1 && vi->position.col != -1)
// 			{ // 이미 도로에 진입한 경우만
// 				enum blinker_dir needed_dir = get_movement_direction(vi->position, next_pos);
// 			}
// 			goto move_sync;
// 		}
// 		// === 수정 부분 끝 ===

// 		if (is_blocking_higher_priority_vehicle(vi))
// 		{
// 			goto move_sync;
// 		}

// 		int res = try_move(start, dest, step, vi);

// 		if (res == 1)
// 		{
// 			step++;
// 			moved = true;
// 		}
// 		else if (res == 0)
// 		{
// 			// 도착 완료
// 			vi->state = VEHICLE_STATUS_FINISHED;

// 			lock_acquire(&step_lock);
// 			unfinished_vehicles--;
// 			vehicles_reported_this_step++;

// 			if (vehicles_reported_this_step == unfinished_vehicles)
// 			{
// 				vehicles_reported_this_step = 0;
// 				vehicles_moved_this_step = 0;
// 				crossroads_step++;
// 				unitstep_changed();

// 				lock_acquire(&blinker_lock);
// 				cond_broadcast(&blinker_cond, &blinker_lock);
// 				lock_release(&blinker_lock);

// 				cond_broadcast(&step_cond, &step_lock);
// 			}
// 			else
// 			{
// 				cond_wait(&step_cond, &step_lock);
// 			}
// 			lock_release(&step_lock);

// 			priority_sema_up(&road_entry_sema); // 도착 후 해제
// 			break;
// 		}

// 	move_sync:
// 		lock_acquire(&step_lock);
// 		vehicles_reported_this_step++;
// 		if (moved)
// 			vehicles_moved_this_step++;

// 		if (vehicles_reported_this_step == unfinished_vehicles)
// 		{
// 			if (vehicles_moved_this_step == 0 && no_movable_vehicles())
// 			{
// 				printf("[DEADLOCK PREVENT] No one moved at step %d → force step++\n", crossroads_step);
// 			}

// 			vehicles_reported_this_step = 0;
// 			vehicles_moved_this_step = 0;
// 			crossroads_step++;
// 			unitstep_changed();

// 			lock_acquire(&blinker_lock);
// 			cond_broadcast(&blinker_cond, &blinker_lock);
// 			lock_release(&blinker_lock);

// 			cond_broadcast(&step_cond, &step_lock);
// 		}
// 		else
// 		{
// 			cond_wait(&step_cond, &step_lock);
// 		}

// 		lock_release(&step_lock);
// 	}
// }
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

// 수정된 no_movable_vehicles 함수 - 고급 신호등 시스템 사용
static bool no_movable_vehicles()
{
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
				// 고급 신호등 시스템으로 이동 가능성 확인
				if (can_vehicle_pass_signal(vi->position, next, vi))
				{
					return false; // 움직일 수 있는 차량이 있음
				}
			}
		}
	}
	return true; // 아무도 움직일 수 없음
}

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

		// 다른 차량의 현재 step 계산
		struct position curr = other->position;
		struct position next;

		while (true)
		{
			next = vehicle_path[s][d][step];
			if (is_position_outside(next))
				break;

			// 아직 도달하지 않은 차량의 다음 위치 중 내가 막고 있는지 확인
			if (next.row == me->position.row && next.col == me->position.col)
				return true;

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
			lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
			return 0; // 정상 종료
		}
		return -1; // 잘못된 경로
	}

	lock_acquire(&vi->map_locks[pos_next.row][pos_next.col]);

	if (vi->state == VEHICLE_STATUS_READY)
	{
		vi->state = VEHICLE_STATUS_RUNNING;
	}
	else
	{
		lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
	}

	vi->position = pos_next;
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
	priority_sema_init(&road_entry_sema, thread_cnt);
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

		if (crossroads_step < vi->arrival)
			goto move_sync;

		if (vi->state == VEHICLE_STATUS_FINISHED)
			goto move_sync;

		// === 핵심 수정: 고급 신호등 시스템으로 이동 가능 여부 확인 ===
		struct position next_pos = vehicle_path[start][dest][step];

		// 고급 신호등 시스템으로 신호 확인
		bool can_move = can_vehicle_pass_signal(vi->position, next_pos, vi);

		if (!can_move)
		{
			goto move_sync;
		}
		// === 수정 부분 끝 ===

		if (is_blocking_higher_priority_vehicle(vi))
		{
			goto move_sync;
		}

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

				// 기존 blinker 시스템에 알림 (하위 호환성)
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
			if (vehicles_moved_this_step == 0 && no_movable_vehicles())
			{
				printf("[DEADLOCK PREVENT] No one moved at step %d → force step++\n", crossroads_step);
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
