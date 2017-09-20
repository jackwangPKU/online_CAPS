#ifndef _PREDICT_H_
#define _PREDICT_H_
    #include "predict.h"
#endif

uint64_t m;
Segment segment[WAY+1];
int segment_num;
Workload workload[MAXN];
int workload_num;
double occupancy[MAXN][WAY+1];
uint64_t accesses;
bool need_calc_ar;
double CPI = 0.58;
double PENALTY = 50;

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
    for (int i = 0; i < segment_num; i++) {
        uint64_t segment_ways = segment[i].ends - segment[i].begins + 1;
        uint64_t segment_blocks = segment_ways * WAY_SIZE / BLOCK;

        for (set<int>::iterator iter = segment[i].workload.begin();
             iter != segment[i].workload.end(); iter++) {
            int wid = *iter;
            occupancy[wid][i] =
                (double)segment_blocks / segment[i].workload.size();
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
        workload[i].miss_ratio = workload[i].mrc[(uint64_t)occ];
        workload[i].ipc = 1/(CPI+workload[i].miss_ratio*workload[i].access_rate*PENALTY);
        //printf("in o2m workload[%d] , miss_ratio: %lf ipc: %lf\n",i,workload[i].miss_ratio,workload[i].ipc);
    }
}

void m2o() {
    for (int i = 0; i < segment_num; i++) {
        uint64_t segment_ways = segment[i].ends - segment[i].begins + 1;
        uint64_t segment_blocks = segment_ways * WAY_SIZE / BLOCK;
        double miss_num[MAXN], miss_num_total = 0;

        for (set<int>::iterator iter = segment[i].workload.begin();
             iter != segment[i].workload.end(); iter++) {
            int wid = *iter;
            miss_num[wid] =
                (workload[wid].miss_ratio * workload[wid].access_rate * workload[wid].ipc * accesses * segment_ways / workload[wid].ways);
            // printf("miss_num %lf\n",miss_num[wid]);
            miss_num_total += miss_num[wid];
            // printf("workload[%d].miss_ratio: %lf, workload[%d].access_rate:
            // %lf,accesses: %d, segment_ways: %d, workload[%d].ways: %d,
            // miss_num[%d]:
            // %d\n",wid,workload[wid].miss_ratio,wid,workload[wid].access_rate,accesses,segment_ways,wid,workload[wid].ways,wid,miss_num[wid]);
        }
        // printf("miss_num_total: %d\n",miss_num_total);

        for (set<int>::iterator iter = segment[i].workload.begin();
             iter != segment[i].workload.end(); iter++) {
            int wid = *iter;
            occupancy[wid][i] = occupancy[wid][i] +
                                miss_num[wid] *
                                    (segment_blocks - occupancy[wid][i]) /
                                    segment_blocks -
                                (miss_num_total - miss_num[wid]) *
                                    occupancy[wid][i] / segment_blocks;
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
    } // printf("%15s\t%s\t%lf\t%lf\t%lf\n", workload[i].name,
      // workload[i].allocation, workload[i].access_rate,
      // workload[i].miss_ratio,workload[i].occ);
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
