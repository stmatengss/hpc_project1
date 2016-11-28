#include <sys/times.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <mach/mach_time.h>

#define FUNCNUM	7
#define ELEMENTS 200000000

typedef int data_t;
#define OP *
#define IDENT 1

typedef struct {
	long int len;
	data_t *data;
}vec_rec, *vec_ptr;

#if defined(__i386__)
static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile ("rdtsc" : "=A" (x));
    return x;
}
#elif defined(__x86_64__)
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo; 
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#endif

long nano_count()
{
    long time = mach_absolute_time();

    static long scaling_factor = 0;
    if(!scaling_factor)
    {
        mach_timebase_info_data_t info;
        kern_return_t ret = mach_timebase_info(&info);
        scaling_factor = info.numer/info.denom;
    }

    return time * scaling_factor;
}

// configuration
#define BLOCK_SIZE	(16*1024*1024)			// block size
#define CACHE_SIZE	(256*1024*1024)			// block size
/* end of configuration */

void CLEAR_L2_CACHE()
{
	int *p=malloc(CACHE_SIZE*sizeof(int));
	for ( int i=0; i<CACHE_SIZE; i++ )
	{
		p[i]=rand();
	}

	int x;
	for ( int i=0; i<CACHE_SIZE; i++ )
	{
		x+=p[i];
	}
}

// CONFIGURATION
#define N_ITER	1000000				// number of loop iterations
#define _MAX_CACHE_LINE_SIZE	64	//maximum possible size of the cache line

#define UN_FOX	(*(int*)((char*)fox + _MAX_CACHE_LINE_SIZE - sizeof(int)/2))
#define FOX	(*fox)					// determining aligned (FOX) and

int optimize(int *fox)
{
	int c = 0;

	for(FOX = 0; FOX < N_ITER; FOX+=1)
		c+=FOX;

	return c;
}


/*------------------------------------------------------------------------
 *
 *						PESSIMIZED VERSION
 *					(loop counter splits cache line)
 *
------------------------------------------------------------------------*/
int pessimize(int *fox)
{
	int c = 0;

	for(UN_FOX = 0; UN_FOX < N_ITER; UN_FOX+=1)
		c+=FOX;

	return c;
}


int main(int argc, char* argv[])
{
	int *fox;
	long val;
	long StartTime, ElapsedTime1, ElapsedTime2;

	fox = (int *)malloc(128*1024*1024);			// fox is guaranteed to be aligned 
								// by 32 bytes boundary

	StartTime=rdtsc();
	pessimize(fox);
	ElapsedTime1=rdtsc()-StartTime;

	CLEAR_L2_CACHE();

        StartTime=rdtsc();
	optimize(fox);
        ElapsedTime2=rdtsc()-StartTime;

	printf("original clocks=%ld optimized clocks=%ld\n",ElapsedTime1,ElapsedTime2);
	printf("speedup=%f%%\n",ElapsedTime1*100.0/ElapsedTime2);

	return 0;
}
