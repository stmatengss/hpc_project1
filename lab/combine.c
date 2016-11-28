#include <sys/times.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
//#include <mach/mach_time.h>

#define FUNCNUM	7
#define ELEMENTS 200000000

typedef float data_t;
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

/*
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
*/

vec_ptr new_vec(long int len)
{ 
    /* Allocate header structure */
    vec_ptr result = (vec_ptr) malloc(sizeof(vec_rec));
    if (!result)
      return NULL; /* Couldn’t allocate storage */ 
    result->len = len;
    /* Allocate array */
    if(len>0){
        data_t *data = (data_t *)calloc(len, sizeof(data_t));
        if (!data) {
            free((void *) result);
            return NULL; /* Couldn’t allocate storage */
        }
        result->data = data;
    }
    else
        result->data = NULL;
    return result;
}

void init_vec( vec_ptr v )
{
    long i;
    for ( i=0; i<v->len; i++)
    {
	v->data[i]=rand();
    }
}

void free_vec( vec_ptr v )
{
    free(v->data);
    free(v);
}

/* retrieve vector element and store at val */
int get_vec_element(vec_ptr v, long int idx, data_t *val)
{
        if (idx < 0 || idx >= v->len)
                return 0;
        *val = v->data[idx];
        return 1;
}

long vec_length( vec_ptr v, long factor )
{
    return v->len/factor;
}

int loop1(int *a, int x, int n, int factor ) 
{ 
  int y = x*x;
  int i;
  for (i = 0; i < n/factor; i++) 
    x = y * a[i];
  return x*y;
} 

int loop2(int *a, int x, int n, int factor ) 
{ 
  int y = x*x;
  int i;
  for (i = 0; i < n/factor; i++) 
  x = x * a[i]; 
  return x*y; 
} 

long fact1(long n, int factor) 
{
    long i;
    long result = 1;
    n=n/factor;
    for (i = n; i > 0; i--) 
	result = result * i;
    return result;
}

long fact2(long n, int factor) 
{
    long i;
    long result = 1;
    n=n/factor;
    for (i = n; i > 0; i-=2) 
    { 
	result = (result * i) * (i-1);
    }
  return result;
}

void combine1(vec_ptr v, data_t *dest, long factor )
{
    long int i;

    *dest = IDENT;
    for (i = 0; i < vec_length(v,factor); i++) {
        data_t val;
        get_vec_element(v, i, &val);
        *dest = *dest OP val;
    }
}

void combine2(vec_ptr v, data_t *dest, long factor)
{
    long int i;
    long int length = vec_length(v,factor);
    *dest = IDENT;
    for (i = 0; i < length; i++) {
       data_t val;
       get_vec_element(v, i, &val);
       *dest = *dest OP val;
    }
}

data_t *get_vec_start(vec_ptr v) 
{
    return v->data;
}

void combine3(vec_ptr v, data_t *dest, long factor)
{ 
    long int i;
    long int length = vec_length(v,factor);
    data_t *data = get_vec_start(v);
    *dest = IDENT; 
    for(i=0;i<length;i++){
    	*dest = *dest OP data[i];
    }
}

void combine4(vec_ptr v, data_t *dest, long factor)
{
    long int i;
    long int length = vec_length(v,factor);
    data_t *data = get_vec_start(v);
    data_t acc = IDENT;

    for (i = 0; i < length; i++) {
      acc = acc OP data[i];
    }
    *dest = acc;
}

void combine4b(vec_ptr v, data_t *dest, long factor)
{
    long int i;
    long int length = vec_length(v,factor);
    data_t acc = IDENT;
    for (i = 0; i < length; i++) {
	if (i >= 0 && i < v->len) {
	    acc = acc OP v->data[i];
	}
    }
    *dest = acc;
}

void combine5(vec_ptr v, data_t *dest, long factor)
{
    int length = vec_length(v,factor);
    int limit = length-1;
    data_t *d = get_vec_start(v);
    data_t x = IDENT;
    int i;
    /* Combine 2 elements at a time */
    for (i = 0; i < limit; i+=2) {
	x =(x OP d[i]) OP d[i+1];
    }
    /* Finish any remaining elements */
    for (; i < length; i++) {
	x = x OP d[i];
    }
    *dest = x;
}

void combine6(vec_ptr v, data_t *dest, long factor)
{
    long int i;
    long int length = vec_length(v,factor);
    long int limit = length-1;
    data_t *data = get_vec_start(v);
    data_t acc0 = IDENT;
    data_t acc1 = IDENT;

    /* Combine 2 elements at a time */
    for(i=0;i<limit;i+=2){
        acc0 = acc0 OP data[i];
        acc1 = acc1 OP data[i+1];
    }

    /* Finish any remaining elements */
    for (; i < length; i++) {
      acc0 = acc0 OP data[i];
    }
    *dest = acc0 OP acc1;
}

/* Change associativity of combining operation */
void combine7(vec_ptr v, data_t *dest, long factor)
{
 long int i;
 long int length = vec_length(v,factor);
 long int limit = length-1;
 data_t *data = get_vec_start(v);
 data_t acc = IDENT;

 /* Combine 2 elements at a time */
 for(i=0;i<limit;i+=2){
 	acc = acc OP (data[i] OP data[i+1]);
 }

 /* Finish any remaining elements */
 for (; i < length; i++) {
 acc = acc OP data[i];
 }
 *dest = acc;
}

void psum1(float a[], float p[], long int n)
{
	long i;
	p[0] = a[0];
	for (i = 1; i < n; i++)
	p[i] = p[i-1] + a[i];
}

void psum2(float a[], float p[], long n)
{
        long i;
        p[0] = a[0]; 
	for(i=1;i<n-1;i+=2){
        float mid_val = p[i-1] + a[i];
        p[i]    = mid_val;
        p[i+1]  = mid_val + a[i+1];
	}
	/* For odd n, finish remaining element */
	if(i<n)
	p[i] = p[i-1] + a[i];
}

static void pr_times(clock_t real, struct tms *tmsstart, struct tms *tmsend){
        static long clktck=0;
        if(0 == clktck)
                if((clktck=sysconf(_SC_CLK_TCK))<0)
                           puts("sysconf err");
        printf("real:%7.3f\n", real/(double)clktck);
        printf("user-cpu:%7.3f\n", (tmsend->tms_utime - tmsstart->tms_utime)/(double)clktck);
        printf("system-cpu:%7.3f\n", (tmsend->tms_stime - tmsstart->tms_stime)/(double)clktck);
        printf("child-user-cpu:%7.3f\n", (tmsend->tms_cutime - tmsstart->tms_cutime)/(double)clktck);
        printf("child-system-cpu:%7.3f\n", (tmsend->tms_cstime - tmsstart->tms_cstime)/(double)clktck);
}

int main()
{
   long i,m;

   vec_ptr v;
   data_t dest;
   v=new_vec(ELEMENTS);

   time_t t;
   srand(time(&t));

   unsigned long starttick,endtick,timedur[10];
   float CPE;

   int *a;
   a=(int *)malloc(sizeof(int)*ELEMENTS);
   for ( m=0; m<ELEMENTS; m++ )
   {
	a[m]=rand();
   }

   void (*combinefuncptr[10])(vec_ptr v, data_t *dest, long factor);
   long (*factfuncptr[10])(long n, int factor);
 
   combinefuncptr[0]=combine1;
   combinefuncptr[1]=combine2;
   combinefuncptr[2]=combine3;
   combinefuncptr[3]=combine4;
   combinefuncptr[4]=combine5;
   combinefuncptr[5]=combine6;
   combinefuncptr[6]=combine7;

   factfuncptr[0]=fact1;
   factfuncptr[1]=fact2;
   

   for ( m=0; m<FUNCNUM; m++ )
   {
      for ( i=1; i<=10; i++ )
      {
         starttick=rdtsc();

         combinefuncptr[m](v,&dest,i);
      
         endtick=rdtsc();
         timedur[i-1] = endtick-starttick;
         printf("%ld clock cycles.\n",timedur[i-1]);
      }

      CPE=0;
      
      for ( i=1; i<=9; i++ )
      {
	 if (timedur[i-1]>timedur[i])
            CPE+=(((timedur[i-1]-timedur[i])*1.0)/ELEMENTS)*(i*i+1)/9; 
      }
      printf("combine%ld: CPE=%.02f\n",m+1,CPE);
   }

   free(v);

   return 1;
}

