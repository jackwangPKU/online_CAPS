/* Dump simple PEBS data from kernel driver */
//extern "C"{
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
#include "simple-pebs.h"
#include "dump-util.h"
//}
#include "predict.h"

#define PGAP 1024/64
#define MAXL 100000+3
#define MAXH  8192
#define domain  256
#define STEP  10000
#define CacheLineSize  56320 * 1024  / 64
/*
extern "C"{
	int get_size(void);
	void open_cpu(void **mapp, int cnum, struct pollfd *pfd, int size);
}
*/
int core_use[15] = {10,12,14,16,44,46,48,50,52,54,56,58,60,62,64};
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
/*predict and SA*/
extern Workload workload[MAXN];
extern int workload_num;
extern bool need_calc_ar;

uint64_t best_cos[MAXN];

double T = 10000;//temperature
double T_min = 1;//threshold
double k = 6e-7;//constant

FILE *fin;
double best, cur_miss_rate;

char *ull2BinaryStr(uint64_t cos) {
    char temp[256];
    char *s = new char[256];
    int i = 0;
    while (cos) {
        temp[i] = cos % 2 + '0';
        cos /= 2;
        i++;
    }
    for (int j = 0; j < i; j++)
        s[j] = temp[i - j - 1];
    s[i] = 0;
    return s;
}

char *cos2Pic(uint64_t cos){
    char temp[256];
    char *s = new char[256];
    int i = 0;
    while(cos){
        if(cos%2==0) temp[i] = ' ';
        else temp[i] = '*';
        i++;
        cos /=2;
    }
    for(;i<20;i++) temp[i]=' ';
    for(int j=0; j<i; j++)
        s[j] = temp[i-j-1];
    s[i] = '\0';
    return s;
}

char *ull216Str(uint64_t cos) {
    char *s = new char[256];
    strcpy(s, "0x");
    char temp[256];
    sprintf(temp, "%llx", cos);
    strcat(s, temp);
    return s;
}
bool containOne1(uint64_t cos) {
    int c = 0;
    while (cos) {
        if (cos % 2 == 1)
            c++;
        cos = cos >> 1;
    }
    if (c == 1)
        return true;
    else
        return false;
}
/* direction: 0 right expand, 1 right reduce, 2 left expand, 3 left reduce*/
uint64_t modifyCos(int index, int d) {
    uint64_t cos = workload[index].cos;
    uint64_t pre_value = cos;
    if ((cos >= 1 << (MAXN - 1) && d == 2) || ((cos % 2 == 1) && d == 0) ||
        (containOne1(cos) && (d == 1 || d == 3)))
        return pre_value;
    if (d == 0) { // right expand
        int c = 0;
        while (cos % 2 == 0) {
            cos = cos >> 1;
  		c++;
        }
        workload[index].cos += 1 << (c - 1);
    } else if (d == 1) {
        int c = 0;
        while (cos % 2 == 0) {
            cos = cos >> 1;
            c++;
        }
        workload[index].cos -= 1 << c;
    } else if (d == 2) {
        int c = 0;
        while (cos % 2 == 0) {
            cos = cos >> 1;
            c++;
        }
        while (cos % 2) {
            cos = cos >> 1;
            c++;
        }
        workload[index].cos += 1 << c;
    } else if (d == 3) {
        int c = 0;
        while (cos % 2 == 0) {
            cos = cos >> 1;
 c++;
        }
        while (cos % 2) {
            cos = cos >> 1;
            c++;
        }
        workload[index].cos -= 1 << (c - 1);
    }
    return pre_value;
}

/*reservoir dumper================================================*/


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

int main(int argv, char **argc)
{
	/*init workload*/
	char filename[100];
    	srand(time(NULL));

    workload_num = argv / 2 - 1;
	if(workload_num >15){
		printf("too many workloads\n");
		return 0;
	}
    need_calc_ar = false;
    if (argc[1][0] == '1') {
        need_calc_ar = true;
    }

    for (int i = 0; i < workload_num; i++) {
        workload[i].occ = 0;
        workload[i].name = strdup(argc[i * 2 + 2]);
        workload[i].allocation = strdup(argc[i * 2 + 3]);
        workload[i].cos = strtouint64(workload[i].allocation);
        workload[i].ways = count_1s(workload[i].cos);
        workload[i].miss_ratio = 0;
        if (need_calc_ar) {
            workload[i].access_rate = workload[i].mrc[L2_CACHE_SIZE / BLOCK];
        } else {
            workload[i].access_rate = 1;
        }
    }
    get_baseIPC();

//printf("OK\n");
	int _size = get_size();
	int ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	void *map[ncpus];
	struct pollfd pfd[ncpus];
	int opt;
	bool binary = false;

	int k =0;
	for (k = 0; k < ncpus; k++){
		open_cpu(&map[k], k, &pfd[k], _size);
	}
	while(1){
	int target = 0;
	for(k = 0; k < workload_num; k++){
		target = core_use[k];
		//printf("target: %d\n",target);
	
		/*get mrc*/
		qlen = 0; insnum = 0;
    	memset(rtd,0,sizeof(unsigned long long)*MAXL);
		memset(tar,0,sizeof(struct node)*MAXH);
		memset(hash,0,sizeof(struct node*)*MAXH);
		m = 0; n = 0;
    	unsigned long long now,tott = 0;
    	unsigned long long loc = rand()%(STEP*2)+1;
		int _count = 0;
		for(;_count<2;){
			if(poll(pfd, ncpus, -1)<0)
				perror("poll");
			if(pfd[target].revents & POLLIN){
				int len;

				if(ioctl(pfd[target].fd, SIMPLE_PEBS_GET_OFFSET, &len) < 0){
					perror("SIMPLE_PEBS_GET_OFFSET");
					continue;
				}
					if(len>1800000){
 				//printf("%d\n",len);
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
		/*access rate*/
		unsigned long long d_instr,d_access;
		if(ioctl(pfd[target].fd, GET_ACCESS, &d_access)<0){
			perror("GET ACCESS");
			return -1;
		}
		if(ioctl(pfd[target].fd, GET_INSTR, &d_instr)<0){
			perror("GET INSTR");
			return -1;
		}
		workload[k].access_rate = (double)d_access/d_instr;
		//printf("access_rate:%lf\n",workload[k].access_rate);
		/*mrc*/
		memset(workload[k].mrc,0,sizeof(double)*MAXS);
		int i =0;
 		for (i = 0; i<qlen; i++)
       		if (tar[i].ed!=-1) tott++,rtd[domain_value_to_index(tar[i].ed-tar[i].label)]++;
    		m = CacheLineSize;

    		double sum = 0; unsigned long long T = 0;
    		double tot = 0;
    		double N = tott+1.0*tott/(n-m)*m;
    		unsigned long long step = 1; int dom = 0,dT = 0; loc = 0;
		unsigned long long c = 1;
			int _index = 0;
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
        		if (c%PGAP==0) {workload[k].mrc[_index] = 1.0*(N-sum)/N;
					//printf("%d: %.6lf\n",c/PGAP -1,workload[k].mrc[c/PGAP -1]);
					//printf("%llu %d %d\n",c,PGAP,c/PGAP);	
					_index ++;
				}
   	 	}
		//for(int y=0;y<MAXS;y++){printf("%d: %.6lf\n",y,workload[k].mrc[y]);}	

		//printf("mrc 0:%.6lf 1:%.6lf\n",workload[k].mrc[0],workload[k].mrc[MAXS]);
	}
 
/*predict and SA==============================*/
    best = predict_total_miss_rate();
    cur_miss_rate = best;
    int _count = 0;
    while (T >= T_min) {
        int target = rand() % workload_num;
        int direction =
            rand() %
            4; // 0 right expand, 1 right reduce, 2 left expand, 3 left reduce
        uint64_t pre_cos = modifyCos(target, direction);
        double tmp = predict_total_miss_rate();
        //printf("miss_rate: %lf\n",tmp);
        if (tmp < best){
            best = tmp;
		for(int _i = 0;_i<workload_num;_i++)
		best_cos[_i] = workload[_i].cos;
	}
        double df = tmp - cur_miss_rate;
        if ((df > 0) && ((rand() % 1000 / (float)1000) > exp(-df / (k * T)))) {
            workload[target].cos = pre_cos;
        } else {
            cur_miss_rate = tmp;
        }
        if(_count==5){
            T*=0.8;
            _count = 0;
        }
        else _count++;
    }
	printf("best %.6lf, cos:=========\n",best);
	char buffer[1024]={0};
	for(int _j = 0; _j<workload_num;_j++ ){
		sprintf(buffer,"pqos -e \"llc:%d=%s\"",_j+1,ull216Str(workload[_j].cos));
		//printf("%s : %s\n",workload[_j].name,ull216Str(workload[_j].cos));
		printf("%s\n",buffer);
		system(buffer);
	}
	for(int _j = 0; _j<workload_num;_j++){
		sprintf(buffer,"pqos -a \"llc:%d=%d\"",_j+1,core_use[_j]);
		printf("%s\n",buffer);
		system(buffer);
	}
	sleep(20000);
	}
	return 0;
}
