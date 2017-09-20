#ifndef _PREDICT_H_
#define _PREDICT_H_
    #include "predict.h"
#endif

extern Workload workload[MAXN];
extern int workload_num;
extern bool need_calc_ar;


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
// direction: 0 right expand, 1 right reduce, 2 left expand, 3 left reduce
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

void display(){
    for(int i=0; i<workload_num; i++){
        printf("%15s: %s\n",workload[i].name,cos2Pic(workload[i].cos));
    }
    printf("\n");
}

int main(int argv, char **argc) {
    char filename[100];
    char target[100];
    srand(time(NULL));

    workload_num = argv / 2 - 1;
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
        strcpy(filename, workload[i].name);
        strcpy(target,"./mrc/");
        strcat(filename, "_real.txt");
        strcat(target,filename);
        fin = fopen(target, "rb");
        if(!fin){
            printf("file not exist\n");
            exit(-1);
        }
        // calc_mrc(i);
        get_mrc(i,fin);
        if (need_calc_ar) {
            workload[i].access_rate = workload[i].mrc[L2_CACHE_SIZE / BLOCK];
        } else {
            workload[i].access_rate = 1;
        }
        fclose(fin);
    }
    get_accessrate();
    get_baseIPC();
    best = predict_total_miss_rate();
    cur_miss_rate = best;
    int _count = 0;
    while (T >= T_min) {
        int target = rand() % workload_num;
        int direction =
            rand() %
            4; // 0 right expand, 1 right reduce, 2 left expand, 3 left reduce
        uint64_t pre_cos = modifyCos(target, direction);
        for(int i = 0; i<workload_num; i++){
            printf("%s %s ",workload[i].name, ull216Str(workload[i].cos));
            if(i==workload_num-1) printf("\n");
        }
        printf("single miss_rate:\n");
        for(int i = 0; i <workload_num; i++){
            printf("%s: %lf ",workload[i].name, workload[i].miss_ratio);
            if(i==workload_num-1) printf("\n");
        }
        display();
        double tmp = predict_total_miss_rate();
        printf("miss_rate: %lf\n",tmp);
        if (tmp < best)
            best = tmp;
        double df = tmp - cur_miss_rate;
        if ((df > 0) && ((rand() % 1000 / (float)1000) > exp(-df / (k * T)))) {
            workload[target].cos = pre_cos;
        } else {
            cur_miss_rate = tmp;
        }
        if(_count==100){
            T*=0.95;
            _count = 0;
        }
        else _count++;
    }

    printf("\n\nThe best: %lf\n", best);

    return 0;
}
