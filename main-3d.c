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

int Init(double *data, long long L);
int Check(double *data, long long L);


//Can be modified
int nprocs, myrank;
int nx, ny, nz;
int slice_sim_num = 2;
int slice_num = 2;
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
	int i, j, k, s;
	int nxy = nx * ny;
	int nxz = nx * nz;
	int nyz = ny * nz;
	int slice_x = (myrank / slice_sim_num2 % slice_sim_num) ;
        int slice_y = (myrank / slice_sim_num % slice_sim_num) ;
        int slice_z = (myrank % slice_sim_num) ;
        int buffer_s;
	int dx, dy, dz;
	int ii, jj, kk, counter;
	MPI_Status status;
	MPI_Request requests[12];
	int request_counter = 0;
	int buffer_counter = 0;
	MPI_Request request;
	//int my_rank;
	double* buffer_send = (double*)malloc(  (ny * nz + nx * ny +  nx * nz)*sizeof(double));	
//	double* buffer_recv = (double*)malloc(  (ny * nz + nx * ny +  nx * nz)*sizeof(double));	
	double* buffer_recv_xy = (double*)malloc(nx*ny*sizeof(double));
	double* buffer_recv_yz = (double*)malloc(ny*nz*sizeof(double));
	double* buffer_recv_xz = (double*)malloc(nx*nz*sizeof(double));
	for(s = 0; s < steps; s ++) {
		buffer_counter = 0;
		for (dx = -1; dx <= 1; dx++) {
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
						if (ABS(dx)) buffer_s = ny * nz;
						if (ABS(dy)) buffer_s = nx * nz;
						if (ABS(dz)) buffer_s = nx * ny;
						
						if (ABS(dx)) {
							if (dx > 0) memcpy(buffer_send + buffer_counter, A + IDX(nx-1, 0, 0), buffer_s * sizeof(double));
							else memcpy(buffer_send + buffer_counter, A + IDX(0, 0, 0), buffer_s * sizeof(double));
						}
						if (ABS(dy)) {
							counter = 0;
							for (ii = 0; ii < nx; ii++) {
								if (dy > 0) memcpy(buffer_send + buffer_counter + counter, A + IDX(ii, ny-1, 0), nz * sizeof(double));
								else memcpy(buffer_send + buffer_counter + counter, A + IDX(ii, 0, 0), nz * sizeof(double));
								counter += nz;
							}
						}
						if (ABS(dz)) {
							counter = 0;
							for (ii = 0; ii < nx; ii++) {
								for (jj = 0; jj < ny; jj++) {
									if (dz > 0) buffer_send[buffer_counter + counter++] = A[IDX(ii, jj, nz-1)];	
									else buffer_send[buffer_counter + counter++] = A[IDX(ii, jj, 0)];
								}
							}		
						}
						MPI_Isend(buffer_send + buffer_counter, buffer_s,
								MPI_DOUBLE, ad_rank, 0, MPI_COMM_WORLD, &requests[request_counter ++ ]);
						buffer_counter += buffer_s;
					}
				}
			}
		}
		for (dx = -1; dx <= 1; dx++) {
			for (dy = -1; dy <= 1; dy++) {
				for (dz = -1; dz <= 1; dz++) {
					int ad_x = slice_x + dx;
					int ad_y = slice_y + dy;
					int ad_z = slice_z + dz;
					int ad_rank = ad_x * slice_sim_num2
						+ ad_y *slice_sim_num + ad_z;
					if (ABS(dx)+ABS(dy)+ABS(dz) == 1 && ad_rank < nprocs && ad_rank >= 0 && pending(ad_x, ad_y, ad_z)) {
						int buffer_s; 
						if (ABS(dx)) {
							buffer_s = nyz;	
							MPI_Irecv(buffer_recv_yz, buffer_s,
								MPI_DOUBLE, ad_rank, 0, MPI_COMM_WORLD, &requests[request_counter++]);
						} 
						if (ABS(dy)) {
							buffer_s = nxz;
							MPI_Irecv(buffer_recv_xz, buffer_s,
								MPI_DOUBLE, ad_rank, 0, MPI_COMM_WORLD, &requests[request_counter++]);
						}

						if (ABS(dz)) {
							buffer_s = nxy;
							MPI_Irecv(buffer_recv_xy, buffer_s,
								MPI_DOUBLE, ad_rank, 0, MPI_COMM_WORLD, &requests[request_counter++]);
						}
					}
				}
			}
		}
		for(counter = 0; counter < request_counter; counter ++) {
			MPI_Wait(&requests[counter], &status);
		}
		request_counter = 0;
#pragma omp parallel for schedule (dynamic)
		for(i = 0; i < nx ; i ++) {
                        int temp_x = i * nyz;
                        for(j = 0; j < ny; j ++) {
                                int temp_y = j * nz; 
                                for(k = 0; k < nz; k ++) {
                                        int base_num = temp_x + temp_y + k;
                                        double r = 0.4*A[base_num];
                                        double temp = 0.0;
                                        if(k !=  0)   			temp += A[base_num - 1]; 
					else if(slice_z == 0)		temp += A[base_num];
                                        else          			temp += buffer_recv_xy[i*ny + j];
                                        if(k != nz-1)			temp += A[base_num + 1]; 
                                        else if(slice_z == slice_num-1)	temp += A[base_num];
					else 				temp += buffer_recv_xy[i*ny + j];
                                        if(j !=  0)			temp += A[base_num - nz];
                                        else if(slice_y == 0)		temp += A[base_num];
					else 				temp += buffer_recv_xz[i*nz + k];
                                        if(j != ny-1)			temp += A[base_num + nz];
                                        else if(slice_y == slice_num-1)	temp += A[base_num];
					else 				temp += buffer_recv_xz[i*nz + k];
                                        if(i !=  0)   			temp += A[base_num - nyz];
                                        else if(slice_x == 0)		temp += A[base_num];
                                        else 				temp += buffer_recv_yz[j*nx + k]; 
                                        if(i != nx-1) 			temp += A[base_num + nyz];
                                        else if(slice_x == slice_num-1)	temp += A[base_num];
                                        else                            temp += buffer_recv_yz[j*nx + k]; 
                                        B[base_num] = r + 0.1 * temp;
                                }
                        }
                }
	
		double *tmp = NULL;
		tmp = A, A = B, B = tmp;

	}
	free(buffer_send);
	free(buffer_recv_xy);
	free(buffer_recv_xz);
	free(buffer_recv_yz);
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
	//free(A),free(B);
	MPI_Finalize();
}
