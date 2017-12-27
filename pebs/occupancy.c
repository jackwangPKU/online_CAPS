/* Dump simple PEBS data from kernel driver */
#include <unistd.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "simple-pebs.h"
#include "dump-util.h"

#define err(x) perror(x), exit(1)
#define corenum 2
int core_use[corenum] = {10,11};
int main(int ac, char **av)
{
	int size = get_size();
	int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	void *map[ncpus];
	struct pollfd pfd[ncpus];
	int opt;
	int target =0;
	int i;
	for (i = 0; i < ncpus; i++)
		open_cpu(&map[i], i, &pfd[i], size);
	/*for (i = 0; i < corenum; i++)
		ioctl(pfd[core_use[i]].fd, GET_OCCUPANCY_INIT, 0);	
	*/
	for(;;){
		if(poll(pfd, ncpus, -1)<0)
			perror("poll");
		for(target=0; target<2; target++)
		if(pfd[target].revents & POLLIN){
			ioctl(pfd[core_use[target]].fd, GET_OCCUPANCY_INIT, 0);
			unsigned long long occu;
			if (ioctl(pfd[core_use[target]].fd, GET_OCCUPANCY, &occu) < 0) {
				perror("GET_OCCUPANCY");
				continue;
			}
			printf("occu: %d\n",occu/1024);
		}
		sleep(1);
	}
	/*	
	for (;;) {
		if (poll(pfd, ncpus, -1) < 0)
			perror("poll");
		for (i = 0; i < ncpus; i++) {
			if (pfd[i].revents & POLLIN) {
				int len;

				if (ioctl(pfd[i].fd, SIMPLE_PEBS_GET_OFFSET, &len) < 0) {
					perror("SIMPLE_PEBS_GET_OFFSET");
					continue;
				}

				if (binary)
					write(1, map[i], len);
				else
					dump_data(i, map[i], len / sizeof(u64));

				if (ioctl(pfd[i].fd, SIMPLE_PEBS_RESET, 0) < 0) {
					perror("SIMPLE_PEBS_RESET");
					continue;
				}
			}
		}
	}
*/
	return 0;
}
