#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <sys/time.h>
#include <iostream>

using namespace std;

#define TIME(a,b) (1.0*((b).tv_sec-(a).tv_sec)+0.000001*((b).tv_usec-(a).tv_usec))
#define IDX(i,j,k) ((i)*ny*nz+(j)*nz+(k))
#define FORNI(n) for(int i=0;i<n;i++) 
#define FORNNI(n) for(int i=1;i<n;i++)
#define FORNJ(n) for(int j=0;j<n;j++) 
#define FORNNJ(n) for(int j=1;j<n;j++)

//#define TEST_WAIT

#ifdef __cplusplus
extern "C" {               
#endif

	extern int Init(double *data, long long L);
	extern int Check(double *data, long long L);
#ifdef __cplusplus
}
#endif
//Can be modified
int nprocs, myrank;

typedef struct
{
	int nx, ny, nz;
} Info;

template<typename T>
void debug(string title, T a)
{
	cout << title << ":" << a <<endl;
}

//Must be modified because only single process is used now
Info setup(int NX, int NY, int NZ, int P)
{
	Info result;
	int myrank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	result.nx = NX;
	result.ny = NY;
	result.nz = NZ;
	//if(myrank == 0)
	//{
	//	result.nx = NX;
	//	result.ny = NY;
	//	result.nz = NZ;
	//}
	//else
	//{
	//	result.nx = 0;
	//	result.ny = 0;
	//	result.nz = 0;
	//}
	return result;
}

void gather(double *A, int nx, int ny, int nz, int nx_dis, int slice_s) {
	int slice_v = slice_s * nx_dis;
	if (myrank == 0) {
		for(int i = 1; i < nprocs; i++) {
			
		//	if (myrank == nprocs-1) {
		//		slice_v = min(nx * ny * nz - IDX(i * myrank, 0, 0), slice_v);
		//	}
			MPI_Recv(A + IDX(i * nx_dis, 0, 0), slice_v, 
				MPI_DOUBLE, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);			
		}
	} else {
		MPI_Request request;
		MPI_Isend(A + IDX(myrank * nx_dis, 0, 0), slice_v,
				MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &request);
	}
}
//Must be re-written, including all the parameters
int stencil(double *A, double *B, int nx, int ny, int nz, int steps) {
	int i, j, k, s;
	//MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	//MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	int nx_dis = nx / nprocs;
	int slice_s = ny * nz;
	int iter_begin = myrank * nx_dis;
	int iter_end = (myrank + 1) * nx_dis;
	MPI_Status status;
	MPI_Request request[4];
	if (myrank == nprocs - 1) {
		iter_end = nx;
	} 
	//int my_rank;
	for (s = 0; s < steps; s ++) {
	//	debug("round", s);
		if (myrank != 0) {
			MPI_Isend(A + IDX(iter_begin, 0, 0), slice_s, 
				MPI_DOUBLE, myrank-1, 1, MPI_COMM_WORLD, &request[0] );
		}
	//	debug("sendA", myrank);
		if (myrank != nprocs-1) {
			MPI_Isend(A + IDX(iter_end-1, 0, 0), slice_s,
				MPI_DOUBLE, myrank+1, 1, MPI_COMM_WORLD, &request[1] );
		
		}
	//	debug("sendB", myrank);
		if (myrank != 0) {
#ifndef TEST_WAIT
			MPI_Recv(A + IDX(iter_begin-1, 0, 0), slice_s,
				MPI_DOUBLE, myrank-1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
#endif
#ifdef TEST_WAIT		
			MPI_Irecv(A + IDX(iter_begin-1, 0, 0), slice_s,
				MPI_DOUBLE, myrank-1, 1, MPI_COMM_WORLD, &request[2] );
#endif
		}
	//	debug("recvA", myrank);
		if (myrank != nprocs-1) {
#ifndef TEST_WAIT
			MPI_Recv(A + IDX(iter_end, 0, 0), slice_s,
				MPI_DOUBLE, myrank+1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE );	
#endif
#ifdef TEST_WAIT
			MPI_Irecv(A + IDX(iter_end, 0, 0), slice_s,
				MPI_DOUBLE, myrank+1, 1, MPI_COMM_WORLD, &request[3] );	
#endif
		}
#ifdef TEST_WAIT
		if (myrank == 0) {
			MPI_Waitall(2, request+2, MPI_STATUS_IGNORE);
		} else if (myrank == nprocs-1) {
			MPI_Waitall(2, request, MPI_STATUS_IGNORE);
		} else {
			MPI_Waitall(4, request, MPI_STATUS_IGNORE);
		}
#endif
	//	debug("recvB", myrank);
	//	debug("A:", A[IDX(iter_begin, 0, 0)]);
	//	debug("B:", B[IDX(iter_begin, 0, 0)]);
#pragma omp parallel for schedule (dynamic)
		for(i = iter_begin; i < iter_end; i ++) {
			for(j = 0; j < ny; j ++) {
				for(k = 0; k < nz; k ++) {
					double r = 0.4*A[IDX(i,j,k)];
					if(k !=  0)   r += 0.1*A[IDX(i,j,k-1)];
					else          r += 0.1*A[IDX(i,j,k)];
					if(k != nz-1) r += 0.1*A[IDX(i,j,k+1)];
					else          r += 0.1*A[IDX(i,j,k)];
					if(j !=  0)   r += 0.1*A[IDX(i,j-1,k)];
					else          r += 0.1*A[IDX(i,j,k)];
					if(j != ny-1) r += 0.1*A[IDX(i,j+1,k)];
					else          r += 0.1*A[IDX(i,j,k)];
					if(i !=  0)   r += 0.1*A[IDX(i-1,j,k)];
					else          r += 0.1*A[IDX(i,j,k)];
					if(i != nx-1) r += 0.1*A[IDX(i+1,j,k)];
					else          r += 0.1*A[IDX(i,j,k)];
					B[IDX(i,j,k)] = r;
				}
			}
		}	
		if (s == steps - 1) {
			gather(A, nx, ny, nz, nx_dis, slice_s);
		}
		double *tmp = NULL;
		tmp = A, A = B, B = tmp;
	}
	return 0;
}



int main(int argc, char **argv) {
	double *A = NULL, *B = NULL;
	int nx, ny, nz;
	//int myrank;
	int NX=100, NY=100, NZ=100, STEPS=10;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	NX = atoi(argv[1]), NY = atoi(argv[2]), NZ = atoi(argv[3]);
	STEPS = atoi(argv[4]);
	if(myrank == 0)
		printf("Size:%dx%dx%d, # of Steps: %d, # of procs: %d\n", 
				NX, NY, NZ, STEPS, nprocs);
	Info info = setup(NX, NY, NZ, nprocs); //This is a single version
	nx = info.nx, ny = info.ny, nz = info.nz;
	long long size = nx*ny*nz;
	A = (double*)malloc(size*sizeof(double));
	B = (double*)malloc(size*sizeof(double));
	Init(A, size);
	struct timeval t1, t2;
	MPI_Barrier(MPI_COMM_WORLD), gettimeofday(&t1, NULL);
	stencil(A, B, nx, ny, nz, STEPS);
	MPI_Barrier(MPI_COMM_WORLD), gettimeofday(&t2, NULL);
	if(myrank == 0) printf("Total time: %.6lf\n", TIME(t1,t2)); 
	if(myrank == 0) cout << A[0] << " " << A[size/2] << " " << A[size-1] <<endl;
	printf("Total time: %.6lf\n", TIME(t1,t2)); 
	if(STEPS%2) Check(A, size);
	//Check(B, size);
	else        Check(A, size);
	free(A),free(B);
	MPI_Finalize();
}
