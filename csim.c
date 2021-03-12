#include <getopt.h>  // getopt, optarg
#include <stdlib.h>  // exit, atoi, malloc, free
#include <stdio.h>   // printf, fprintf, stderr, fopen, fclose, FILE
#include <limits.h>  // ULONG_MAX
#include <string.h>  // strcmp, strerror
#include <errno.h>   // errno

/* fast base-2 integer logarithm */
#define INT_LOG2(x) (31 - __builtin_clz(x))
#define NOT_POWER2(x) (__builtin_clz(x) + __builtin_ctz(x) != 31)

/* tag_bits = ADDRESS_LENGTH - set_bits - block_bits */
#define ADDRESS_LENGTH 64

/**
 * Print program usage (no need to modify).
 */
static void print_usage() {
    printf("Usage: csim [-hv] -S <num> -K <num> -B <num> -p <policy> -t <file>\n");
    printf("Options:\n");
    printf("  -h           Print this help message.\n");
    printf("  -v           Optional verbose flag.\n");
    printf("  -S <num>     Number of sets.           (must be > 0)\n");
    printf("  -K <num>     Number of lines per set.  (must be > 0)\n");
    printf("  -B <num>     Number of bytes per line. (must be > 0)\n");
    printf("  -p <policy>  Eviction policy. (one of 'FIFO', 'LRU')\n");
    printf("  -t <file>    Trace file.\n\n");
    printf("Examples:\n");
    printf("  $ ./csim    -S 16  -K 1 -B 16 -p LRU -t traces/yi.trace\n");
    printf("  $ ./csim -v -S 256 -K 2 -B 16 -p LRU -t traces/yi.trace\n");
}

/* Parameters set by command-line args (no need to modify) */
int verbose = 0;   // print trace if 1
int S = 0;         // number of sets
int K = 0;         // lines per set
int B = 0;         // bytes per line

typedef enum { FIFO = 1, LRU = 2 } Policy;
Policy policy;     // 0 (undefined) by default

FILE *trace_fp = NULL;

/**
 * Parse input arguments and set verbose, S, K, B, policy, trace_fp.
 *
 * TODO: Finish implementation
 */
static void parse_arguments(int argc, char **argv) {
    char c;
    while ((c = getopt(argc, argv, "S:K:B:p:t:vh")) != -1) {
        switch(c) {
            case 'S':
                S = atoi(optarg);
                //fprintf(stderr, "Error: %d\n", S);
                if (NOT_POWER2(S)) {
                    fprintf(stderr, "ERROR: S must be a power of 2\n");
                    exit(1);
                }
                break;
            case 'K':
                // TODO
                K = atoi(optarg);
                break;
            case 'B':
                // TODO
                B = atoi(optarg);
                if (NOT_POWER2(B)) {
                    fprintf(stderr, "ERROR: B must be a power of 2\n");
                    exit(1);
                }
                break;
            case 'p':
                if (!strcmp(optarg, "FIFO")) {
                    policy = FIFO;
                }
                // TODO: parse LRU, exit with error for unknown policy
                else if (!strcmp(optarg, "LRU")){
                    policy = LRU;
                }
                else{
                    fprintf(stderr, "ERROR: no matching policy\n");
                    exit(1); 
                }
                break;
            case 't':
                // TODO: open file trace_fp for reading
                trace_fp = fopen(optarg,"r"); 
                if (!trace_fp) {
                    fprintf(stderr, "ERROR: %s: %s\n", optarg, strerror(errno));
                    exit(1);
                }
                break;
            case 'v':
                // TODO
                verbose = 1;

                break;
            case 'h':
                // TODO
                print_usage(argv);
                exit(0);
            default:
                print_usage();
                exit(1);
        }
    }

    /* Make sure that all required command line args were specified and valid */
    if (S <= 0 || K <= 0 || B <= 0 || policy == 0 || !trace_fp) {
        printf("ERROR: Negative or missing command line arguments\n");
        print_usage();
        if (trace_fp) {
            fclose(trace_fp);
        }
        exit(1);
    }

    /* Other setup if needed */

}

/**
 * Cache data structures
 * TODO: Define your own!
 */
struct Line{
    int valid;
    long unsigned tag;
    int order;
    int time;
};


/**
 * Allocate cache data structures.
 *
 * This function dynamically allocates (with malloc) data structures for each of
 * the `S` sets and `K` lines per set.
 *
 * TODO: Implement
 */
struct Line* cache;
static void allocate_cache() {
    cache = (struct Line*)malloc(S*K*sizeof(struct Line));
    for(int i=0; i<S*K; i++){
        cache[i].valid = 0;
        cache[i].order =INT_MAX;
        cache[i].time =INT_MAX;
        cache[i].tag = 0;
    }
}

/**
 * Deallocate cache data structures.
 *
 * This function deallocates (with free) the cache data structures of each
 * set and line.
 *
 * TODO: Implement
 */
static void free_cache() {
    free(cache);
}

/* Counters used to record cache statistics */
int miss_count     = 0;
int hit_count      = 0;
int eviction_count = 0;
//int count_FIFO = 0;
int time =0;

/**
 * Simulate a memory access.
 *
 * If the line is already in the cache, increase `hit_count`; otherwise,
 * increase `miss_count`; increase `eviction_count` if another line must be
 * evicted. This function also updates the metadata used to implement eviction
 * policies (LRU, FIFO).
 *
 * TODO: Implement
 */
int time_stamp=0; //global variable timesstamp
static void access_data(unsigned long addr) {
    /*//update used time
    for(int i=0; i<S*K;i++){
        if(cache[i].valid){
            cache[i].time++;
        }
    }*/
    time_stamp++;
    //count_FIFO++;
    
    //printf("Access to %016lx\n", addr);
    int b = INT_LOG2(B);
    int s = INT_LOG2(S);
    long unsigned tag = addr>>(b+s);
    long unsigned set = (addr>>b) & (unsigned long)(S-1);
    //int found = 0;
    //printf("offset bit : %d\n",b);
   // printf("set bit : %d\n",s);
    //printf("address : %lu\n",addr);
    /*printf("Tag : %lu\n",tag);
    printf("Set : %ld\n",set);
    printf("set*K: %ld\n", set*K);
    printf("set*K+K: %ld\n\n", set*K+K);
    */
    for(long unsigned i=set*K; i<set*K+K;i++){
        //True when valid, tag match
        //printf("index i : %d valid bit : %d\n",i,cache[i].valid);
        if (cache[i].valid){
            //printf("Entered %d\n",count_FIFO);
            if(cache[i].tag == tag){
                //printf("Entered hit\n");
                hit_count++;
                
                //set access order
                //cache[i].order = count_FIFO;

                //set access time back to 0
                //cache[i].time =0;
                //Don't enter the miss case
                cache[i].time = time_stamp;
                //found = 1;
                
                return; 
            }
        }       
    }
    
    int full = 1;
    int index =0;
    int min=0;
    
    //int max =0;

    //if not hit, miss
    //if(!found){
        //printf("entered miss\n");
        miss_count++;
        for(long unsigned i=set*K;i<set*K+K;i++){
            if(!cache[i].valid){
                cache[i].tag = tag;
                cache[i].valid = 1;
                //set access order
                cache[i].order = time_stamp;
                //set time back to 0
                //cache[i].time=0;
                cache[i].time = time_stamp;
                full=0;
                return;
            }
        }
        if(full){
            //printf("entered eviction\n");
            if(policy==FIFO){
                min = cache[set*K].order;
                index=set*K;
                for(long unsigned i=set*K+1;i<set*K+K;i++){  
                    if(cache[i].order<min){
                        min = cache[i].order;
                        index=i;
                    }
                }
                cache[index].tag = tag;
                cache[index].valid = 1;
                //set access order
                cache[index].order = time_stamp;
                //set access time back to 0
                //cache[index].time = 0;
                cache[index].time = time_stamp;
                eviction_count++;
            }
            else if(policy==LRU){
                /*max = cache[set*K].time;
                index=set*K;
                for(int i=set*K+1;i<set*K+K;i++){  
                    if(cache[i].time>max){
                        max = cache[i].order;
                        index=i;
                    }
                }*/
                min = cache[set*K].time;
                index = set*K;
                for(long unsigned i=set*K+1;i<set*K+K;i++){  
                    if(cache[i].time<min){
                        min = cache[i].time;
                        index=i;
                    }
                }
                cache[index].tag = tag;
                cache[index].valid = 1;
                //set access order
                //cache[index].order = time_stamp;
                //set access time back to 0
                //cache[index].time = 0;
                cache[index].time=time_stamp;
                eviction_count++;
            }
        }
    //}
    
    //printf("count = %d\n",count_FIFO);
}

/**
 * Replay the input trace.
 *
 * This function:
 * - reads lines (e.g., using fgets) from the file handle `trace_fp` (a global variable)
 * - skips lines not starting with ` S`, ` L` or ` M`
 * - parses the memory address (unsigned long, in hex) and len (unsigned int, in decimal)
 *   from each input line
 * - calls `access_data(address)` for each access to a cache line
 *
 * TODO: Implement
 */
static void replay_trace() {
   // access_data(0);
    char operation;
    unsigned long address;
    int size;
    char string[200];
   // unsigned long end_Address = address+ size;
    
    while(fgets(string, 200, trace_fp)!=NULL){
        sscanf(string, " %c %lx,%d", &operation, &address, &size);
        /*printf("\nString : %s",string);
        printf("Operation : %c\n",operation);
        printf("Address : %lu\n",address);
        printf("Size : %d\n",size);*/
        unsigned long end_Address = address + (unsigned long)size;


        switch(operation){
            case 'S': 
                for(unsigned long i=address-(address%B); i<end_Address;i=i+B){
                    access_data(i);
                }
                //access_data(address);
                break;
            case 'L':
                for(unsigned long i=address-(address%B); i<end_Address;i=i+B){
                    access_data(i);
                }
                //access_data(address);                   
                break;
            case 'M':
                for(unsigned long i=address-(address%B); i<end_Address;i=i+B){
                    access_data(i);
                    access_data(i);
                }
                //access_data(address);
                //access_data(address);
                break;
            }
        //printf("\n");
    }
    fclose(trace_fp);

}

//static void printv(){}

/**
 * Print cache statistics (DO NOT MODIFY).
 */
static void print_summary(int hits, int misses, int evictions) {
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
}

int main(int argc, char **argv) {
    //printf("first");
    parse_arguments(argc, argv);  // set global variables used by simulation
    //printf("before allocate");
    allocate_cache();             // allocate data structures of cache
    //printf("before replaytrace");
    replay_trace();               // simulate the trace and update counts
    free_cache();                 // deallocate data structures of cache
    //fclose(trace_fp);             // close trace file
    //printv();
    print_summary(hit_count, miss_count, eviction_count);  // print counts
    //printf("%d",10);
    return 0;
}
