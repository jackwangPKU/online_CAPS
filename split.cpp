#include<cstdio>
#include<cstring>
#include<cstdlib>
#include<fstream>
#include<stdlib.h>
using namespace std;
int main(int argv, char ** argc){
	FILE *fin,*fout;
	char filename[100] = "input.txt";
	if(argv >= 5){
		strcpy(filename,argc[1]);
	}
	else{
		printf("usage: ./split input output size number\n");
	}
	int size = atoi(argc[3]);
	int num = atoi(argc[4]);
	unsigned long long data;
	char output[100] = "output";
	char filename2[100] = "output.txt";
	strcpy(output,argc[2]);
	int n = 0;
	int c = 0;
	int _count = 0;
	int m = size*1024*1024/8;
	strcpy(filename2,output);
	char _c[10];
	sprintf(_c,"%d",c);
	strcat(filename2,_c);
	strcat(filename2,".ref");
	fin = fopen(filename,"rb");
	fout = fopen(filename2,"wb");
	while(fread(&data,sizeof(data),1,fin)){
		if(n == num){
			fclose(fin);
			return 0;
		}
		if(_count < m){
			fwrite(&data,sizeof(data),1,fout);
			_count ++;
		}
		else{
			n++;
			_count = 0;
			strcpy(filename2,output);
			c++;
			sprintf(_c,"%d",c);
			strcat(filename2,_c);

			//strcat(filename2,itoa(c));
			strcat(filename2,".ref");
			fclose(fout);
			fout = fopen(filename2,"wb");
		}	
	}
}
