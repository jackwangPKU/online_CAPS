#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/time.h>
#include <ctime>
using namespace std;

const int PGAP = 1024/64;
const int MAXL = 100000+3;
const int MAXH = 8192;
const int domain = 256;
const int STEP = 10000;
const int CacheLineSize = 30 * 1024 * 1024 / 64;
struct node {
    unsigned long long addr,label,ed;
    node *pre,*nxt;
    node(unsigned long long _addr = 0, unsigned long long _label = 0, node *_pre = NULL, node *_nxt = NULL)
         : addr(_addr),label(_label),ed(-1),pre(_pre),nxt(_nxt) {}
};

struct tnode {
    unsigned long long offset;
};

node *hash[MAXH],tar[MAXH];
int insnum,qlen;
FILE *fin,*fout;
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
    node *tmp = tar+loc;
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
    node *tmp = hash[t];
    while (tmp) {
        if (tmp->addr==now) {
            if (tmp->ed==-1) tmp->ed = n;
            return;
        }
        tmp = tmp->nxt;
    }
    return;
}

void solve()
{
    qlen = 0; insnum = 0;
    //rtd = new long long[MAXL];
    memset(rtd,0,sizeof(unsigned long long)*MAXL);
    m = 0; n = 0;
    unsigned long long tm,now,size,tott = 0;
    srand(time(NULL));
    //timeval start,end;
    //gettimeofday(&start, NULL );
    unsigned long long loc = rand()%(STEP*2)+1;
    tnode data;
    while (fread(&data,sizeof(data),1,fin)) {
        now = data.offset>>6;
		n++;
    
            find(now);
            if (n==loc) insert(now),loc += rand()%(STEP*2)+1;
    }
    //gettimeofday(&end, NULL );
    //long long timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
    for (int i = 0; i<qlen; i++)
        if (tar[i].ed!=-1) tott++,rtd[domain_value_to_index(tar[i].ed-tar[i].label)]++;
    m = 1.0*(qlen-tott)/qlen*n;

    double sum = 0; unsigned long long T = 0;
    double tot = 0;
    double N = tott+1.0*tott/(n-m)*m;
    unsigned long long step = 1; int dom = 0,dT = 0; loc = 0;
    for (unsigned long long c = 1; c<=m; c++) {
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

    //FILE *fsz = fopen("dsize.txt","a");
    //fprintf(fsz,"AET8k%d %lld %.3f %.3f\n",STEP,qlen,(qlen*5+MAXH)*8/1024.0,timeuse/1000000.0);
    //fclose(fsz);
}

int main(int argv, char **argc)
{
    char filename[100] = "input.txt";
    if (argv>=3) {
        strcpy(filename,argc[1]);
    }
    fin = fopen(filename,"rb");
	strcpy(filename,argc[2]);
    fout = fopen(filename,"w");
    solve();
    fclose(fin);
    fclose(fout);
    return 0;
}
