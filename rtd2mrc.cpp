#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sys/time.h>
using namespace std;
typedef unsigned long long uint64_t;
const int PGAP = 1024/64; // 1kb
const int MAXT = 512000+3;
const int domain = 25600;
const int CacheLineSize = 30*1024*1024/64;

FILE *fin,*fout;
long long rtd[MAXT];
	
void solve()
{
    long long n,m;
	memset(rtd,0,sizeof(rtd));
    n = 0;
	long long i = 0;

    while (i<MAXT && fscanf(fin,"%lld",&rtd[i])) {
		//printf("%lld %lld\n",i,rtd[i]);
		n += rtd[i];
		i++;
    }
	//printf("%lld\n",n);
    double sum = 0; long long T = 0;
    double tot = 0;
    double N = n;
    long long step = 1; int dom = 0,dT = 0,loc = 0;
    m = CacheLineSize;
    for (long long c = 1; c<=m; c++) {
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
        //ans[c] = 1.0*(N-sum)/N;
        if (c%PGAP==0) fprintf(fout,"%.6lf\n",1.0*(N-sum)/N);
    }

}

int main(int argv, char **argc)
{
	int num = 0;
	char benchname[50] = "bench";
    char filename[100] = "input.txt";
    if (argv>=3) {
        strcpy(benchname,argc[1]);
		num = atoi(argc[2]);
    }
    else{
        printf("usage: ./rth2mrc [benchname] number\n");
        exit(-1);
    }
	for(int i=0;i<num;i++){
		sprintf(filename,"%s%d.rth",benchname,i);
		fin = fopen(filename,"rt");
		sprintf(filename,"%s%d.txt",benchname,i);
		fout = fopen(filename,"w");
		solve();
		fclose(fin);
		fclose(fout);
	}
    return 0;
}
