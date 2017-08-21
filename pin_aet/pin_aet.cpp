#include "pin.H"
#include <iostream>
#include <fstream>

#include "pin_profile.H"

#include <cassert>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <cstdint>
#include <vector>
#include <set>
#include <list>
#include <deque>
#include <set>
#include <unordered_map>
//#include <sys/time.h>
//using namespace std;
//typedef unsigned long long uint64_t;
const int PGAP = 1024/64; // 1kb
const int MAXT = 512000+3;
const int MAXH = 19999997;
const int domain = 25600;
const int CacheLineSize = 30*1024*1024/64;

ofstream outFile;
char benchname[100];
int rth_count = 0;
long long phase_count = 0;
//misc.h
#define PAGE_SIZE_4KB

#ifdef PAGE_SIZE_4KB
const uint64_t PAGE_MASK = 0xFFFFFFFFFFFFF000;
const uint32_t PAGE_SHIFT = 12;
#elif defined PAGE_SIZE_2MB
const uint64_t PAGE_MASK = 0xFFFFFFFFFFE00000;
const uint32_t PAGE_SHIFT = 21;
#else
#error "Bad PAGE_SIZE"
#endif

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

node *_hash[MAXH];
//FILE *fin,*fout;
//ifstream infile;
long long rtd[MAXT];
long long n=0,m=CacheLineSize;

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
    node *tmp = new node(now,n,_hash[t]);
    _hash[t] = tmp;
}

long long find(uint64_t now)
{
    int t = now%MAXH;
    node *tmp = _hash[t];
	node *pre = NULL;
    while (tmp) {
        if (tmp->addr==now) {
            long long tlabel = tmp->label;
            if (pre==NULL) _hash[t] = tmp->nxt;
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
	if(rth_count)outFile.close();
	rth_count++;
	outFile.open(filename);
	for(int i = 0; i < MAXT; i++){
		outFile << rtd[i] << endl;		
	}
}

void solve(uint64_t addr){
    //memset(rtd,0,sizeof(rtd));
    //n = 0;
    	n++;
	phase_count++;
	long long t = find(addr);
        if (t) rtd[domain_value_to_index(n-t)]++;
        insert(addr);
	if(phase_count>=1000000000){//print rth
		print_rth();
		phase_count=0;
		memset(rtd,0,sizeof(rtd));
	}
}
void print_mrc(ofstream &outFile){
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
        if (c%PGAP==0) outFile << std::setprecision(6) << 1.0*(N-sum)/N << endl;
    }

}

enum cacheState {MOD,OWN,EXC,SHA,INV};

struct cacheLine{
   uint64_t tag;
   cacheState state;
   cacheLine(){tag = 0; state = INV;}
   bool operator<(const cacheLine& rhs) const
   { return tag < rhs.tag; }
   bool operator==(const cacheLine& rhs) const
   { return tag == rhs.tag; }
};
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//cache.h
class Cache{
public:
   virtual ~Cache(){}
   virtual cacheState findTag(uint64_t set,
                                 uint64_t tag) const = 0;
   virtual void changeState(uint64_t set,
                              uint64_t tag, cacheState state) = 0;
   virtual void updateLRU(uint64_t set, uint64_t tag) = 0;
   virtual bool checkWriteback(uint64_t set,
      uint64_t& tag) const = 0;
   virtual void insertLine(uint64_t set,
                              uint64_t tag, cacheState state) = 0;
};

class SetCache : public Cache{
   std::vector<std::set<cacheLine> > sets;
   std::vector<std::list<uint64_t> > lruLists;
   std::vector<std::unordered_map<uint64_t,
      std::list<uint64_t>::iterator> > lruMaps;
public:
   SetCache(unsigned int num_lines, unsigned int assoc);
   cacheState findTag(uint64_t set, uint64_t tag) const;
   void changeState(uint64_t set, uint64_t tag,
                     cacheState state);
   void updateLRU(uint64_t set, uint64_t tag);
   bool checkWriteback(uint64_t set, uint64_t& tag) const;
   void insertLine(uint64_t set, uint64_t tag,
                     cacheState state);
};

class DequeCache : public Cache{
   std::vector<std::deque<cacheLine> > sets;
public:
   DequeCache(unsigned int num_lines, unsigned int assoc);
   cacheState findTag(uint64_t set, uint64_t tag) const;
   void changeState(uint64_t set, uint64_t tag,
                    cacheState state);
   void updateLRU(uint64_t set, uint64_t tag);
   bool checkWriteback(uint64_t set, uint64_t& tag) const;
   void insertLine(uint64_t set, uint64_t tag,
                     cacheState state);
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//prefetch.h
class System;

class Prefetch {
public:
   virtual int prefetchMiss(uint64_t address, unsigned int tid, 
                              System* sys)=0;
   virtual int prefetchHit(uint64_t address, unsigned int tid,
                              System* sys)=0;
};

//"Prefetcher" that does nothing
class NullPrefetch : public Prefetch {
public:
   int prefetchMiss(uint64_t address, unsigned int tid, 
                              System* sys);
   int prefetchHit(uint64_t address, unsigned int tid,
                              System* sys);
};

// Modeling AMD's L1 prefetcher, a sequential
// line prefetcher. Primary difference is that
// thre real prefetcher has a dynamic prefetch width.
class SeqPrefetch : public Prefetch {
   uint64_t lastMiss;
   uint64_t lastPrefetch;
   static const  int prefetchNum = 3;
public:
   int prefetchMiss(uint64_t address, unsigned int tid, System* sys);
   int prefetchHit(uint64_t address, unsigned int tid, System* sys);
   SeqPrefetch()
   { lastMiss = lastPrefetch = 0;}
};

// A simple adjacent line prefetcher
class AdjPrefetch : public Prefetch {
public:
   int prefetchMiss(uint64_t address, unsigned int tid, System* sys);
   int prefetchHit(uint64_t address, unsigned int tid, System* sys);
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//system.h
struct SystemStats {
   unsigned long long hits;
   unsigned long long local_reads;
   unsigned long long remote_reads;
   unsigned long long othercache_reads;
   unsigned long long local_writes;
   unsigned long long remote_writes;
   unsigned long long compulsory;
   unsigned long long prefetched;
   SystemStats()
   {
      hits = local_reads = remote_reads = othercache_reads =
         local_writes = remote_writes = compulsory = prefetched = 0;
   }
};

class System {
protected:
   friend class Prefetch;
   friend class SeqPrefetch;
   friend class AdjPrefetch;
   Prefetch* prefetcher;
   uint64_t SET_MASK;
   uint64_t TAG_MASK;
   uint64_t LINE_MASK;
   unsigned int SET_SHIFT;
   std::vector<unsigned int> tid_to_domain;
   //The cutoff associativity for using the deque cache implementation
   static const unsigned int assocCutoff = 64;

   // Used for compulsory misses
   std::set<uint64_t> seenLines;
   // Stores virtual to physical page mappings
   std::map<uint64_t, uint64_t> virtToPhysMap;
   // Used for determining new virtual to physical mappings
   uint64_t nextPage;
   bool countCompulsory;
   bool doAddrTrans;

   uint64_t virtToPhys(uint64_t address);
   void checkCompulsory(uint64_t line);
public:
   System(std::vector<unsigned int> tid_to_domain,
            unsigned int line_size, unsigned int num_lines, unsigned int assoc,
            Prefetch* prefetcher, bool count_compulsory=false,
            bool do_addr_trans=false);
   virtual void memAccess(uint64_t address, char rw,
                           unsigned int tid, bool is_prefetch=false) = 0;
   SystemStats stats;
};


//For a system containing a sinle cache
//  performs about 10% better than the MultiCache implementation
class SingleCacheSystem : public System {
   Cache* cache;
   //vector<unsigned long long> trace;

public:
   SingleCacheSystem(std::vector<unsigned int> tid_to_domain,
            unsigned int line_size, unsigned int num_lines, unsigned int assoc,
            Prefetch* prefetcher, bool count_compulsory=false,
            bool do_addr_trans=false);
   ~SingleCacheSystem();
   void memAccess(uint64_t address, char rw, unsigned int tid,
                     bool is_prefetch=false);
/*
   vector<unsigned long long>& gettrace(){
		return trace;
   }
*/
};
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//cache.cpp
SetCache::SetCache(unsigned int num_lines, unsigned int assoc)
{
   assert(num_lines % assoc == 0);
   // The set bits of the address will be used as an index
   // into sets. Each set is a set containing "assoc" items
   sets.resize(num_lines / assoc);
   lruLists.resize(num_lines / assoc);
   lruMaps.resize(num_lines / assoc);
   for(unsigned int i=0; i < sets.size(); i++) {
      for(unsigned int j=0; j < assoc; ++j) {
         cacheLine temp;
         temp.tag = j;
         temp.state = INV;
         sets[i].insert(temp);
         lruLists[i].push_front(j);
         lruMaps[i].insert(make_pair(j, lruLists[i].begin()));
      }
   }
}

/* FIXME invalid vs not found */
// Given the set and tag, return the cache lines state
cacheState SetCache::findTag(uint64_t set, 
                              uint64_t tag) const
{
   cacheLine temp;
   temp.tag = tag;
   std::set<cacheLine>::const_iterator it = sets[set].find(temp);

   if(it != sets[set].end()) {
      return it->state;
   }

   return INV;
}

/* FIXME invalid vs not found */
// Changes the cache line specificed by "set" and "tag" to "state"
void SetCache::changeState(uint64_t set, uint64_t tag, 
                              cacheState state)
{
   cacheLine temp;
   temp.tag = tag;
   std::set<cacheLine>::const_iterator it = sets[set].find(temp);

   if(it != sets[set].end()) {
      cacheLine *target;
      target = (cacheLine*)&*it;
      target->state = state;
   }
}

// A complete LRU is mantained for each set, using a separate
// list and map. The front of the list is considered most recently used.
void SetCache::updateLRU(uint64_t set, uint64_t tag)
{
   std::unordered_map<uint64_t, 
      std::list<uint64_t>::iterator>::iterator map_it;
   std::list<uint64_t>::iterator it;
   uint64_t temp;

   map_it = lruMaps[set].find(tag);

   #ifdef DEBUG
   assert(map_it != lruMaps[set].end());
   cacheState foundState = findTag(set, tag);
   assert(foundState != INV);
   #endif

   it = map_it->second;

   #ifdef DEBUG
   assert(it != lruLists[set].end());
   #endif

   temp = *it;

   lruLists[set].erase(it);
   lruLists[set].push_front(temp);

   lruMaps[set].erase(map_it);
   lruMaps[set].insert(make_pair(tag, lruLists[set].begin()));
}

// Called if a new cache line is to be inserted. Checks if
// the least recently used line needs to be written back to
// main memory.
bool SetCache::checkWriteback(uint64_t set, 
                                 uint64_t& tag) const
{
   cacheLine evict, temp;
   tag = lruLists[set].back();
   temp.tag = tag;
   evict = *sets[set].find(temp);

   return (evict.state == MOD || evict.state == OWN);
}

// FIXME: invalid vs not found
// Insert a new cache line by popping the least recently used line
// and pushing the new line to the back (most recently used)
void SetCache::insertLine(uint64_t set, uint64_t tag, 
                           cacheState state)
{
   uint64_t to_evict = lruLists[set].back();
   cacheLine newLine, temp;
   newLine.tag = tag;
   newLine.state = state;
   temp.tag = to_evict;

   sets[set].erase(temp);
   sets[set].insert(newLine);
   
   lruMaps[set].erase(to_evict);
   lruLists[set].pop_back();
   lruLists[set].push_front(tag);
   lruMaps[set].insert(make_pair(tag, lruLists[set].begin()));
}

DequeCache::DequeCache(unsigned int num_lines, unsigned int assoc)
{
   assert(num_lines % assoc == 0);
   // The set bits of the address will be used as an index
   // into sets. Each set is a deque containing "assoc" items
   sets.resize(num_lines / assoc);
   for(unsigned int i=0; i < sets.size(); i++) {
      sets[i].resize(assoc);
   }
}

// Given the set and tag, return the cache lines state
// INVALID and "not found" are equivalent
cacheState DequeCache::findTag(uint64_t set, 
                                 uint64_t tag) const
{
   std::deque<cacheLine>::const_iterator it = sets[set].begin();

   for(; it != sets[set].end(); ++it)
   {
      // The cache may hold many invalid entries of a given line
      // and 1 or 0 valid entries. Return the valid entry if it
      // exists.
      if(it->tag == tag && it->state != INV)
         return it->state;
   }

   return INV;
}

// Changes the cache line specificed by "set" and "tag" to "state"
void DequeCache::changeState(uint64_t set, uint64_t tag, 
                              cacheState state)
{
   std::deque<cacheLine>::iterator it = sets[set].begin();

   for(; it != sets[set].end(); ++it)
   {
      // The cache may hold many invalid entries of a given line
      // and 1 or 0 valid entries. Use the valid entry
      if(it->tag == tag && it->state != INV)
         it->state = state;
   } 
}

// A complete LRU is mantained for each set, using the ordering
// of the set deque. The end is considered most
// recently used.
void DequeCache::updateLRU(uint64_t set, uint64_t tag)
{
   std::deque<cacheLine>::iterator it = sets[set].begin();
   cacheLine temp;

#ifdef DEBUG
   cacheState foundState = findTag(set, tag);
   assert(foundState != INV);
#endif

   for(; it != sets[set].end(); ++it)
   {
      // The cache may hold many invalid entries of a given line
      // and 1 or 0 valid entries. Use the valid entry.
      if(it->tag == tag && it->state != INV)
      {
         temp = *it;
         break;
      }
   } 

   sets[set].erase(it);
   sets[set].push_back(temp);
}

// Called if a new cache line is to be inserted. Checks if
// the least recently used line needs to be written back to
// main memory.
bool DequeCache::checkWriteback(uint64_t set, 
                                 uint64_t& tag) const
{
   cacheLine evict = sets[set].front();
   tag = evict.tag;

   return (evict.state == MOD || evict.state == OWN);
}

// Insert a new cache line by popping the least recently used line
// and pushing the new line to the back (most recently used)
void DequeCache::insertLine(uint64_t set, uint64_t tag, 
                              cacheState state)
{
   cacheLine newLine;
   newLine.tag = tag;
   newLine.state = state;

   sets[set].pop_front();
   sets[set].push_back(newLine);
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//system.cpp
System::System(std::vector<unsigned int> tid_to_domain,
            unsigned int line_size, unsigned int num_lines, unsigned int assoc,
            Prefetch* prefetcher, bool count_compulsory /*=false*/,
            bool do_addr_trans /*=false*/)
{
   assert(num_lines % assoc == 0);

   stats.hits = stats.local_reads = stats.remote_reads =
      stats.othercache_reads = stats.local_writes =
      stats.remote_writes = stats.compulsory = 0;

   LINE_MASK = ((uint64_t) line_size)-1;
   SET_SHIFT = log2(line_size);
   SET_MASK = ((num_lines / assoc) - 1) << SET_SHIFT;
   TAG_MASK = ~(SET_MASK | LINE_MASK);

   nextPage = 0;

   countCompulsory = count_compulsory;
   doAddrTrans = do_addr_trans;
   this->tid_to_domain = tid_to_domain;
   this->prefetcher = prefetcher;
}

void System::checkCompulsory(uint64_t line)
{
   std::set<uint64_t>::iterator it;

   it = seenLines.find(line);
   if(it == seenLines.end()) {
      stats.compulsory++;
      seenLines.insert(line);
   }
}

uint64_t System::virtToPhys(uint64_t address)
{
   std::map<uint64_t, uint64_t>::iterator it;
   uint64_t virt_page = address & PAGE_MASK;
   uint64_t phys_page;
   uint64_t phys_addr = address & (~PAGE_MASK);

   it = virtToPhysMap.find(virt_page);
   if(it != virtToPhysMap.end()) {
      phys_page = it->second;
      phys_addr |= phys_page;
   }
   else {
      phys_page = nextPage << PAGE_SHIFT;
      phys_addr |= phys_page;
      virtToPhysMap.insert(std::make_pair(virt_page, phys_page));
      //nextPage += rand() % 200 + 5 ;
      ++nextPage;
   }

   return phys_addr;
}



void SingleCacheSystem::memAccess(uint64_t address, char rw, unsigned
   int tid, bool is_prefetch /*=false*/)
{
   uint64_t set, tag;
   bool hit;
   cacheState state;

   if(doAddrTrans) {
      address = virtToPhys(address);
   }

   set = (address & SET_MASK) >> SET_SHIFT;
   tag = address & TAG_MASK;
   state = cache->findTag(set, tag);
   hit = (state != INV);

   if(countCompulsory && !is_prefetch) {
      checkCompulsory(address & LINE_MASK);
   }

   // Handle hits
   if(rw == 'W' && hit) {
      cache->changeState(set, tag, MOD);
   }

   if(hit) {
      cache->updateLRU(set, tag);
      if(!is_prefetch) {
         stats.hits++;
         stats.prefetched += prefetcher->prefetchHit(address, tid, this);
      }
      return;
   }
   else{
       //printf("%llx\n",address);
       //trace.push_back(address);
       solve(address);
   }

   cacheState new_state = INV;
   uint64_t evicted_tag;
   bool writeback = cache->checkWriteback(set, evicted_tag);

   if(writeback) {
      stats.local_writes++;
   }

   if(rw == 'R') {
      new_state = EXC;
   }
   else {
      new_state = MOD;
   }

   if(!is_prefetch) {
      stats.local_reads++;
   }

   cache->insertLine(set, tag, new_state);
   if(!is_prefetch) {
      stats.prefetched += prefetcher->prefetchMiss(address, tid, this);
   }
}

SingleCacheSystem::SingleCacheSystem(std::vector<unsigned int> tid_to_domain,
            unsigned int line_size, unsigned int num_lines, unsigned int assoc,
            Prefetch* prefetcher, bool count_compulsory /*=false*/,
            bool do_addr_trans /*=false*/) :
            System(tid_to_domain, line_size, num_lines, assoc,
               prefetcher, count_compulsory, do_addr_trans)
{
   if(assoc > assocCutoff) {
         cache = new SetCache(num_lines, assoc);
   }
   else {
         cache = new DequeCache(num_lines, assoc);
   }
}

SingleCacheSystem::~SingleCacheSystem()
{
   delete cache;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//prefetch.cpp

int NullPrefetch::prefetchMiss(uint64_t address __attribute__((unused)), 
                              unsigned int tid __attribute__((unused)),
                              System* sys __attribute__((unused)))
{
   return 0;
}
int NullPrefetch::prefetchHit(uint64_t address __attribute__((unused)), 
                           unsigned int tid __attribute__((unused)),
                           System* sys __attribute__((unused)))
{
   return 0;
}

int AdjPrefetch::prefetchMiss(uint64_t address, unsigned int tid,
                                 System* sys)
{
   sys->memAccess(address + (1 << sys->SET_SHIFT), 'R', tid, true);
   return 1;
}

// Called to check for prefetches in the case of a cache miss.
int SeqPrefetch::prefetchMiss(uint64_t address, unsigned int tid,
                                 System* sys)
{
   uint64_t set = (address & sys->SET_MASK) >> sys->SET_SHIFT;
   uint64_t tag = address & sys->TAG_MASK;
   uint64_t lastSet = (lastMiss & sys->SET_MASK) >> sys->SET_SHIFT;
   uint64_t lastTag = lastMiss & sys->TAG_MASK;
   int prefetched = 0;

   if(tag == lastTag && (lastSet+1) == set) {
      for(int i=0; i < prefetchNum; i++) {
         prefetched++;
         // Call memAccess to resolve the prefetch. The address is 
         // incremented in the set portion of its bits (least
         // significant bits not in the cache line offset portion)
         sys->memAccess(address + ((1 << sys->SET_SHIFT) * (i+1)), 
                           'R', tid, true);
      }
      
      lastPrefetch = address + (1 << sys->SET_SHIFT);
   }

   lastMiss = address;
   return prefetched;
}

int AdjPrefetch::prefetchHit(uint64_t address, unsigned int tid,
      System* sys)
{
   sys->memAccess(address + (1 << sys->SET_SHIFT), 'R', tid, true);
   return 1;
}

// Called to check for prefetches in the case of a cache hit.
int SeqPrefetch::prefetchHit(uint64_t address, unsigned int tid,
      System* sys)
{
   uint64_t set = (address & sys->SET_MASK) >> sys->SET_SHIFT;
   uint64_t tag = address & sys->TAG_MASK;
   uint64_t lastSet = (lastPrefetch & sys->SET_MASK) 
                                    >> sys->SET_SHIFT;
   uint64_t lastTag = lastPrefetch & sys->TAG_MASK;

   if(tag == lastTag && lastSet == set) {
      // Call memAccess to resolve the prefetch. The address is 
      // incremented in the set portion of its bits (least
      // significant bits not in the cache line offset portion)
      sys->memAccess(address + ((1 << sys->SET_SHIFT) * prefetchNum), 
                        'R', tid, true);
      lastPrefetch = lastPrefetch + (1 << sys->SET_SHIFT);
   }

   return 1;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
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

node *_hash[MAXH];
//FILE *fin,*fout;
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
    node *tmp = new node(now,n,_hash[t]);
    _hash[t] = tmp;
}

long long find(uint64_t now)
{
    int t = now%MAXH;
    node *tmp = _hash[t];
	node *pre = NULL;
    while (tmp) {
        if (tmp->addr==now) {
            long long tlabel = tmp->label;
            if (pre==NULL) _hash[t] = tmp->nxt;
            else pre->nxt = tmp->nxt;
            delete tmp;
            return tlabel;
        }
        pre = tmp;
        tmp = tmp->nxt;
    }
    return 0;
}

void solve(vector<unsigned long long>& trace, ofstream& outFile)
{
    memset(rtd,0,sizeof(rtd));
    n = 0;
    uint64_t addr;
	for(vector<unsigned long long>::iterator i=trace.begin(),end=trace.end();i!=end;++i){
		addr = (*i);
        n++;
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
        if (c%PGAP==0) outFile << std::setprecision(6) << 1.0*(N-sum)/N << endl;
    }

}

*/
/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,    "pintool",
    "o", "aet.out", "specify dcache file name");
KNOB<BOOL>   KnobTrackLoads(KNOB_MODE_WRITEONCE,    "pintool",
    "tl", "1", "track individual loads -- increases profiling time");
KNOB<BOOL>   KnobTrackStores(KNOB_MODE_WRITEONCE,   "pintool",
   "ts", "1", "track individual stores -- increases profiling time");

long long counter=0;	

/* ===================================================================== */
/* Cache Configure                                                  */
/* ===================================================================== */

unsigned int arr_map[] = {0};
vector<unsigned int> tid_map(arr_map, arr_map +
         sizeof(arr_map) / sizeof(unsigned int));
SeqPrefetch prefetch;
SingleCacheSystem sys(tid_map, 64, 4096, 8, &prefetch, false, false);

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This tool represents LLC MRC from memory access trace.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary() << endl; 
    return -1;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
VOID GetWrite(ADDRINT addr){
	if(addr!=0){
		sys.memAccess(addr, 'W', 0);
	}
}

VOID GetRead(ADDRINT addr){
	if(addr!=0){
		sys.memAccess(addr, 'R', 0);
	}
}
/*
//HAITANG---LOAD
VOID FootPrint(ADDRINT addr, UINT32 size)
{
  counter++;

  if(counter%100000==0) {
    cout<< "~~~~~~~~counter" << counter/100000 <<endl;
  }

  if (((addr+size-1)&0x7f)!=(addr&0x7f)){
    solve(addr>>6);
    solve((addr+size-1)>>6);
    
  } else{
    solve(addr>>6);
  }	
}
*/
/* ===================================================================== */

VOID Instruction(INS ins, void * v)
{
    if (INS_IsMemoryRead(ins) && INS_IsStandardMemop(ins))
    {
      if( KnobTrackLoads ) {
	INS_InsertPredicatedCall(
				 ins, IPOINT_BEFORE, (AFUNPTR) GetRead,
				 IARG_MEMORYREAD_EA,
				 IARG_END);
        }
   }

    if ( INS_IsMemoryWrite(ins) && INS_IsStandardMemop(ins))
    {
        if( KnobTrackStores )
	  {           
	    INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE,  (AFUNPTR) GetWrite,
                    IARG_MEMORYWRITE_EA,
                    IARG_END);
	    
                
	  }

    }
}
/* ===================================================================== */

VOID Fini(int code, VOID * v)
{
    //outFile << "PIN:FootPrint result!\n";

	//print_mrc(outFile);
	if(phase_count>=10000000) print_rth();
	printf("L3 access trace length: %lld\n",n);
    outFile.close();
}


/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
   // srand(time(NULL));

    PIN_InitSymbols();
 

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    //outFile.open(KnobOutputFile.Value().c_str());
	strcpy(benchname,KnobOutputFile.Value().c_str());	
	memset(rtd,0,sizeof(rtd));

    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns

    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
