
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/vehicle.h"
#include "projects/crossroads/map.h"
#include "projects/crossroads/ats.h"
#include "projects/crossroads/priority_sync.h"

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

extern struct priority_sema road_entry_sema;

void parse_vehicles(struct vehicle_info *vehicle_info, char *input)
{
	int idx = 0;
	char *token, *save_ptr;

	for (token = strtok_r(input, ":", &save_ptr); token != NULL; token = strtok_r(NULL, ":", &save_ptr))
	{
		int len = strlen(token);
		if (len < 3)
			continue;

		char id = token[0];
		char start = token[1];
		char dest = token[2];
		int type = isupper(id) ? VEHICL_TYPE_AMBULANCE : VEHICL_TYPE_NORMAL;
		int arrival = -1, golden_time = -1;

		if (type == VEHICL_TYPE_AMBULANCE && len > 3)
		{
			char *time_part = &token[3];
			char *dot = strchr(time_part, '.');
			if (dot)
			{
				*dot = '\0';
				arrival = atoi(time_part);
				golden_time = atoi(dot + 1);
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
}

static int is_position_outside(struct position pos)
{
	return (pos.row == -1 || pos.col == -1);
}

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

void init_on_mainthread(int thread_cnt)
{
	/* Called once before spawning threads */
}

void vehicle_loop(void *_vi)
{
	int res;
	int start, dest, step;

	struct vehicle_info *vi = _vi;
	vi->t = thread_current(); // 여기에서 직접 저장해줌

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
