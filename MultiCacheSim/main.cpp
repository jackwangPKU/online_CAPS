#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <string>
#include "system.h"
#include <string.h>
using namespace std;

int main(int ac, char **av)
{
   // tid_map is used to inform the simulator how
   // thread ids map to NUMA/cache domains. Using
   // the tid as an index gives the NUMA domain.
    printf("OK\n");
    if(ac < 3){
        printf("input filepath for read and write\n");
        exit(-1);
    }
    char filename[100]="";
    //printf("%s\n",av[1]);
    strcpy(filename,av[1]);
    //printf("%s\n",filename);
    unsigned int arr_map[] = {0};
   vector<unsigned int> tid_map(arr_map, arr_map +
         sizeof(arr_map) / sizeof(unsigned int));
   SeqPrefetch prefetch;
   // The constructor parameters are:
   // the tid_map, the cache line size in bytes,
   // number of cache lines, the associativity,
   // the prefetcher object,
   // whether to count compulsory misses,
   // whether to do virtual to physical translation,
   // and number of caches/domains
   // WARNING: counting compulsory misses doubles execution time
   SingleCacheSystem sys(tid_map, 64, 4096, 8, &prefetch, false, false);
   char rw;
   uint64_t address;
   unsigned long long lines = 0;
   ifstream infile;
   // This code works with the output from the
   // ManualExamples/pinatrace pin tool
   infile.open(filename, ifstream::in);
   assert(infile.is_open());

   while(!infile.eof())
   {
      infile.ignore(256, ':');
      infile >> rw;
      assert(rw == 'R' || rw == 'W');
      infile >> hex >> address;
      if(address != 0) {
         // By default the pinatrace tool doesn't record the tid,
         // so we make up a tid to stress the MultiCache functionality
         sys.memAccess(address, rw, lines);
      }

      //++lines;
   }
   sys.returntrace(av[2]);
   infile.close();

   return 0;
}
