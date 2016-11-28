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

struct MYLIST{								// list element
		struct MYLIST *next;
		int	val;
};

#define N_ELEM		(BLOCK_SIZE/sizeof(struct MYLIST))

int main()
{
	int a;
	struct MYLIST *p, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;

	// allocating memory
	struct MYLIST *one_list   = (struct MYLIST*) malloc(BLOCK_SIZE);
	struct MYLIST *spl_list_1 = (struct MYLIST*) malloc(BLOCK_SIZE/2);
	struct MYLIST *spl_list_2 = (struct MYLIST*) malloc(BLOCK_SIZE/2);
	struct MYLIST *spl_list_3 = (struct MYLIST*) malloc(BLOCK_SIZE/4);
	struct MYLIST *spl_list_4 = (struct MYLIST*) malloc(BLOCK_SIZE/4);
	struct MYLIST *spl_list_5 = (struct MYLIST*) malloc(BLOCK_SIZE/6);
	struct MYLIST *spl_list_6 = (struct MYLIST*) malloc(BLOCK_SIZE/6);
	struct MYLIST *spl_list_7 = (struct MYLIST*) malloc(BLOCK_SIZE/8);
	struct MYLIST *spl_list_8 = (struct MYLIST*) malloc(BLOCK_SIZE/8);

	// initialization
	for (a = 0; a < N_ELEM; a++)
	{
		one_list[a].next = one_list + a + 1;
		one_list[a].val  = a;
	} one_list[N_ELEM-1].next = 0;
	// tracing

	double starttime, endtime;

	starttime=nano_count();
	p = one_list;
	while(p = p[0].next);
	endtime=nano_count();

	printf("one time=%.03lf\n",endtime-starttime);


	/* -----------------------------------------------------------------------
	 *
	 *					processing two splitted lists
	 *
	----------------------------------------------------------------------- */
	CLEAR_L2_CACHE(); 
	// initialization
	for (a = 0; a < N_ELEM/2; a++)
	{
		spl_list_1[a].next = spl_list_1 + a + 1;
		spl_list_1[a].val  = a;

		spl_list_2[a].next = spl_list_2 + a + 1;
		spl_list_2[a].val  = a;

	} spl_list_1[N_ELEM/2-1].next = 0;
	  spl_list_2[N_ELEM/2-1].next = 0;

	// tracing
	starttime=nano_count();
	p1 = spl_list_1; p2 = spl_list_2;
	while((p1 = p1[0].next) && (p2 = p2[0].next));
	// attention! this tracing method supposes that both lists
	// have equal number of elements, otherwise it will be necessary
	// to correct the code, for example, as follows:
	// while(p1 || p2)
	// {
	//		if (p1) p1 = p1[0].next;
	//		if (p2) p2 = p2[0].next;
	// }
	// however this will make the code less illustrative, therfore 
	// the first variant is used
	endtime=nano_count();
	printf("two time=%.03lf\n",endtime-starttime);


	/* -----------------------------------------------------------------------
	 *
	 *					processing four splitted lists
	 *
	----------------------------------------------------------------------- */
	CLEAR_L2_CACHE(); 

	// initialization
	for (a = 0; a < N_ELEM/4; a++)
	{
		spl_list_1[a].next = spl_list_1 + a + 1;
		spl_list_1[a].val  = a;

		spl_list_2[a].next = spl_list_2 + a + 1;
		spl_list_2[a].val  = a;

		spl_list_3[a].next = spl_list_3 + a + 1;
		spl_list_3[a].val  = a;

		spl_list_4[a].next = spl_list_4 + a + 1;
		spl_list_4[a].val  = a;
	} spl_list_1[N_ELEM/4-1].next = 0; spl_list_2[N_ELEM/4-1].next = 0;
	  spl_list_3[N_ELEM/4-1].next = 0; spl_list_4[N_ELEM/4-1].next = 0;

	// tracing
	starttime=nano_count();
	p1 = spl_list_1; p2 = spl_list_2; p3 = spl_list_3; p4 = spl_list_4;
	while( (p1 = p1[0].next) && (p2 = p2[0].next)
		&& (p3 = p3[0].next) && (p4 = p4[0].next));
	endtime=nano_count();
	printf("four time=%.03lf\n",endtime-starttime);


	/* -----------------------------------------------------------------------
	 *
	 *					processing siz splitted lists
	 *
	----------------------------------------------------------------------- */
	CLEAR_L2_CACHE(); 
	// initialization
	for (a=0;a < N_ELEM/6;a++)
	{
		spl_list_1[a].next = spl_list_1 + a + 1;
		spl_list_1[a].val  = a;

		spl_list_2[a].next = spl_list_2 + a + 1;
		spl_list_2[a].val  = a;

		spl_list_3[a].next = spl_list_3 + a + 1;
		spl_list_3[a].val  = a;

		spl_list_4[a].next = spl_list_4 + a + 1;
		spl_list_4[a].val  = a;

		spl_list_5[a].next = spl_list_5 + a + 1;
		spl_list_5[a].val  = a;

		spl_list_6[a].next = spl_list_6 + a + 1;
		spl_list_6[a].val  = a;
	} spl_list_1[N_ELEM/6-1].next = 0; spl_list_2[N_ELEM/6-1].next = 0;
	  spl_list_3[N_ELEM/6-1].next = 0; spl_list_4[N_ELEM/6-1].next = 0;
	  spl_list_5[N_ELEM/6-1].next = 0; spl_list_6[N_ELEM/6-1].next = 0;

	// tracing
	starttime=nano_count();
		p1 = spl_list_1; p2 = spl_list_2; p3 = spl_list_3; p4 = spl_list_4;
		p5 = spl_list_5; p6 = spl_list_6;
		while( (p1 = p1[0].next) && (p2 = p2[0].next)
			&& (p3 = p3[0].next) && (p4 = p4[0].next)
			&& (p5 = p5[0].next) && (p6 = p6[0].next));
	endtime=nano_count();
	printf("six time=%.03lf\n",endtime-starttime);


	/* -----------------------------------------------------------------------
	 *
	 *					processing eight splitted lists
	 *
	----------------------------------------------------------------------- */
	CLEAR_L2_CACHE();
	// initialization
	for (a=0;a < N_ELEM/8;a++)
	{
		spl_list_1[a].next = spl_list_1 + a + 1;
		spl_list_1[a].val  = a;

		spl_list_2[a].next = spl_list_2 + a + 1;
		spl_list_2[a].val  = a;

		spl_list_3[a].next = spl_list_3 + a + 1;
		spl_list_3[a].val  = a;

		spl_list_4[a].next = spl_list_4 + a + 1;
		spl_list_4[a].val  = a;

		spl_list_5[a].next = spl_list_5 + a + 1;
		spl_list_5[a].val  = a;

		spl_list_6[a].next = spl_list_6 + a + 1;
		spl_list_6[a].val  = a;

		spl_list_7[a].next = spl_list_7 + a + 1;
		spl_list_7[a].val  = a;

		spl_list_8[a].next = spl_list_8 + a + 1;
		spl_list_8[a].val  = a;

	} spl_list_1[N_ELEM/8-1].next = 0; spl_list_2[N_ELEM/8-1].next = 0;
	  spl_list_3[N_ELEM/8-1].next = 0; spl_list_4[N_ELEM/8-1].next = 0;
	  spl_list_5[N_ELEM/8-1].next = 0; spl_list_6[N_ELEM/8-1].next = 0;
	  spl_list_7[N_ELEM/8-1].next = 0; spl_list_8[N_ELEM/8-1].next = 0;

	// tracing
	starttime=nano_count();
		p1 = spl_list_1; p2 = spl_list_2;p3 = spl_list_3; p4 = spl_list_4;
		p5 = spl_list_5; p6 = spl_list_6;p5 = spl_list_5; p6 = spl_list_6;
		p7 = spl_list_7; p8 = spl_list_8;
		while( (p1 = p1[0].next) && (p2 = p2[0].next)
			&& (p3 = p3[0].next) && (p4 = p4[0].next)
			&& (p5 = p5[0].next) && (p6 = p6[0].next)
			&& (p7 = p7[0].next) && (p8 = p8[0].next));
	endtime=nano_count();
	printf("eight time=%.03lf\n",endtime-starttime);

	return 0;
}



