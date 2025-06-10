#ifndef __PROJECTS_PROJECT2_VEHICLE_H__
#define __PROJECTS_PROJECT2_VEHICLE_H__

#include "projects/crossroads/position.h"
#include "lib/kernel/list.h"

#define VEHICLE_STATUS_READY 0
#define VEHICLE_STATUS_RUNNING 1
#define VEHICLE_STATUS_FINISHED 2

#define VEHICL_TYPE_NORMAL 0
#define VEHICL_TYPE_AMBULANCE 1

struct vehicle_info
{
	char id;
	char state;
	char start;
	char dest;

	char type;
	char arrival;
	char golden_time;

	struct position position;
	struct lock **map_locks;
	struct thread *t;	   // vehicle 스레드 자체 포인터
	struct list_elem elem; // 우선순위 세마포 대기열에 넣기 위함
};

void vehicle_loop(void *vi);

void parse_vehicles(struct vehicle_info *vehicle_info, char *input);

#endif /* __PROJECTS_PROJECT2_VEHICLE_H__ */