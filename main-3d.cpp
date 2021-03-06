#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <sys/time.h>
#include <iostream>
#include <cstring>
#include <algorithm>

using namespace std;

#define TIME(a,b) (1.0*((b).tv_sec-(a).tv_sec)+0.000001*((b).tv_usec-(a).tv_usec))
#define IDX(i,j,k) ((i)*ny*nz+(j)*nz+(k))
#define ABS(x) (x>0?x:-x)
#define MAX(x, y) (x>y?x:y)
#define MIN(x, y) (x<y?x:y)

#ifdef __cplusplus
extern "C" {               // 告诉编译器下列代码要以C链接约定的模式进行链接
#endif


	extern int Init(double *data, long long L);
	extern int Check(double *data, long long L);


#ifdef __cplusplus
}
#endif
//Can be modified
int nprocs, myrank;
int nx, ny, nz;
const int slice_sim_num = 2;
const int slice_sim_num2 = 4;

int hash_s(int x, int y) {
	return nprocs * x + y;
}

inline bool pending_bound(int x, int y, int z) {
	if (x < slice_sim_num && x >= 0 &&
	    y < slice_sim_num && y >= 0 &&
	    z < slice_sim_num && z >= 0) return true;
	return false;	
}

typedef struct {
	int nx, ny, nz;
} Info;

template<typename T>
void debug(string title, T a) {
	cout << title << ":" << a <<endl;
}

void debugCuda(string title, double *A, int begin[], int end[] ) {
	cout << "---" << title << "---" << endl;
	for (int i = begin[0]; i < end[0]; i++) {
		cout << "A:" << i << endl;
		for (int j = begin[1]; j < end[1] ; j ++) {
			for (int k = begin[2]; k < end[2]; k++) {
				cout << A[IDX(i, j, k)] << ' ';
			}

			cout << endl;
		}
		cout << endl;
	}
}
void debugCuda(string title, double *A, int begin[], int end[], int p ) {

	cout << "---" << title << "---" << endl;
	int test_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &test_rank);
	if (test_rank != p) return;
	for (int i = begin[0]; i < end[0]; i++) {
		cout << "A:" << i << endl;
		for (int j = begin[1]; j < end[1] ; j ++) {
			for (int k = begin[2]; k < end[2]; k++) {
				cout << A[IDX(i, j, k)] << ' ';
			}

			cout << endl;
		}
		cout << endl;
	}
}//Must be modified because only single process is used now
Info setup(int NX, int NY, int NZ, int P) {
	Info result;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	result.nx = NX / slice_sim_num;
	result.ny = NY / slice_sim_num;
	result.nz = NZ / slice_sim_num;
	if (myrank < NX % slice_sim_num) {
		result.nx ++;
	}
	if (myrank < NY % slice_sim_num) {
		result.ny ++;
	}
	if (myrank < NZ % slice_sim_num) {
		result.nz ++;
	}
	return result;
}


//Must be re-written, including all the parameters
int stencil(double *A, double *B, int nx, int ny, int nz, int steps)
{
	int test_begin[] = {0, 0, 0};
	int test_end[] = {3, 3, 3};
//	debugCuda("Cuda", A, test_begin, test_end, 0);
	int i, j, k, s;
	int dx, dy, dz;
	int ad_x, ad_y, ad_z;
	int ad_rank;
	int nyz = ny * nz;
	//MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	//MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	int nx_dis = nx / slice_sim_num;
	int ny_dis = ny / slice_sim_num;
	int nz_dis = nz / slice_sim_num;
	int slice_x = (myrank / slice_sim_num2 % slice_sim_num) ;  
	int slice_y = (myrank / slice_sim_num % slice_sim_num) ;  
	int slice_z = (myrank % slice_sim_num) ;  
	int buffer_s;
//	debug("slice_x", slice_x);	
//	debug("slice_y", slice_y);	
//	debug("slice_z", slice_z);	
	int iter_begin_x = slice_x * nx_dis;
	int iter_begin_y = slice_y * ny_dis;
	int iter_begin_z = slice_z * nz_dis;
	int iter_end_x = min(nx, (slice_x + 1) * nx_dis);
	int iter_end_y = min(ny, (slice_y + 1) * ny_dis);
	int iter_end_z = min(nz, (slice_z + 1) * nz_dis);
	MPI_Status status;
	MPI_Request requests[12];
	int request_counter = 0;
	int buffer_counter = 0;
	MPI_Request request;
	//int my_rank;
	double* buffer_send = (double*)malloc(2 * (ny_dis * nz_dis + nx_dis * ny_dis +  nx_dis * nz_dis)*sizeof(double));	
	double* buffer_recv = (double*)malloc(2 * (ny_dis * nz_dis + nx_dis * ny_dis +  nx_dis * nz_dis)*sizeof(double));	
	for(s = 0; s < steps; s ++) {
		buffer_counter = 0;
		int fuck_count = 0;
//		if( myrank == 0) debug("round", s) ;
		for (dx = -1; dx <= 1; dx++) {
			if (s == 0) break;
			for (dy = -1; dy <= 1; dy++) {
				for (dz = -1; dz <= 1; dz++) {
					ad_x = slice_x + dx;
					ad_y = slice_y + dy;
					ad_z = slice_z + dz;
					ad_rank = ad_x * slice_sim_num2
						+ ad_y *slice_sim_num + ad_z;
					//if(myrank == 0)debug("n", ad_rank)
					
					//if(myrank == 0) cout << ad_x << ad_y  <<ad_z <<endl;
					if (ABS(dx)+ABS(dy)+ABS(dz) == 1 && ad_rank < nprocs && ad_rank >= 0 && pending_bound(ad_x, ad_y, ad_z)) {
//						debug("ad_rank", ad_rank);
						buffer_s; 
						fuck_count ++;
						if (ABS(dx)) buffer_s = ny_dis * nz_dis;
						if (ABS(dy)) buffer_s = nx_dis * nz_dis;
						if (ABS(dz)) buffer_s = nx_dis * ny_dis;
							// MPI Recv Packet
						MPI_Irecv(buffer_recv + buffer_counter, buffer_s,
							MPI_DOUBLE, ad_rank, hash_s(ad_rank, myrank), MPI_COMM_WORLD, &requests[request_counter++]);
					//	MPI_Recv(buffer_recv + buffer_counter, buffer_s,
				//			MPI_DOUBLE, ad_rank, hash_s(ad_rank, myrank), MPI_COMM_WORLD, &status);
					
						if (ABS(dx)) {
							if (dx > 0) memcpy(A + IDX(iter_end_x, 0, 0), buffer_recv + buffer_counter, buffer_s * sizeof(double));
							else memcpy(A + IDX(iter_begin_x - 1, 0, 0), buffer_recv + buffer_counter, buffer_s * sizeof(double));
						}
						if (ABS(dy)) {
							int counter = 0;
							for (int ii = iter_begin_x; ii < iter_end_x; ii++) {
								if (dy > 0) memcpy(A + IDX(ii, iter_end_y , 0), buffer_recv + buffer_counter + counter, nz_dis * sizeof(double));
								else memcpy(A + IDX(ii, iter_begin_y - 1, 0), buffer_recv + buffer_counter + counter, nz_dis * sizeof(double));
								counter += nz_dis;
							}
						}
						if (ABS(dz)) {
							int counter = 0;
							for (int ii = iter_begin_x; ii < iter_end_x; ii++) {
								for (int jj = iter_begin_y; jj < iter_end_y; jj++) {
									if (dz > 0) A[IDX(ii, jj, iter_end_z)] = buffer_recv[buffer_counter + counter++];	
									else A[IDX(ii, jj, iter_begin_z - 1)] = buffer_recv[buffer_counter + counter++] ;
								}
							}		
						}
						buffer_counter += buffer_s;
					}
				}
			}
		}
		if(myrank==0)debug("fucking", fuck_count);
		if (s != 0) {
			//debug("debug", request_counter);
			for(int counter = 0; counter < request_counter; counter ++) {
				MPI_Wait(&requests[counter], &status);
			}
			request_counter = 0;
		}

#pragma omp parallel for schedule (dynamic)
		for(i = 0; i < nx; i ++) {
			int temp_x = i * ny * nz;
			for(j = 0; j < ny; j ++) {
				int temp_y = j * nz;
				for(k = 0; k < nz; k ++) {
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
				//	
		if (myrank == 0) {
		//	debug("A[0,0,0]", A[IDX(0,0,0)]);
		//	debug("A[n,n,n]", A[IDX(iter_end_x-1, iter_end_y-1, iter_end_z-1)]);
		}
			double *tmp = NULL;
		tmp = A, A = B, B = tmp;
		
		buffer_counter = 0;

		for (int dx = -1; dx <= 1; dx++) {
			if (s == steps - 1) break;
			for (int dy = -1; dy <= 1; dy++) {
				for (int dz = -1; dz <= 1; dz++) {
					int ad_x = slice_x + dx;
					int ad_y = slice_y + dy;
					int ad_z = slice_z + dz;
					int ad_rank = ad_x * slice_sim_num2
						+ ad_y *slice_sim_num + ad_z;
					if (ABS(dx)+ABS(dy)+ABS(dz) == 1 && ad_rank < nprocs && ad_rank >= 0) {
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
							int counter = 0;
							for (int ii = iter_begin_x; ii < iter_end_x; ii++) {
								if (dy > 0) memcpy(buffer_send + buffer_counter + counter, A + IDX(ii, iter_end_y - 1, 0), nz_dis * sizeof(double));
								else memcpy(buffer_send + buffer_counter + counter, A + IDX(ii, iter_begin_y, 0), nz_dis * sizeof(double));
								counter += nz_dis;
							}
						}
						if (ABS(dz)) {
							int counter = 0;
							for (int ii = iter_begin_x; ii < iter_end_x; ii++) {
								for (int jj = iter_begin_y; jj < iter_end_y; jj++) {
									if (dz > 0) buffer_send[buffer_counter + counter++] = A[IDX(ii, jj, iter_end_z - 1)];	
									else buffer_send[buffer_counter + counter++] = A[IDX(ii, jj, iter_begin_z)];
								}
							}		
						}
						// MPI Send Packet
						MPI_Isend(buffer_send + buffer_counter, buffer_s,
								MPI_DOUBLE, ad_rank, hash_s(myrank, ad_rank), MPI_COMM_WORLD, &request);
						buffer_counter += buffer_s;
					}
				}
			}
		}

	}
	//debug("fuck", "fuck");
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
	//debug("shit", "shit");
	MPI_Barrier(MPI_COMM_WORLD), gettimeofday(&t2, NULL);
	if(myrank == 0) printf("Total time: %.6lf\n", TIME(t1,t2)); 
	Check(A, size)
	Check(B, size);
	free(A),free(B);
	MPI_Finalize();
}
