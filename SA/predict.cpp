#ifndef _PREDICT_H_
#define _PREDICT_H_
    #include "predict.h"
#endif

//uint64_t m;
Segment segment[WAY+1];
int segment_num;
Workload workload[MAXN];
int workload_num;
double occupancy[MAXN][WAY+1];
uint64_t accesses;
bool need_calc_ar;
double CPI = 0.4;
double PENALTY = 70;
double evict_p[MAXN];
double evict_r[MAXN];
void segmentation() {
    set<int> current;
    set<int> tmp;
    segment_num = 0;

    for (int i = 0; i < WAY; i++) {
        tmp.clear();
        for (int j = 0; j < workload_num; j++) {
            if ((1 << i) & workload[j].cos) {
                tmp.insert(j);
            }
        }
        if (tmp.size() == 0) {
            continue;
        }
        if (segment_num == 0 || tmp != current) {
            segment[segment_num].begins = i;
            segment[segment_num].ends = i;
            segment[segment_num].workload = tmp;
            current = tmp;
            segment_num++;
        } else {
            segment[segment_num - 1].ends = i;
        }
    }
}

void init_occupancy() {
//	printf("segment num :%d\n",segment_num);
    for (int i = 0; i < segment_num; i++) {
        uint64_t segment_ways = segment[i].ends - segment[i].begins + 1;
        uint64_t segment_blocks = segment_ways * WAY_SIZE / BLOCK;

        for (set<int>::iterator iter = segment[i].workload.begin();
             iter != segment[i].workload.end(); iter++) {
            int wid = *iter;
            occupancy[wid][i] =
                (double)segment_blocks / segment[i].workload.size();
//printf("segment_blocks: %llu, segment[%d].workload.size: %d",segment_blocks,i,segment[i].workload.size());		
//printf("occupancy[%d][%d]: %lf\n",wid,i, occupancy[wid][i]);
        }
    }
}

void o2m() {
    for (int i = 0; i < workload_num; i++) {
        double occ = 0;
        for (int j = 0; j < segment_num; j++) {
            occ += occupancy[i][j];
            //printf("%lf %lf\n",occ,workload[i].mrc[(int)occ]);
        }
        workload[i].occ = occ;
        if(occ>=MAXS) occ=MAXS-1;
        // if(occ<0) printf("error\n");
        workload[i].miss_ratio = workload[i].mrc[(uint64_t)occ]==0 ? 0.00001 : workload[i].mrc[(uint64_t)occ];
		//printf("%.6lf %llu\n",workload[i].miss_ratio,occ);
        workload[i].ipc = 1/(CPI+workload[i].miss_ratio*workload[i].access_rate*PENALTY);
        //printf("in o2m workload[%d] , miss_ratio: %lf ipc: %lf access rate: %lf\n",i,workload[i].miss_ratio,workload[i].ipc,workload[i].access_rate);
    	workload[i].apc = workload[i].ipc*workload[i].access_rate;
	//printf("%lf %lf %lf\n",workload[i].ipc,workload[i].access_rate,workload[i].apc);
	}
}

void m2o() {
    for (int i = 0; i < segment_num; i++) {
        uint64_t segment_ways = segment[i].ends - segment[i].begins + 1;
        uint64_t segment_blocks = segment_ways * WAY_SIZE / BLOCK;
        double miss_num[MAXN], miss_num_total = 0;
	double apc_total = 0;
        for (set<int>::iterator iter = segment[i].workload.begin();
             iter != segment[i].workload.end(); iter++) {
            int wid = *iter;
	    //apc_total += workload[wid].apc;
            miss_num[wid] =
                (workload[wid].miss_ratio * workload[wid].access_rate * workload[wid].ipc * accesses * segment_ways / workload[wid].ways);
       //      printf("miss_num %lf\n",miss_num[wid]);
            //miss_num_total += miss_num[wid];
            //printf("workload[%d].miss_ratio: %lf, workload[%d].access_rate: %lf,accesses: %d,\
 	ipc: %lf, segment_ways: %d, workload[%d].ways: %d, miss_num[%d]: %lf\n",\
	wid,workload[wid].miss_ratio,wid,workload[wid].access_rate,accesses,workload[wid].ipc,\
	segment_ways,wid,workload[wid].ways,wid,miss_num[wid]);
        }
        // printf("miss_num_total: %d\n",miss_num_total);
        set<int>::iterator iter = segment[i].workload.begin();
	int first = *iter;
	evict_r[first] = 1;
	double tmp = 1;
        for (iter = segment[i].workload.begin();
             iter != segment[i].workload.end(); iter++) {
            int wid = *iter;
		if(wid!=first){
		evict_r[wid] = occupancy[wid][i] * workload[first].apc / (occupancy[first][i] * workload[wid].apc);
		//printf("%lf %lf %lf %lf \n",occupancy[wid][i], workload[first].apc, occupancy[first][i], workload[wid].apc);
		//printf("evtct_r[%d] %lf\n",wid,evict_r[wid]);
		tmp += evict_r[wid];
		}
	}
	for (iter = segment[i].workload.begin();
             iter != segment[i].workload.end(); iter++) {
            int wid = *iter;
		evict_p[wid] = evict_r[wid] / tmp;
	//	printf("w%d: missratio: %lf, access_rate: %lf, ipc: %lf, access: %d, missnum: %lf ,evict_p : %lf, occu: %lf\n",wid,workload[wid].miss_ratio,workload[wid].access_rate,workload[wid].ipc,accesses,miss_num[wid],evict_p[wid],occupancy[wid][i]);
	}

	
	for (iter = segment[i].workload.begin();
             iter != segment[i].workload.end(); iter++) {
            int wid = *iter;
		for (set<int>::iterator iter2 = segment[i].workload.begin();
             		iter2 != segment[i].workload.end(); iter2++) {
			int wid2 = *iter2;
			if(wid2!=wid){
            		occupancy[wid][i] += (evict_p[wid2] * miss_num[wid] - evict_p[wid] * miss_num[wid2]);
			}
			//printf("occupancy[%d] : %lf\n",wid,occupancy[wid][i]);
		}
/*
 * model 0:
 * p0 / p1 = C0 * APC0 / C1 * APC1 
 * p0 + p1 = 1
*/ 
/*  	
		double p1 = 1.0 * occupancy[wid][i] * (apc_total - workload[wid].apc) / ((segment_blocks - occupancy[wid][i]) * workload[wid].apc + occupancy[wid][i] * (apc_total - workload[wid].apc));
		double p0 = 1-p1;
		occupancy[wid][i] = occupancy[wid][i] + miss_num[wid]  - miss_num_total * p1;
printf("%lf %lf\n",p0,p1);
*/

/* model 1:
 * p0 / p1 = C0 * APC0 / C1 * APC1
 * p1 * C1 + p0 * C0 = 1
 * 
 * */
/*
	double R = workload[wid].apc * (segment_blocks - occupancy[wid][i]) / ((apc_total - workload[wid].apc) * occupancy[wid][i]);
	double p0 = R / (occupancy[wid][i] + R * (segment_blocks - occupancy[wid][i]));
	double p1 = 1.0 / (occupancy[wid][i] + R * (segment_blocks - occupancy[wid][i]));
	//printf("%lf %llu occupancy[%d][%d]:%lf, %lf\n",workload[wid].apc,segment_blocks,wid,i,occupancy[wid][i],apc_total);
	//printf("%lf %lf %lf\n",R,p0,p1);
	occupancy[wid][i] = occupancy[wid][i] * (1-(miss_num_total - miss_num[wid]) * p0) + (segment_blocks - occupancy[wid][i]) * miss_num[wid] * p1;
*/
/*
            occupancy[wid][i] = occupancy[wid][i] +
                                miss_num[wid] *
                                    (segment_blocks - occupancy[wid][i]) /
                                    segment_blocks -
                                (miss_num_total - miss_num[wid]) *
                                    occupancy[wid][i] / segment_blocks;
*/
            if(occupancy[wid][i]<=0){
		occupancy[wid][i] = 10;
		}

	}
    }
}

void get_accessrate(){
    FILE *acc =  fopen("access_rate.txt","rb");
    if(!acc){
        printf("access_rate file not exist\n");
        exit(-1);
    }
    char name[100];
    double access_rate;
    //double total = 0;
    while(fscanf(acc,"%s %lf",name,&access_rate)==2){
        //total += access_rate;
        for(int i=0; i<workload_num; i++){
            if(strcmp(workload[i].name,name)==0){
                workload[i].access_rate = access_rate;
                break;
            }
        }
    }
    //for(int i=0; i<workload_num;i++){
    //    workload[i].access_rate /= total;
    //}
    fclose(acc);
}

void get_baseIPC(){
    FILE * ipc = fopen("base_IPC.txt","rb");
    if(!ipc){
        printf("base_IPC file not exist\n");
        exit(-1);
    }
    char name[100];
    double base_ipc;
    while(fscanf(ipc,"%s %lf",name,&base_ipc)==2){
        for(int i=0; i<workload_num; i++){
            if(strcmp(workload[i].name,name)==0){
                workload[i].base_ipc = base_ipc;
                break;
            }
        }
    }
    fclose(ipc);
}

void get_mrc(int i,FILE *fin) {
    int c = 0;
    double pre = 0;
    for (; c < MAXS; c++) {
        if (fscanf(fin, "%lf", &workload[i].mrc[c]) > 0)
            ;
        else {
            pre = workload[i].mrc[c - 1];
            break;
        }
    }
    for (; c < MAXS; c++)
        workload[i].mrc[c] = pre;
}

double predict_occupancy(double *real){
    memset(segment, 0, sizeof(segment));
    memset(occupancy, 0, sizeof(occupancy));

    for(int i=0;i<workload_num;i++) workload[i].ways = count_1s(workload[i].cos);
    segmentation();
    init_occupancy();
    //get_accessrate();
    // iteration process
    /*
    for (accesses = 1000; accesses >= 100; accesses -= 1) {
        o2m();
        m2o();
    }
    */
    accesses = 20000;
//printf("workload num:%d\n",workload_num);
    for (int i = 0; i < 200; i++) {
        o2m();
        m2o();
       /* 
        for( int j = 0; j< workload_num; j++ ){
            printf("%d\n",(int)workload[j].occ);
        }
        */
        // printf("\n");
        if (i % 10 == 0)
            accesses=accesses-800;
    }
    o2m();
    double error=0;
    for(int i = 0; i < workload_num; i++){
	error += fabs(real[i]-workload[i].occ)/real[i];
	printf("real occ: %lf pred: %lf\n",(double)real[i],workload[i].occ);
	}
    error /= workload_num;
	//printf("error %d:%lf\n",error);
    return error;
}

double predict_max_weighted_slowdown(double CPI, double PENALTY) {
    char filename[100];
    memset(segment, 0, sizeof(segment));
    memset(occupancy, 0, sizeof(occupancy));

    for(int i=0;i<workload_num;i++) workload[i].ways = count_1s(workload[i].cos);
    segmentation();
    init_occupancy();
    //get_accessrate();
    // iteration process
    /*
    for (accesses = 1000; accesses >= 100; accesses -= 1) {
        o2m();
        m2o();
    }
    */
    accesses = 20000;
    for (int i = 0; i < 1000; i++) {
        o2m();
        m2o();
        /*
        for( int j = 0; j< workload_num; j++ ){
            if(j==0)printf("%d\n",(int)workload[j].occ);
        }
        */
        // printf("\n");
        if (i % 10 == 0)
            accesses=accesses-150;
    }
    o2m();

    double max_weighted_slowdown = 0;

    for (int i = 0; i < workload_num; i++) {
        double pre_ipc = 1/(CPI+workload[i].miss_ratio*workload[i].access_rate*PENALTY);
	workload[i].weighted_slowdown = workload[i].base_ipc/pre_ipc;
	if(workload[i].weighted_slowdown>max_weighted_slowdown) max_weighted_slowdown = workload[i].weighted_slowdown;
    } // printf("%15s\t%s\t%lf\t%lf\t%lf\n", workload[i].name,
      // workload[i].allocation, workload[i].access_rate,
      // workload[i].miss_ratio,workload[i].occ);
    return max_weighted_slowdown;
}

double predict_fair_weighted_slowdown(double CPI, double PENALTY) {
    char filename[100];
    memset(segment, 0, sizeof(segment));
    memset(occupancy, 0, sizeof(occupancy));

    for(int i=0;i<workload_num;i++) workload[i].ways = count_1s(workload[i].cos);
    segmentation();
    init_occupancy();
    //get_accessrate();
    // iteration process
    /*
    for (accesses = 1000; accesses >= 100; accesses -= 1) {
        o2m();
        m2o();
    }
    */
    accesses = 20000;
    for (int i = 0; i < 1000; i++) {
        o2m();
        m2o();
        /*
        for( int j = 0; j< workload_num; j++ ){
            if(j==0)printf("%d\n",(int)workload[j].occ);
        }
        */
        // printf("\n");
        if (i % 10 == 0)
            accesses-=150;
    }
    o2m();

    double fair_weighted_slowdown = 0;

    for (int i = 0; i < workload_num; i++) {
        double pre_ipc = 1/(CPI+workload[i].miss_ratio*workload[i].access_rate*PENALTY);
        workload[i].weighted_slowdown = workload[i].base_ipc/pre_ipc;
        fair_weighted_slowdown += 1/workload[i].weighted_slowdown;
    } // printf("%15s\t%s\t%lf\t%lf\t%lf\n", workload[i].name,
      // workload[i].allocation, workload[i].access_rate,
      // workload[i].miss_ratio,workload[i].occ);
    fair_weighted_slowdown = workload_num/fair_weighted_slowdown;
    return fair_weighted_slowdown;
}

double predict_weighted_slowdown(double CPI, double PENALTY) {
    char filename[100];
    memset(segment, 0, sizeof(segment));
    memset(occupancy, 0, sizeof(occupancy));

    for(int i=0;i<workload_num;i++) workload[i].ways = count_1s(workload[i].cos);
    segmentation();
    init_occupancy();
    //get_accessrate();
    // iteration process
    /*
    for (accesses = 1000; accesses >= 100; accesses -= 1) {
        o2m();
        m2o();
    }
    */
    accesses = 20000;
    for (int i = 0; i < 1000; i++) {
        o2m();
        m2o();
        /*
        for( int j = 0; j< workload_num; j++ ){
            if(j==0)printf("%d\n",(int)workload[j].occ);
        }
        */
        // printf("\n");
        if (i % 10 == 0)
            accesses-=150;
    }
    o2m();

    double total_weighted_slowdown = 0;

    for (int i = 0; i < workload_num; i++) {
        double pre_ipc = 1/(CPI+workload[i].miss_ratio*workload[i].access_rate*PENALTY);
	workload[i].weighted_slowdown = workload[i].base_ipc/pre_ipc;
	total_weighted_slowdown += workload[i].weighted_slowdown;
    } // printf("%15s\t%s\t%lf\t%lf\t%lf\n", workload[i].name,
      // workload[i].allocation, workload[i].access_rate,
      // workload[i].miss_ratio,workload[i].occ);
    return total_weighted_slowdown/workload_num;
}

double predict_total_ipc(double CPI, double PENALTY) {
    char filename[100];
    memset(segment, 0, sizeof(segment));
    memset(occupancy, 0, sizeof(occupancy));

    for(int i=0;i<workload_num;i++) workload[i].ways = count_1s(workload[i].cos);
    segmentation();
    init_occupancy();
    //get_accessrate();
    // iteration process
    /*
    for (accesses = 1000; accesses >= 100; accesses -= 1) {
        o2m();
        m2o();
    }
    */
    accesses = 20000;
    for (int i = 0; i < 1000; i++) {
        o2m();
        m2o();
    /*
        for( int j = 0; j< workload_num; j++ ){
            if(j==0)printf("%d\n",(int)workload[j].occ);
        }
    */
        // printf("\n");
        if (i % 10 == 0)
            accesses-=150;
    }

    o2m();

    double pre_total_ipc = 0;

    for (int i = 0; i < workload_num; i++) {
        pre_total_ipc += 1/(CPI+workload[i].miss_ratio*workload[i].access_rate*PENALTY);
    } // printf("%15s\t%s\t%lf\t%lf\t%lf\n", workload[i].name,
      // workload[i].allocation, workload[i].access_rate,
      // workload[i].miss_ratio,workload[i].occ);
    return pre_total_ipc;
}

double predict_total_miss_rate() {
    char filename[100];
    memset(segment, 0, sizeof(segment));
    memset(occupancy, 0, sizeof(occupancy));

    for(int i=0;i<workload_num;i++) workload[i].ways = count_1s(workload[i].cos);
    segmentation();
    init_occupancy();
    //get_accessrate();
    // iteration process
    /*
    for (accesses = 1000; accesses >= 100; accesses -= 1) {
        o2m();
        m2o();
    }
    */
    accesses = 20000;
    for (int i = 0; i < 200; i++) {
        o2m();
        m2o();
        /*
        for( int j = 0; j< workload_num; j++ ){
            if(j==0)printf("%d\n",(int)workload[j].occ);
        }
        */
        // printf("\n");
 /*       if (i % 10 == 0)
            accesses-=150;
   */
	 }
    o2m();

    double pre_total_miss_ratio = 0;
    for (int i = 0; i < workload_num; i++) {
        pre_total_miss_ratio += workload[i].miss_ratio*workload[i].access_rate*1000;
    
	 //printf("%15s\t%llx\t%lf\t%.20lf\t%lf\n", workload[i].name,
       //workload[i].cos, workload[i].access_rate,
      // workload[i].miss_ratio,workload[i].occ);
	}
	 return pre_total_miss_ratio;
}

void predict_all(double CPI,double PENALTY,double *miss,double *ipc,double *ws,double *ms,double *fs) {
    char filename[100];
    memset(segment, 0, sizeof(segment));
    memset(occupancy, 0, sizeof(occupancy));

    for(int i=0;i<workload_num;i++) workload[i].ways = count_1s(workload[i].cos);
    segmentation();
    init_occupancy();
    //get_accessrate();
    // iteration process
    /*
    for (accesses = 1000; accesses >= 100; accesses -= 1) {
        o2m();
        m2o();
    }
    */
    accesses = 1000;
    for (int i = 0; i < 8000; i++) {
        o2m();
        m2o();
        /*
        for( int j = 0; j< workload_num; j++ ){
            if(j==0)printf("%d\n",(int)workload[j].occ);
        }
        */
        // printf("\n");
        if (i % 10 == 0)
            accesses--;
    }
    o2m();
    double pre_total_miss_ratio = 0;
    for (int i = 0; i < workload_num; i++) {
        pre_total_miss_ratio += workload[i].miss_ratio*workload[i].access_rate;
    }

    double pre_total_ipc = 0;
    for (int i = 0; i < workload_num; i++) {
        //printf("%lf ",workload[i].ipc);
        //if(i==workload_num-1)printf("\n");
        pre_total_ipc += workload[i].ipc;
    }

    double total_weighted_slowdown = 0;
    for (int i = 0; i < workload_num; i++) {
	workload[i].weighted_slowdown = workload[i].base_ipc/workload[i].ipc;
    //printf("%lf ",workload[i].weighted_slowdown);
    //if(i==workload_num-1) printf("\n");
    total_weighted_slowdown += workload[i].weighted_slowdown;
    }
    total_weighted_slowdown /= workload_num;

    double max_weighted_slowdown = 0;
    for (int i = 0; i < workload_num; i++) {
    //printf("ws[%d]: %lf, max: %lf\n",i,workload[i].weighted_slowdown,max_weighted_slowdown);
	if(workload[i].weighted_slowdown>max_weighted_slowdown) max_weighted_slowdown = workload[i].weighted_slowdown;
    }

    double fair_weighted_slowdown = 0;
    for (int i = 0; i < workload_num; i++) {
        fair_weighted_slowdown += 1/workload[i].weighted_slowdown;
    }
    fair_weighted_slowdown = workload_num/fair_weighted_slowdown;
    *miss = pre_total_miss_ratio;
    *ipc = pre_total_ipc;
    *ws = total_weighted_slowdown;
    *ms = max_weighted_slowdown;
    *fs = fair_weighted_slowdown;
}
