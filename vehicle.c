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

static bool can_pass(enum blinker_dir dir, struct vehicle_info *vi)
{
	return (dir == BLINKER_EW && (vi->start == 'A' || vi->start == 'C')) ||
		   (dir == BLINKER_NS && (vi->start == 'B' || vi->start == 'D'));
}

int is_position_outside(struct position pos)
{
	return (pos.row == -1 || pos.col == -1);
}

static bool no_movable_vehicles()
{
	for (int i = 0; i < vehicle_count; i++)
	{
		struct vehicle_info *vi = &vehicle_list_ptr[i];
		if (vi->state == VEHICLE_STATUS_RUNNING)
		{
			if (can_pass(current_blinker_state, vi))
			{
				int start = vi->start - 'A';
				int dest = vi->dest - 'A';
				struct position next = vehicle_path[start][dest][0];
				if (!is_position_outside(next))
					return false;
			}
		}
	}
	return true;
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
	priority_sema_init(&road_entry_sema, 6);
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

		lock_acquire(&blinker_lock);
		bool can_move = can_pass(current_blinker_state, vi);
		lock_release(&blinker_lock);
		if (!can_move)
			goto move_sync;

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
