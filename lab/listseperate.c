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

double nano_count()
{
    long time = mach_absolute_time();

    static long scaling_factor = 0;
    if(!scaling_factor)
    {
        mach_timebase_info_data_t info;
        kern_return_t ret = mach_timebase_info(&info);
        scaling_factor = info.numer/info.denom;
    }

    return time * scaling_factor / 1000000.0 ;
}

// configuration
#define BLOCK_SIZE	(32*1024*1024)			// block size
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

#define N_ELEM	40000	// number of list elements

struct list{			// CLASSIC LIST
	struct list	*next;	// pointer to the next node
	int			val;	// value
	char		content[256];
};

struct mylist{			// OPTIMIZED SEPARATED LIST
	int *next;	// array of pointers to the next node
	int *val;			// array of values
	char		content[256];
};

int main()
{
	int a;
	int b = 0;
	struct list *classic_list,*tmp_list;
	struct mylist separated_list;
	double starttime,endtime;

	// allocating memory
	classic_list = (struct list*) malloc(N_ELEM * sizeof(struct list));

	// list initialization
	for (a = 0; a < N_ELEM; a++)
	{
		classic_list[a].next= classic_list + a+1;
		classic_list[a].val = a;
	} classic_list[N_ELEM-1].next = 0;
	
	// tracing the list

	CLEAR_L2_CACHE();
	starttime=nano_count();
			tmp_list=classic_list;
			while(tmp_list = tmp_list[0].next);
	endtime=nano_count();
	printf("time=%.03lf\n",endtime-starttime);


	/* ----------------------------------------------------------------------
	 *
	 *			processing optimized separated list
	 *
	----------------------------------------------------------------------- */
	// allocating memory
	separated_list.next = (int *) malloc(N_ELEM*sizeof(int));
	separated_list.val  = (int *)       malloc(N_ELEM*sizeof(int));
	
	// list initialization
	for (a=0;a<N_ELEM;a++)
	{
		separated_list.next[a] = a+1;
		/*                 ^^^ pay attention to the position of
		                       square brackets */
		separated_list.val[a] = a;
	} separated_list.next[N_ELEM-1] = 0;

	// tracing the list
	CLEAR_L2_CACHE();
	starttime=nano_count();
			while(b=separated_list.next[b]);
	endtime=nano_count();
	printf("time=%.03lf\n",endtime-starttime);

}

