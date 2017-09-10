#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sys/time.h>
using namespace std;
typedef unsigned long long uint64_t;
const int PGAP = 1024/64; // 1kb
const int MAXT = 512000+3;
const int MAXH = 19999997;
const int domain = 25600;
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
char benchname[50];
int rth_count = 0;
long long phase_count = 0;
long long PHASE = 0;
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


void print_rth(){
	char filename[100];
	strcpy(filename,benchname);
	char index[20];
	sprintf(index,"%d.rth",rth_count);
	strcat(filename,index);
	if(rth_count)fclose(fout);
	rth_count++;
	fout = fopen(filename,"w");
	for(int i = 0; i < MAXT; i++){
		fprintf(fout,"%lld\n",rtd[i]);		
	}
}

void solve()
{
    memset(rtd,0,sizeof(rtd));
    n = 0;
    uint64_t addr;
    tnode data;
	printf("OK\n");
    while (fread(&data,sizeof(data),1,fin)) {
		if(phase_count>=PHASE){
			print_rth();
			phase_count=0;
			memset(rtd,0,sizeof(rtd));
		}
		addr = data.offset>>6;
        n++;
		phase_count++;
		long long t = find(addr);
        if (t) rtd[domain_value_to_index(n-t)]++;
        insert(addr);
    }
	if(phase_count >= PHASE/2)
		print_rth();

}

int main(int argv, char **argc)
{
    char filename[100] = "input.txt";
    if (argv>=3) {
        strcpy(benchname,argc[1]);
		strcpy(filename,benchname);
		strcat(filename,".ref");
		PHASE = atoll(argc[2]);
    }
    else{
        printf("usage: ./rth [benchname] [phase length]\n");
        exit(-1);
    }

    fin = fopen(filename,"rb");
	if(!fin){
		printf("input file not exist\n");
		exit(-1);
	}
	//infile.open(filename,ios::binary);
    //strcpy(filename,argc[2]);
    //fout = fopen(filename,"w");
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
