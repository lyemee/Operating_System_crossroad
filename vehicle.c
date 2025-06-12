
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/vehicle.h"
#include "projects/crossroads/map.h"
#include "projects/crossroads/ats.h"

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

void parse_vehicles(struct vehicle_info *vehicle_info, char *input)
{
	int idx = 0;
	char *token, *save_ptr;

	for (token = strtok_r(input, ":", &save_ptr); token != NULL; token = strtok_r(NULL, ":", &save_ptr))
	{
		struct vehicle_info *v = &vehicle_info[idx];

		v->id = token[0];
		v->start = token[1];
		v->dest = token[2];

		// 기본값: 일반 차량, 즉시 출발
		v->type = VEHICL_TYPE_NORMAL;
		v->arrival = 0;
		v->golden_time = -1;

		// 앰뷸런스인 경우 추가 파싱
		if (strlen(token) > 3 && isdigit(token[3]))
		{
			v->type = VEHICL_TYPE_AMBULANCE;

			char *timing = &token[3];
			v->arrival = atoi(timing);

			char *dot = strchr(timing, '.');
			if (dot != NULL)
			{
				v->golden_time = atoi(dot + 1);
			}
			else
			{
				v->golden_time = -1;
			}
		}

		v->state = VEHICLE_STATUS_READY;
		v->position.row = -1;
		v->position.col = -1;
		v->map_locks = NULL; // 이후에 초기화 필요

		idx++;
	}
}

// original function
static int is_position_outside(struct position pos)
{
	return (pos.row == -1 || pos.col == -1);
}

// original function
/* return 0:termination, 1:success, -1:fail */
static int try_move(int start, int dest, int step, struct vehicle_info *vi)
{
	struct position pos_cur, pos_next;

	pos_next = vehicle_path[start][dest][step];
	pos_cur = vi->position;

	if (vi->state == VEHICLE_STATUS_RUNNING)
	{
		/* check termination */
		if (is_position_outside(pos_next))
		{
			/* actual move */
			vi->position.row = vi->position.col = -1;
			/* release previous */
			lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
			return 0;
		}
	}

	/* lock next position */
	lock_acquire(&vi->map_locks[pos_next.row][pos_next.col]);
	if (vi->state == VEHICLE_STATUS_READY)
	{
		/* start this vehicle */
		vi->state = VEHICLE_STATUS_RUNNING;
	}
	else
	{
		/* release current position */
		lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
	}
	/* update position */
	vi->position = pos_next;

	return 1;
}

// original function
void init_on_mainthread(int thread_cnt)
{
	/* Called once before spawning threads */
}

// original function
void vehicle_loop(void *_vi)
{
	int res;
	int start, dest, step;

	struct vehicle_info *vi = _vi;

	start = vi->start - 'A';
	dest = vi->dest - 'A';

	vi->position.row = vi->position.col = -1;
	vi->state = VEHICLE_STATUS_READY;

	step = 0;
	while (1)
	{
		/* vehicle main code */
		res = try_move(start, dest, step, vi);
		if (res == 1)
		{
			step++;
		}

		/* termination condition. */
		if (res == 0)
		{
			break;
		}

		/* unitstep change! */
		unitstep_changed();
	}

	/* status transition must happen before sema_up */
	vi->state = VEHICLE_STATUS_FINISHED;
}
