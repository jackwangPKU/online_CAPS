#include<iostream>
#include<fstream>
#include<cstring>
#include<cstdlib>
#include<iomanip>
using namespace std;
long long total = 0;
const int  MAXT = 512000+3;
const int CacheLineSize = 1024 * 1024 *20/64;
const int PGAP = 1024/64;
const int domain = 25600;
ifstream fin;
ofstream fout;
long long rtd[MAXT];
long long n=0;
char filename[50];

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


void print_mrc(ofstream &f){
    double sum = 0; long long T = 0;
    double tot = 0;
    double N = n;
    long long step = 1; int dom = 0,dT = 0,loc = 0;
    long long m = CacheLineSize;
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
        if (c%PGAP==0) f << std::setprecision(6) << 1.0*(N-sum)/N << endl;//fprintf(fout,"%.6lf\n",1.0*(N-sum)/N);
    }
	//fout.close();
}

int main(int argv, char **argc)
{
	if(argv<4){
		printf("usage: ./mrc [benchname] [num in a phase] [num of total]\n");
	}
	int sum = atoi(argc[3]);
	int phase = atoi(argc[2]);
	strcpy(filename,argc[1]);
	memset(rtd,0,sizeof(rtd));
	int _count = 0;
	int out_count = 0;
	for(int i = 0; i < sum; i++){
		cout<<"file num: "<<i<<endl;
		_count ++;
		char file[50];
		char index[10];
		sprintf(index,"%d.rth",i);
		strcpy(file,filename);
		strcat(file,index);
		fin.open(file);
		if(!fin){
			cout<<"Open file error"<<endl;
			exit(-1);
		}
		int tmp;
		for(int k = 0; !fin.eof(); k++){
			if(k>MAXT){
				cout<<"error rtd index"<<endl;
				exit(-1);
			}
			fin >> tmp;
			rtd[k] += tmp;
			n+=tmp;
			total++;
		}
		fin.close();
		if(_count == phase ){
			char _index[10];
			char out_file[50];
			sprintf(_index,"%d.mrc",out_count);
			strcpy(out_file,filename);
			strcat(out_file,_index);
			fout.open(out_file);
			if(!fout){
				cout<<"Open file error"<<endl;
				exit(-1);
			}
cout<<"OK"<<endl;
			print_mrc(fout);
			out_count++;
			_count = 0;
			n=0;
			memset(rtd,0,sizeof(rtd));
			fout.close();	
		}
	}
	cout<<"trace size: "<<total<<endl;
}

