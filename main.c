#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#define TIME(a,b) (1.0*((b).tv_sec-(a).tv_sec)+0.000001*((b).tv_usec-(a).tv_usec))
#define IDX(i,j,k) ((i)*ny*nz+(j)*nz+(k))

extern int Init(double *data, long long L);
extern int Check(double *data, long long L);

//Can be modified
typedef struct
{
  int nx, ny, nz;
} Info;

int nprocs, myrank;
//Must be modified because only single process is used now
Info setup(int NX, int NY, int NZ, int P)  
{
  Info result;
  int myrank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  if(myrank == 0)
  {
    result.nx = NX;
    result.ny = NY;
    result.nz = NZ;
  }
  else
  {
    result.nx = 0;
    result.ny = 0;
    result.nz = 0;
  }
  return result;
}

int stencil(double *A, double *B, int nx, int ny, int nz, int steps)
{
	int i, j, k, s;
	int nx_dis = nx / nprocs;
	int slice_s = ny * nz;
	int iter_begin = myrank * nx_dis;
	int iter_end = (myrank + 1) * nx_dis;
	MPI_Status status;
	if (myrank == nprocs - 1) {
		iter_end = nx;
	} 
	//int my_rank;
	//MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	//cout << "The process is " << myrank << endl; 
	for(s = 0; s < steps; s ++) {
		if (myrank != 0) {
			MPI_Send(A + IDX(iter_begin, 0, 0), slice_s, 
				MPI_DOUBLE, myrank-1, 0, MPI_COMM_WORLD );
		}
		if (myrank != nprocs-1) {
			MPI_Send(A + IDX(iter_end-1, 0, 0), slice_s,
				MPI_DOUBLE, myrank+1, 0, MPI_COMM_WORLD );
		}
//#pragma omp parallel for schedule (dynamic)
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
					if (i < 9 && j < 9 && k < 9 && i > 6 && j > 6 && k > 6) printf(" %d,%d,%d,%d,%.10lf\n",s,i,j,k,A[IDX(i,j,k)]);
				}
			}
		}
		if (myrank != 0) {
			MPI_Recv(A + IDX(iter_begin-1, 0, 0), slice_s,
				MPI_DOUBLE, myrank-1, 1, MPI_COMM_WORLD, &status );
		}
		if (myrank != nprocs-1) {
		MPI_Recv(A + IDX(iter_end, 0, 0), slice_s,
				MPI_DOUBLE, myrank+1, 1, MPI_COMM_WORLD, &status );	
		}
		
		double *tmp = NULL;
		tmp = A, A = B, B = tmp;
	}
	return 0;
}


int main(int argc, char **argv) {
  double *A = NULL, *B = NULL;
  int nx, ny, nz;
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
  if(STEPS%2) Check(B, size);
  else        Check(A, size);
  free(A),free(B);
  MPI_Finalize();
}
