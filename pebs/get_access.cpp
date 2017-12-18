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
#include <ctime>
#include <time.h>
#include <math.h>
#include "simple-pebs.h"
#include "dump-util.h"

#define err(x) perror(x), exit(1)

int core_use[2] = {6,7};
int main(int ac, char **av)
{
	int size = get_size();
	int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	void *map[ncpus];
	struct pollfd pfd[ncpus];
	int opt;

	int i;
	for (i = 0; i < ncpus; i++)
		open_cpu(&map[i], i, &pfd[i], size);
	int target = 0;
	int _count = 0;

	char buffer[26];
  
	int millisec;
    struct tm* tm_info;
	struct timeval tv;

	for(;;){
		if(poll(pfd, ncpus, -1)<0)
			perror("poll");
		for(target = 0; target<2;target++){
	
	  		gettimeofday(&tv, NULL);

		    millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
			  if (millisec>=1000) { // Allow for rounding up to nearest second
				      millisec -=1000;
					      tv.tv_sec++;
						    }

			    tm_info = localtime(&tv.tv_sec);

				  strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
				   // printf("%s.%03d\n", buffer, millisec);
		if(pfd[target].revents & POLLIN){
				unsigned long long access,cycles,instr;
				if (ioctl(pfd[core_use[target]].fd, GET_ACCESS, &access) < 0) {
					perror("GET_ACCESS");
					continue;
				}
				if (ioctl(pfd[core_use[target]].fd, GET_CYCLES, &cycles) < 0) {
					perror("GET_CYCLES");
					continue;
				}
				if (ioctl(pfd[core_use[target]].fd, GET_INSTR, &instr) < 0) {
					perror("GET_INSTR");
					continue;
				}
				printf("%s.%03d,core:%d,%llu,%llu,%llu,%.6lf\n",buffer,millisec,core_use[target],access,cycles,instr,(double)access/instr);	
				//_count++;
			}
		}
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
