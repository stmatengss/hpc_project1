#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>


#define TIME(a,b) (1.0*((b).tv_sec-(a).tv_sec)+0.000001*((b).tv_usec-(a).tv_usec))
#define IDX(i,j,k) ((i)*ny*nz+(j)*nz+(k))
#define ABS(x) (x>0?x:-x)
#define MAX(x, y) (x>y?x:y)
#define MIN(x, y) (x<y?x:y)

	extern int Init(double *data, long long L);
	extern int Check(double *data, long long L);


//Can be modified
int nprocs, myrank;
int nx, ny, nz;
int slice_sim_num = 2;
int slice_sim_num2 = 4;

int hash_s(int x, int y) {
	return nprocs * x + y;
}

int pending(int x, int y, int z) {
	if (x < slice_sim_num && x >=0 && 
	    y < slice_sim_num && y >=0 &&
            z < slice_sim_num && z >=0) return 1;
	return 0;
}

typedef struct {
	int nx, ny, nz;
} Info;

Info setup(int NX, int NY, int NZ, int P) {
	Info result;
	int myrank = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	result.nx = NX;
	result.ny = NY;
	result.nz = NZ;
	return result;
}


//Must be re-written, including all the parameters
int stencil(double *A, double *B, int nx, int ny, int nz, int steps)
{
	int test_begin[] = {0, 0, 0};
	int test_end[] = {3, 3, 3};
//	debugCuda("Cuda", A, test_begin, test_end, 0);
	int i, j, k, s;
	int nyz = ny * nz;
	//MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	//MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	int nx_dis = nx / slice_sim_num;
	int ny_dis = ny / slice_sim_num;
	int nz_dis = nz / slice_sim_num;
	int slice_x = (myrank / slice_sim_num2 % slice_sim_num) ;  
	int slice_y = (myrank / slice_sim_num % slice_sim_num) ;  
	int slice_z = (myrank % slice_sim_num) ;  
//	debug("slice_x", slice_x);	
//	debug("slice_y", slice_y);	
//	debug("slice_z", slice_z);	
	int iter_begin_x = slice_x * nx_dis;
	int iter_begin_y = slice_y * ny_dis;
	int iter_begin_z = slice_z * nz_dis;

	int dx, dy, dz;
	int ii, jj, kk, counter;
	int iter_end_x =  (slice_x + 1) * nx_dis;
	int iter_end_y =  (slice_y + 1) * ny_dis;
	int iter_end_z =  (slice_z + 1) * nz_dis;
	MPI_Status status;
	MPI_Request requests[12];
	int request_counter = 0;
	int buffer_counter = 0;
	MPI_Request request;
	int fuck_counter;
	//int my_rank;
	double* buffer_send = (double*)malloc( 44 + (ny_dis * nz_dis + nx_dis * ny_dis +  nx_dis * nz_dis)*sizeof(double));	
	double* buffer_recv = (double*)malloc( 44 + (ny_dis * nz_dis + nx_dis * ny_dis +  nx_dis * nz_dis)*sizeof(double));	
	for(s = 0; s < steps; s ++) {
		buffer_counter = 0;
		fuck_counter = 0;
//		if( myrank == 0) debug("round", s) ;
		for (dx = -1; dx <= 1; dx++) {
			if (s == 0) break;
			for (dy = -1; dy <= 1; dy++) {
				for (dz = -1; dz <= 1; dz++) {
					int ad_x = slice_x + dx;
					int ad_y = slice_y + dy;
					int ad_z = slice_z + dz;
					int ad_rank = ad_x * slice_sim_num2
						+ ad_y *slice_sim_num + ad_z;
					if (ABS(dx)+ABS(dy)+ABS(dz) == 1 && ad_rank < nprocs && ad_rank >= 0 && pending(ad_x, ad_y, ad_z)) {
						int buffer_s; 
						if (ABS(dx)) buffer_s = ny_dis * nz_dis;
						if (ABS(dy)) buffer_s = nx_dis * nz_dis;
						if (ABS(dz)) buffer_s = nx_dis * ny_dis;
						if(myrank==0)printf("buffer_s: %d, recv from %d to %d\n", buffer_s, myrank, ad_rank );
							// MPI Recv Packet
						MPI_Irecv(buffer_recv + buffer_counter, buffer_s,
							MPI_DOUBLE, ad_rank, 0, MPI_COMM_WORLD, &requests[request_counter++]);
					//	MPI_Recv(buffer_recv + buffer_counter, buffer_s,
				//			MPI_DOUBLE, ad_rank, hash_s(ad_rank, myrank), MPI_COMM_WORLD, &status);
					//	MPI_Irecv(buffer_recv + buffer_counter, buffer_s,
					//		MPI_DOUBLE, ad_rank, hash_s(ad_rank, myrank), MPI_COMM_WORLD, &requests[request_counter++]);
						
						if (ABS(dx)) {
							if (dx > 0) memcpy(A + IDX(iter_end_x, 0, 0), buffer_recv + buffer_counter, buffer_s * sizeof(double));
							else memcpy(A + IDX(iter_begin_x - 1, 0, 0), buffer_recv + buffer_counter, buffer_s * sizeof(double));
						}
						if (ABS(dy)) {
							counter = 0;
							for (ii = iter_begin_x; ii < iter_end_x; ii++) {
								if (dy > 0) memcpy(A + IDX(ii, iter_end_y , 0), buffer_recv + buffer_counter + counter, nz_dis * sizeof(double));
								else memcpy(A + IDX(ii, iter_begin_y - 1, 0), buffer_recv + buffer_counter + counter, nz_dis * sizeof(double));
								counter += nz_dis;
							}
						}
						if (ABS(dz)) {
							counter = 0;
							for (ii = iter_begin_x; ii < iter_end_x; ii++) {
								for (jj = iter_begin_y; jj < iter_end_y; jj++) {
									if (dz > 0) A[IDX(ii, jj, iter_end_z)] = buffer_recv[buffer_counter + counter++];	
									else A[IDX(ii, jj, iter_begin_z - 1)] = buffer_recv[buffer_counter + counter++] ;
								}
							}		
						}
						buffer_counter += buffer_s + 4;
					}
				}
			}
		}
		if (s != 0) {
			for(counter = 0; counter < request_counter; counter ++) {
				MPI_Wait(&requests[counter], &status);
			}
			request_counter = 0;
		}
	
#pragma omp parallel for schedule (dynamic)
		for(i = iter_begin_x; i < iter_end_x; i ++) {
			int temp_x = i * ny * nz;
			for(j = iter_begin_y; j < iter_end_y; j ++) {
				int temp_y = j * nz;
				for(k = iter_begin_z; k < iter_end_z; k ++) {
					double r = 0.0;
					if(k !=  0)   r += A[IDX(i,j,k-1)];
					else          r += A[IDX(i,j,k)];
					if(k != nz-1) r += A[IDX(i,j,k+1)];
					else          r += A[IDX(i,j,k)];
					if(j !=  0)   r += A[IDX(i,j-1,k)];
					else          r += A[IDX(i,j,k)];
					if(j != ny-1) r += A[IDX(i,j+1,k)];
					else          r += A[IDX(i,j,k)];
					if(i !=  0)   r += A[IDX(i-1,j,k)];
					else          r += A[IDX(i,j,k)];
					if(i != nx-1) r += A[IDX(i+1,j,k)];
					else          r += A[IDX(i,j,k)];
					B[IDX(i,j,k)] = r * 0.1 + 0.4*A[IDX(i,j,k)];
				}
			}
		}	
		double *tmp = NULL;
		tmp = A, A = B, B = tmp;
			
		buffer_counter = 0;

		for (dx = -1; dx <= 1; dx++) {
			if (s == steps - 1) break;
		//	request_counter = 0;
			for (dy = -1; dy <= 1; dy++) {
				for (dz = -1; dz <= 1; dz++) {
					int ad_x = slice_x + dx;
					int ad_y = slice_y + dy;
					int ad_z = slice_z + dz;
					int ad_rank = ad_x * slice_sim_num2
						+ ad_y *slice_sim_num + ad_z;
					if (ABS(dx)+ABS(dy)+ABS(dz) == 1 && ad_rank < nprocs && ad_rank >= 0 && pending(ad_x, ad_y, ad_z)) {
					//	debug("ad_rank", ad_rank);
						int buffer_s; 
						if (ABS(dx)) buffer_s = ny_dis * nz_dis;
						if (ABS(dy)) buffer_s = nx_dis * nz_dis;
						if (ABS(dz)) buffer_s = nx_dis * ny_dis;
						
						if (ABS(dx)) {
							if (dx > 0) memcpy(buffer_send + buffer_counter, A + IDX(iter_end_x - 1, 0, 0), buffer_s * sizeof(double));
							else memcpy(buffer_send + buffer_counter, A + IDX(iter_begin_x, 0, 0), buffer_s * sizeof(double));
						}
						if (ABS(dy)) {
							counter = 0;
							for (ii = iter_begin_x; ii < iter_end_x; ii++) {
								if (dy > 0) memcpy(buffer_send + buffer_counter + counter, A + IDX(ii, iter_end_y - 1, 0), nz_dis * sizeof(double));
								else memcpy(buffer_send + buffer_counter + counter, A + IDX(ii, iter_begin_y, 0), nz_dis * sizeof(double));
								counter += nz_dis;
							}
						}
						if (ABS(dz)) {
							counter = 0;
							for (ii = iter_begin_x; ii < iter_end_x; ii++) {
								for (jj = iter_begin_y; jj < iter_end_y; jj++) {
									if (dz > 0) buffer_send[buffer_counter + counter++] = A[IDX(ii, jj, iter_end_z - 1)];	
									else buffer_send[buffer_counter + counter++] = A[IDX(ii, jj, iter_begin_z)];
								}
							}		
						}

						// MPI Send Packet
						//MPI_Isend(buffer_send + buffer_counter, buffer_s,
						//		MPI_DOUBLE, ad_rank, hash_s(myrank, ad_rank), MPI_COMM_WORLD, &requests[request_counter ++ ]);
						if(myrank==0)printf("buffer_s: %d, send from %d to %d\n", buffer_s, myrank, ad_rank );
						MPI_Isend(buffer_send + buffer_counter, buffer_s,
								MPI_DOUBLE, ad_rank, 0, MPI_COMM_WORLD, &requests[request_counter ++ ]);
						buffer_counter += buffer_s + 222;
					}
				}
			}
		}
/*
		if (s != steps - 1) {
			for(counter = 0; counter < request_counter; counter ++) {
				MPI_Wait(&requests[counter], &status);
			}
			request_counter = 0;
		}
*/

	}
	free(buffer_send);
	free(buffer_recv);
	return 0;
}



int main(int argc, char **argv) {
	double *A = NULL, *B = NULL;
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
	//if(myrank == 0) printf("Total time: %.6lf\n", TIME(t1,t2)); 
	printf("Total time: %.6lf\n", TIME(t1,t2)); 
	//else        Check(A, size);
	Check(A, size), Check(B, size);
	free(A),free(B);
	MPI_Finalize();
}
