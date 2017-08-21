#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sys/time.h>
using namespace std;
typedef unsigned long long uint64_t;
const int PGAP = 1024/64; // 1kb
const int MAXT = 10000+3;
const int MAXH = 19999997;
const int domain = 256;
const int CacheLineSize = 30*1024*1024/64;

struct node {
    uint64_t addr;
	long long label;
    node *nxt;
    node(uint64_t _addr = 0, long long _label = 0, node *_nxt = NULL)
         : addr(_addr),label(_label),nxt(_nxt) {}
};

struct tnode {
    uint64_t offset;
};

node *hash[MAXH];
FILE *fin,*fout;
//ifstream infile;
long long rtd[MAXT];
long long n,m;

long long domain_value_to_index(long long value)
{
    long long loc = 0,step = 1;
    int index = 0;
    while (loc+step*domain<value) {
        loc += step*domain;
        step *= 2;
        index += domain;
    }
    while (loc<value) index++,loc += step;
    return index;
}

long long domain_index_to_value(long long index)
{
    long long value = 0,step = 1;
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

void insert(uint64_t now)
{
    int t = now%MAXH;
    node *tmp = new node(now,n,hash[t]);
    hash[t] = tmp;
}

long long find(uint64_t now)
{
    int t = now%MAXH;
    node *tmp = hash[t],*pre = NULL;
    while (tmp) {
        if (tmp->addr==now) {
            long long tlabel = tmp->label;
            if (pre==NULL) hash[t] = tmp->nxt;
            else pre->nxt = tmp->nxt;
            delete tmp;
            return tlabel;
        }
        pre = tmp;
        tmp = tmp->nxt;
    }
    return 0;
}

void solve()
{
    memset(rtd,0,sizeof(rtd));
    n = 0;
    uint64_t addr;
    tnode data;
	printf("OK\n");
    while (fread(&data,sizeof(data),1,fin)) {
	//while(!infile.eof()){
        //printf("%d\n",n);
		//infile.read((char*)&data,sizeof(data));
		addr = data.offset>>6;
        n++;
        //if (n%100000000==0) printf("%lld\n",n);
		long long t = find(addr);
        if (t) rtd[domain_value_to_index(n-t)]++;
        insert(addr);
    }

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
    char filename[100] = "input.txt";
    if (argv>=3) {
        strcpy(filename,argc[1]);
    }
    else{
        printf("input filename\n");
        exit(-1);
    }
    fin = fopen(filename,"rb");
	//infile.open(filename,ios::binary);
    strcpy(filename,argc[2]);
    fout = fopen(filename,"w");
    solve();
	//infile.close();
    fclose(fin);
	/*
	strcpy(filename,argc[3]);
	fout = fopen(filename,"w");
	for(int i = 0;i<MAXT;i++)
		fprintf(fout,"%lld\n",rtd[i]);
	*/
    fclose(fout);
    return 0;
}
