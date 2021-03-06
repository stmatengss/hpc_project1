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

#define TEST_WAIT

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

int hash_s(int x, int y) {
	return x * nprocs + y; 
}
//Must be modified because only single process is used now
Info setup(int NX, int NY, int NZ, int P)
{
	Info result;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	result.nx = NX;
	result.ny = NY;
	result.nz = NZ;
 	int dis = NX / nprocs;	
	result.nx = dis;
	result.ny = NY;
	result.nz = NZ;
	if (myrank < NX % nprocs)
	{
		result.nx ++;
	}
	return result;
}

void gather(double *A, int nx, int ny, int nz, int nx_dis, int slice_s) {
	int slice_v = slice_s * nx_dis;
	if (myrank == 0) {
		for(int i = 1; i < nprocs; i++) {
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
	MPI_Status status;
	MPI_Request request[4];
	int nyz = ny * nz;
	int slice_s = nyz; 
	int nxyz = nx * ny * nz;	
	double* buffer_left = (double*)malloc(sizeof(double) * nyz);
	double* buffer_right = (double*)malloc(sizeof(double) * nyz);
	//int my_rank;
	for (s = 0; s < steps; s ++) {
		if (myrank != 0) {
			MPI_Isend(A, slice_s, 
				MPI_DOUBLE, myrank-1, hash_s(myrank, myrank-1), MPI_COMM_WORLD, &request[0] );
		}
		if (myrank != nprocs-1) {
			MPI_Isend(A + IDX(nx-1, 0, 0), slice_s,
				MPI_DOUBLE, myrank+1, hash_s(myrank, myrank+1), MPI_COMM_WORLD, &request[2] );
		
		}

		if (myrank != 0) {
			MPI_Irecv(buffer_left, slice_s,
				MPI_DOUBLE, myrank-1, hash_s(myrank-1, myrank), MPI_COMM_WORLD, &request[1] );
		}
		if (myrank != nprocs-1) {
			MPI_Irecv(buffer_right, slice_s,
				MPI_DOUBLE, myrank+1, hash_s(myrank+1, myrank), MPI_COMM_WORLD, &request[3] );	
		}
		if (myrank == 0) {
			MPI_Waitall(2, request+2, MPI_STATUS_IGNORE);
		} else if (myrank == nprocs-1) {
			MPI_Waitall(2, request, MPI_STATUS_IGNORE);
		} else {
			MPI_Waitall(4, request, MPI_STATUS_IGNORE);
		}	
#pragma omp parallel for schedule (dynamic) 
		for(i = 0; i < nx ; i ++) {
			int temp_x = i * nyz;
			for(j = 0; j < ny; j ++) {
				int temp_y = j * nz;
				for(k = 0; k < nz; k ++) {
					int base_num = temp_x + temp_y + k;
					double r = 0.4*A[base_num];
					double temp = 0.0;
					if(k !=  0)   temp += A[base_num - 1];
					else          temp += A[base_num];
					if(k != nz-1) temp += A[base_num + 1];
					else          temp += A[base_num];
					if(j !=  0)   temp += A[base_num - nz];
					else          temp += A[base_num];
					if(j != ny-1) temp += A[base_num + nz];
					else          temp += A[base_num];
					if(i !=  0)   temp += A[base_num - nyz];
					else if(myrank == 0)	temp += A[base_num];
					else 			temp += buffer_left[temp_y + k];
					if(i != nx-1) temp += A[base_num + nyz];
					else if(myrank == nprocs-1)	temp += A[base_num];
					else				temp += buffer_right[temp_y + k];
					B[base_num] = r + 0.1 * temp;
				}
			}
		}

		double *tmp = NULL;
		tmp = A, A = B, B = tmp;

				
	}

	return 0;
}

double *A = NULL, *B = NULL;


int main(int argc, char **argv) {
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
	printf("nx=%d, ny=%d, nz=%d\n", nx, ny, nz);
	A = (double*)malloc(size*sizeof(double));
	B = (double*)malloc(size*sizeof(double));
	Init(A, size);
	struct timeval t1, t2;
	MPI_Barrier(MPI_COMM_WORLD), gettimeofday(&t1, NULL);
	stencil(A, B, nx, ny, nz, STEPS);
	MPI_Barrier(MPI_COMM_WORLD), gettimeofday(&t2, NULL);
	if(myrank == 0) printf("Total time: %.6lf\n", TIME(t1,t2)); 
	printf("Total time: %.6lf\n", TIME(t1,t2)); 
	//if(STEPS%2) Check(A, size);
	Check(A, size);
	Check(B, size);
	//else        Check(A, size);
	free(A),free(B);
	MPI_Finalize();
}
