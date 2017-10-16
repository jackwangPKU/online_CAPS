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


#include "simple-pebs.h"
#include "dump-util.h"

#define err(x) perror(x), exit(1)

void dump_data(int cpunum, u64 *map, int num)
{
	int i;
	
	printf("dump %d\n", num);
	for (i = 0; i < num; i++)
		printf("%d: %lx\n", cpunum, map[i]);
}

static void usage(void)
{
	fprintf(stderr, "Usage: dumper [-b]\n"
		"-b binary dump\n");
	exit(1);
}

int main(int ac, char **av)
{
	int size = get_size();
	int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	void *map[ncpus];
	struct pollfd pfd[ncpus];
	int opt;
	bool binary = false;

	while ((opt = getopt(ac, av, "b")) != -1) {
		switch (opt) {
		case 'b':
			binary = true;
			break;
		default:
			usage();
		}
	}

	int i;
	for (i = 0; i < ncpus; i++)
		open_cpu(&map[i], i, &pfd[i], size);
	
	int target = 7;
	FILE *outfile;
	outfile = fopen("tmp.ref","wb");
	if(outfile == NULL){
		exit(-1);
	}
	int _count = 0;
	for(;;){
		if(poll(pfd, ncpus, -1)<0)
			perror("poll");
		if(pfd[target].revents & POLLIN){
			int len;

			if(ioctl(pfd[target].fd, SIMPLE_PEBS_GET_OFFSET, &len) < 0){
				perror("SIMPLE_PEBS_GET_OFFSET");
				continue;
			}
			if(len>1800000){
				/*printf("%d\n",len);*/
				if (binary)
					fwrite(map[target], len,1,outfile);
				else
					dump_data(target, map[target], len / sizeof(u64));

				if (ioctl(pfd[target].fd, SIMPLE_PEBS_RESET, 0) < 0) {
					perror("SIMPLE_PEBS_RESET");
					continue;
				}
				unsigned long long access,cycles,instr;
				if (ioctl(pfd[target].fd, GET_ACCESS, &access) < 0) {
					perror("GET_ACCESS");
					continue;
				}
				if (ioctl(pfd[target].fd, GET_CYCLES, &cycles) < 0) {
					perror("GET_CYCLES");
					continue;
				}
				if (ioctl(pfd[target].fd, GET_INSTR, &instr) < 0) {
					perror("GET_INSTR");
					continue;
				}
				printf("%d %llu %llu %llu\n",len,access,cycles,instr);	
				_count++;
			}
		}
	}
	fclose(outfile);
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
