#include<time.h>
#include<cstdio>
#include<iostream>
#include<sys/time.h>
#include<cstdlib>
#include<cstring>
using namespace std;

struct Sample{
    unsigned long long addr[10];
};

FILE *fin,*fout;
int main(int argv, char **argc){
    char filename[100] = "input.txt";
    if(argv < 4){
        cout<<"usage: ./sample [input_file] [output_file] [sample frequence (>10)]"<<endl;
        exit(-1);
    }
    int freq = atoi(argc[3])/10;
    strcpy(filename,argc[1]);
    fin = fopen(filename,"rb");
    strcpy(filename,argc[2]);
    fout = fopen(filename,"wb");
    int c = 0;
    Sample sample;
    while(fread(&sample,sizeof(sample),1,fin)){
        if(c==0){
            srand(time(NULL));
            int index = rand()%10;
            fprintf(fout,"%lld",sample.addr[index]);
        }
        c = (c+1)%freq;
    }
    fclose(fin);
    fclose(fout);
}
