// lookup_table.cpp : Defines the entry point for the console application.
//

#include "csapp.h"

typedef unsigned long DWORD;

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

void RGBtoBW(DWORD *pBitmap, DWORD width, DWORD height, long stride )
{
	DWORD row, col;
	DWORD pixel, red, green, blue, alpha, bw;

	for ( row=0; row<height; row++ )
	{
		for ( col=0; col<width; col++ )
		{
			pixel = pBitmap[col+row*stride/4];
			alpha = (pixel>>24)&0xff;
			red   = (pixel>>16)&0xff;
			green = (pixel>>8)&0xff;
			blue  = pixel&0xff;
			bw = (DWORD)(red*0.299+green*0.587+blue*0.114);
			pBitmap[col+row*stride/4]=(alpha<<24)+(bw<<16)+(bw<<8)+(bw);
		}
	}
}

void RGBtoBW_1(DWORD *pBitmap, DWORD width, DWORD height, long stride )
{
	DWORD i, row, col;
	DWORD pixel, red, green, blue, alpha, bw;

	DWORD mul299[256];
	DWORD mul587[256];
	DWORD mul144[256];

	for ( i=0; i<256; i++ )
	{
		mul299[i]=(DWORD)(i*0.299);
		mul587[i]=(DWORD)(i*0.587);
		mul144[i]=(DWORD)(i*0.144);
	}

	for ( row=0; row<height; row++ )
	{
		for ( col=0; col<width; col++ )
		{
			pixel = pBitmap[col+row*stride/4];
			alpha = (pixel>>24)&0xff;
			red   = (pixel>>16)&0xff;
			green = (pixel>>8)&0xff;
			blue  = pixel&0xff;
			bw = mul299[red]+mul587[green]+mul144[blue];
			pBitmap[col+row*stride/4]=(alpha<<24)+(bw<<16)+(bw<<8)+(bw);
		}
	}
}

void RGBtoBW_2(DWORD *pBitmap, DWORD width, DWORD height, long stride )
{
	DWORD i, row, col;
	DWORD pixel, red, green, blue, alpha, bw;

	DWORD mul299[256];
	DWORD mul587[256];
	DWORD mul144[256];
	DWORD bmerge[256];

	for ( i=0; i<256; i++ )
	{
		mul299[i]=(DWORD)(i*0.299);
		mul587[i]=(DWORD)(i*0.587);
		mul144[i]=(DWORD)(i*0.144);
		bmerge[i]=(DWORD)((i<<16)+(i<<8)+i);
	}

	for ( row=0; row<height; row++ )
	{
		for ( col=0; col<width; col++ )
		{
			pixel = pBitmap[col+row*stride/4];
			alpha = (pixel>>24)&0xff;
			red   = (pixel>>16)&0xff;
			green = (pixel>>8)&0xff;
			blue  = pixel&0xff;
			bw = mul299[red]+mul587[green]+mul144[blue];
			pBitmap[col+row*stride/4]=(alpha<<24)+bmerge[bw];
		}
	}
}

int main(int argc, char* argv[])
{
	DWORD bitmap[352][288];
	long i,j;
	long StartTime,ElapsedTime1,ElapsedTime2,ElapsedTime3;

	for ( i=0; i<352; i++ )
		for ( j=0; j<288; j++ )
			bitmap[i][j]=i*j;

	StartTime=rdtsc();
	RGBtoBW((DWORD *)bitmap,352,288,16);
	ElapsedTime1=rdtsc()-StartTime;


	StartTime=rdtsc();
	RGBtoBW_1((DWORD *)bitmap,352,288,16);
	ElapsedTime2=rdtsc()-StartTime;
	printf("total speedrate with 3 tables:%lf%%\n",ElapsedTime1*100.00/ElapsedTime2);

	StartTime=rdtsc();
	RGBtoBW_2((DWORD *)bitmap,352,288,16);
	ElapsedTime3=rdtsc()-StartTime;

	printf("total speedrate with 4 tables:%lf%%\n",ElapsedTime1*100.00/ElapsedTime3);

	return 0;
}


