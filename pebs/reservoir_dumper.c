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
#include <time.h>
#include <sys/time.h>
#include <string.h>
typedef uint64_t u64;

#include "simple-pebs.h"
#include "dump-util.h"

#define err(x) perror(x), exit(1)

#define PGAP 1024/64
#define MAXL 100000+3
#define MAXH  8192
#define domain  256
#define STEP  10000
#define CacheLineSize  30 * 1024 * 1024 / 64
struct node {
    unsigned long long addr,label,ed;
    struct node *pre,*nxt;
    };

struct tnode {
    unsigned long long offset;
};

struct node *hash[MAXH];
struct node tar[MAXH];
int insnum,qlen;

unsigned long long rtd[MAXL];
unsigned long long n;
unsigned long long m = CacheLineSize;

unsigned long long domain_value_to_index(unsigned long long value)
{
    unsigned long long loc = 0,step = 1;
    int index = 0;
    while (loc+step*domain<value) {
        loc += step*domain;
        step *= 2;
        index += domain;
    }
    while (loc<value) index++,loc += step;
    return index;
}

unsigned long long domain_index_to_value(unsigned long long index)
{
    unsigned long long value = 0,step = 1;
    while (index>domain) {
        value += step*domain;
        step *= 2;
        index -= domain;
    }
    while (index>0) {
        value += step;
        index--;
    }
    return value;
}


void insert(unsigned long long now)
{
    insnum++;
    int loc = qlen;
    if (qlen==MAXH) {
        loc = rand()%insnum;
        if (loc>=qlen) return;
        if (tar[loc].pre==NULL && tar[loc].nxt==NULL) {
            hash[tar[loc].addr%MAXH] = NULL;
        }
        else if (tar[loc].pre==NULL) {
            hash[tar[loc].addr%MAXH] = tar[loc].nxt;
            tar[loc].nxt->pre = NULL;
        }
        else {
            tar[loc].pre->nxt = tar[loc].nxt;
            if (tar[loc].nxt) tar[loc].nxt->pre = tar[loc].pre;
        }
    }
    else qlen++;
    int t = now%MAXH;
    struct node *tmp = tar+loc;
    tmp->addr = now;
    tmp->label = n;
    tmp->nxt = hash[t];
    tmp->ed = -1;
    if (hash[t]) hash[t]->pre = tmp;
    tmp->pre = NULL;
    hash[t] = tmp;
}

void find(unsigned long long now)
{
    int t = now%MAXH;
    struct node *tmp = hash[t];
    while (tmp) {
        if (tmp->addr==now) {
            if (tmp->ed==-1) tmp->ed = n;
            return;
        }
        tmp = tmp->nxt;
    }
    return;
}

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
	printf("OK\n");

	qlen = 0; insnum = 0;
    memset(rtd,0,sizeof(unsigned long long)*MAXL);
    m = 0; n = 0;
    unsigned long long now,tott = 0;
    srand(time(NULL));

    unsigned long long loc = rand()%(STEP*2)+1;

	int _size = get_size();
	int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	void *map[ncpus];
	struct pollfd pfd[ncpus];
	int opt;
	bool binary = false;

	int k =0;
	for (k = 0; k < ncpus; k++)
		open_cpu(&map[k], k, &pfd[k], _size);
	printf("OK\n");

	int target = 7;
	FILE *outfile;
	outfile = fopen("tmp.txt","wb");
	if(outfile == NULL){
		exit(-1);
	}
	printf("OK\n");
	int _count = 0;
	for(;_count<=50;){
		if(poll(pfd, ncpus, -1)<0)
			perror("poll");
		if(pfd[target].revents & POLLIN){
			int len;

			if(ioctl(pfd[target].fd, SIMPLE_PEBS_GET_OFFSET, &len) < 0){
				perror("SIMPLE_PEBS_GET_OFFSET");
				continue;
			}
			if(len>1800000){
				printf("%d\n",len);
				/*
				if (binary)
					fwrite(map[target], len,1,outfile);
				else
					dump_data(target, map[target], len / sizeof(u64));
				*/
				unsigned long long * head = (unsigned long long *)map[target];
				int j =0;
				for(j = 0;j<len;j++){
					now = head[j] >> 6;
					n++;
					find(now);
					if (n==loc){
						insert(now);
						loc += rand()%(STEP*2)+1;
					}
				}
				if (ioctl(pfd[target].fd, SIMPLE_PEBS_RESET, 0) < 0) {
					perror("SIMPLE_PEBS_RESET");
					continue;
				}	
				_count++;
			}
		}
	}
 int i =0;
 for (i = 0; i<qlen; i++)
        if (tar[i].ed!=-1) tott++,rtd[domain_value_to_index(tar[i].ed-tar[i].label)]++;
    m = 1.0*(qlen-tott)/qlen*n;

    double sum = 0; unsigned long long T = 0;
    double tot = 0;
    double N = tott+1.0*tott/(n-m)*m;
    unsigned long long step = 1; int dom = 0,dT = 0; loc = 0;
	unsigned long long c = 1;
    for (c = 1; c<=m; c++) {
        while (T<=n && tot/N<c) {
            tot += N-sum;
            T++;
            if (T>loc) {
                if (++dom>domain) dom = 1,step *= 2;
                loc += step;
                dT++;
            }
            sum += 1.0*rtd[dT]/step;
        }
        if (c%PGAP==0) fprintf(outfile,"%.6lf\n",1.0*(N-sum)/N);
    }


	fclose(outfile);
	return 0;
}
