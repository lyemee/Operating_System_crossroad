#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "threads/thread.h"
#include "threads/synch.h"
#include "projects/crossroads/vehicle.h"
#include "projects/crossroads/map.h"
#include "projects/crossroads/ats.h"
#include "projects/crossroads/crossroads.h"
#include "projects/crossroads/blinker.h"

/* 단위 스텝 동기화를 위한 전역 변수들 */
static int total_vehicles = 0;
static int active_vehicles = 0;
static int vehicles_moved = 0;

static struct lock step_sync_lock;
static struct condition step_sync_cond;
static struct lock ats_protection_lock;

/* 🚑 스마트 우선순위 시스템에 추가된 함수 선언 */
extern void update_ambulance_prediction_step(int current_step);

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
		v->map_locks = NULL;

		idx++;
	}
}

static int is_position_outside(struct position pos)
{
	return (pos.row == -1 || pos.col == -1);
}

/* 교차로 영역 판단 함수 */
static bool is_intersection(struct position pos)
{
	return (pos.row >= 2 && pos.row <= 4 && pos.col >= 2 && pos.col <= 4);
}

/* return 0:termination, 1:success, -1:fail */
static int try_move(int start, int dest, int step, struct vehicle_info *vi)
{
	struct position pos_cur, pos_next;
	bool was_in_intersection, will_be_in_intersection;

	pos_next = vehicle_path[start][dest][step];
	pos_cur = vi->position;

	/* 현재와 다음 위치의 교차로 여부 확인 */
	was_in_intersection = is_intersection(pos_cur);
	will_be_in_intersection = is_intersection(pos_next);

	if (vi->state == VEHICLE_STATUS_RUNNING)
	{
		/* check termination */
		if (is_position_outside(pos_next))
		{
			/* 교차로에서 나가는 경우 권한 해제 */
			if (was_in_intersection)
			{
				release_intersection_permission(vi, pos_next);
			}

			/* actual move */
			vi->position.row = vi->position.col = -1;
			/* release previous */
			lock_release(&vi->map_locks[pos_cur.row][pos_cur.col]);
			return 0;
		}
	}

	/* 🚑 교차로 진입 시 스마트 우선순위 시스템 확인 */
	if (!was_in_intersection && will_be_in_intersection)
	{
		/* 교차로 진입 권한 요청 */
		if (!acquire_intersection_permission(vi, pos_next))
		{
			return -1; /* 진입 실패 - 대기 */
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

	/* 교차로 내부에서 스텝 업데이트 */
	if (will_be_in_intersection)
	{
		update_intersection_step(vi, pos_next);
	}

	/* 교차로에서 나가는 경우 권한 해제 */
	if (was_in_intersection && !will_be_in_intersection)
	{
		release_intersection_permission(vi, pos_next);
	}

	return 1;
}

/* 단위 스텝 동기화 시스템 초기화 */
void init_on_mainthread(int thread_cnt)
{
	total_vehicles = thread_cnt;
	active_vehicles = thread_cnt;
	vehicles_moved = 0;

	lock_init(&step_sync_lock);
	cond_init(&step_sync_cond);
	lock_init(&ats_protection_lock);
}

/* 단위 스텝 동기화 배리어 함수 */
static void step_barrier(void)
{
	lock_acquire(&step_sync_lock);

	vehicles_moved++;

	/* 모든 활성 차량이 도달했는지 확인 */
	if (vehicles_moved >= active_vehicles)
	{
		/* 스텝 증가 */
		lock_acquire(&ats_protection_lock);
		crossroads_step++;
		unitstep_changed();

		/* 🚑 스마트 우선순위 시스템에 현재 스텝 알림 */
		update_ambulance_prediction_step(crossroads_step);

		lock_release(&ats_protection_lock);

		/* 교차로 상태 출력 (주기적으로) */
		if (crossroads_step % 3 == 0)
		{
			print_intersection_status();
		}

		/* 리셋 */
		vehicles_moved = 0;

		/* 모든 대기자 깨우기 */
		cond_broadcast(&step_sync_cond, &step_sync_lock);
	}
	else
	{
		/* 조건 변수로 대기 */
		cond_wait(&step_sync_cond, &step_sync_lock);
	}

	lock_release(&step_sync_lock);
}

/* 차량 종료 처리 */
static void vehicle_finished(void)
{
	lock_acquire(&step_sync_lock);
	active_vehicles--;
	cond_broadcast(&step_sync_cond, &step_sync_lock);
	lock_release(&step_sync_lock);
}

/* 메인 차량 루프 */
void vehicle_loop(void *_vi)
{
	int res;
	int start, dest, step;
	struct vehicle_info *vi = _vi;

	start = vi->start - 'A';
	dest = vi->dest - 'A';

	vi->position.row = vi->position.col = -1;
	vi->state = VEHICLE_STATUS_READY;

	step = 0; // unit step은 0부터, 차는 1에 앞으로 전진
	while (1)
	{
		/* 🚑 앰뷸런스의 경우 출발 시간 체크 */
		if (vi->type == VEHICL_TYPE_AMBULANCE && vi->arrival > crossroads_step + 1)
		{
			printf("🚑 [앰뷸런스 %c] 출발 시간 대기 중 (현재:%d, 출발:%d)\n",
				   vi->id, crossroads_step, vi->arrival);
		}
		else
		{
			/* 차량 이동 시도 */
			res = try_move(start, dest, step, vi);
			if (res == 1)
			{
				step++;
			}
			else if (res == -1)
			{
				// 대기 상황은 이미 try_move에서 로그 출력
			}
			else if (res == 0)
			{

				step_barrier(); /* 마지막 배리어 통과 */
				break;
			}
		}

		step_barrier(); /* 스텝 동기화 */
	}

	/* 차량 종료 처리 */
	vi->state = VEHICLE_STATUS_FINISHED;
	vehicle_finished();
}