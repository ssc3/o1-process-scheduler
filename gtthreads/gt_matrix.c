#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>

#include "gt_include.h"
#include "math.h"


#define ROWS 512
#define COLS ROWS
//#define SIZE COLS

#define NUM_CPUS 2
#define NUM_GROUPS NUM_CPUS
#define PER_GROUP_COLS (SIZE/NUM_GROUPS)

#define NUM_THREADS 128
#define PER_THREAD_ROWS (SIZE/NUM_THREADS)

int SIZE = 512;
int temp_array[128];
int CD[128][256][256];

typedef struct matrix
{
	int m[ROWS][ROWS];

	int rows;
	int cols;
	unsigned int reserved[2];
	gt_spinlock_t matrix_lock;
} matrix_t;


typedef struct __uthread_arg
{
	matrix_t *_A, *_B, *_C;
	unsigned int reserved0;

	unsigned int tid;
	unsigned int gid;
	int nice_value;
	int start_row; /* start_row -> (start_row + PER_THREAD_ROWS) */
	int start_col; /* start_col -> (start_col + PER_GROUP_COLS) */
	int end_row;
	int end_col;
}uthread_arg_t;
	
struct timeval tv1;

static void generate_matrix(matrix_t *mat, int val, int size)
{

	int i,j;
	mat->rows = size;
	mat->cols = size;
	for(i = 0; i < mat->rows;i++)
		for( j = 0; j < mat->cols; j++ )
		{
			mat->m[i][j] = val;
		}
	return;
}

static void print_matrix(matrix_t *mat)
{
	int i, j;

	for(i=0;i<SIZE;i++)
	{
		for(j=0;j<SIZE;j++)
			printf(" %d ",mat->m[i][j]);
		printf("\n");
	}

	return;
}

static void * uthread_mulmat(void *p)
{
	int i, j, k;
	int start_row, end_row;
	int start_col, end_col;
	unsigned int cpuid;
	struct timeval tv2;

#define ptr ((uthread_arg_t *)p)

	i=0; j= 0; k=0;



	/*Matrix multiplication here is done by trying to keep a small memory footprint. The results of each A[]*B[] are stored in the same matrix C[]. This way, with 128 uthreads there is less likelyhood of the program statically allocating 128 threads, each with three 512*512 matrices. So implemented gtspinlock on Matrix C structure so that only one thread can write to it at any given time*/

#ifdef SCHED_YIELD	
	if(!(gt_spin_lock_custom(&ptr->_C->matrix_lock)))
	{
		printf("\n Yielding \n");
		yield_custom();
	}
#endif
	start_row = 0;//ptr->start_row;
	end_row = ptr->end_row;//(ptr->start_row + PER_THREAD_ROWS);

#ifdef GT_GROUP_SPLIT
	start_col = ptr->start_col;
	end_col = (ptr->start_col + PER_THREAD_ROWS);
#else
	start_col = 0;//ptr->start_col;
	end_col = ptr->end_col;
#endif

	//fprintf(stderr,"\nLocked by thread %d, CPU %d",ptr->tid, ptr->gid);
#ifdef GT_THREADS
	cpuid = kthread_cpu_map[kthread_apic_id()]->cpuid;
	//fprintf(stderr, "\nThread(id:%d, group:%d, cpu:%d) started",ptr->tid, ptr->gid, cpuid);
#else
	//fprintf(stderr, "\nThread(id:%d, group:%d) started",ptr->tid, ptr->gid);
#endif
	//fprintf(stderr,"\n");
/*Actual multimplication*/
	for(i = start_row; i < ptr->end_row; i++)
		for(j = start_col; j < ptr->end_col; j++)
			for(k = 0; k < ptr->end_col; k++)
				CD[ptr->tid][i][j] += ptr->_A->m[i][k] * ptr->_B->m[k][j];

	/*Storing the result of multiplication in a temp array. Only one element of the multiplication result is stored for demo purposes*/
	temp_array[ptr->tid] = CD[ptr->tid][0][0];

	/*Reset matrix C[] for use by another thread*/


#ifdef GT_THREADS
	//fprintf(stderr, "\nThread(id:%d, group:%d, cpu:%d) finished (TIME : %lu s and %lu us)",
	//		ptr->tid, ptr->gid, cpuid, (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv1.tv_usec));
#else
	gettimeofday(&tv2,NULL);
	//fprintf(stderr, "\nThread(id:%d, group:%d) finished (TIME : %lu s and %lu us)",
	//		ptr->tid, ptr->gid, (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv1.tv_usec));

#endif

	//fprintf(stderr,"\nUnlocked by thread %d, CPU %d",ptr->tid, ptr->gid);
#ifdef SCHED_YIELD
	gt_spin_unlock(&(ptr->_C->matrix_lock));
#endif
#undef ptr

	return 0;
}
matrix_t A, B, C;

static void init_matrices(int size)
{
	generate_matrix(&A, 1, size);
	generate_matrix(&B, 1, size);
	generate_matrix(&C, 0, size);

	return;
}
long sum (long *array, int start, int end)
{
	int i;
	long sum_value = 0;
	for(i = start; i<end; i++)
	sum_value += array[i];

	return sum_value;
}
		
float standard_deviation(long *array, int start, int end, float mean)
{
	int i;
	long sd = 0;
	long temp = 0;
	for(i=start; i<end; i++)
	temp += (array[i] - mean)*(array[i] - mean);

	sd = (float) sqrt(temp/32);

	return sd;
}

void print_final_stats (int *arraytoprint)
{
	int i;
	
        //printf("\n*********** PRINTING THE MATRIX **********\n");

	//for (i = 0; i < NUM_THREADS; i++)
	//printf("\n%d",arraytoprint[i]);

        //printf("\n*********** END PRINT MATRIX *************\n");


        printf("\n*********** MEAN VALUES *************\n");

	float mean32 = (float) sum(stats.life_time, 0, 32)/32;
	float mean64 = (float) sum(stats.life_time, 32, 64)/32;
	float mean128 = (float) sum(stats.life_time, 64, 96)/32;
	float mean256 = (float) sum(stats.life_time, 96, 128)/32;

	printf("\n Mean of Create-to-End of 32 matrices of size 32x32 = %f usec \n", mean32);
	printf("\n Mean of Create-to-End of 32 matrices of size 64x64 = %f usec \n", mean64);
	printf("\n Mean of Create-to-End of 32 matrices of size 128x128 = %f usec \n", mean128);
	printf("\n Mean of Create-to-End of 32 matrices of size 256x256 = %f usec \n", mean256);

	mean32 = (float) sum(stats.running_time, 0, 32)/32;
	mean64 = (float) sum(stats.running_time, 32, 64)/32;
	mean128 = (float) sum(stats.running_time, 64, 96)/32;
	mean256 = (float) sum(stats.running_time, 96, 128)/32;



	printf("\n -------------------------------- \n");

	printf("\n Mean of CPU runtimes of 32 matrices of size 32x32 = %f usec \n", mean32);
	printf("\n Mean of CPU runtimes of 32 matrices of size 64x64 = %f usec\n", mean64);
	printf("\n Mean of CPU runtimes of 32 matrices of size 128x128 = %f usec\n", mean128);
	printf("\n Mean of CPU runtimes of 32 matrices of size 256x256 = %f usec\n", mean256);


        printf("\n*********** STANDARD DEVIATION VALUES *************\n");

	float sd32 = (float) standard_deviation(stats.life_time, 0, 32, mean32);
	float sd64 = (float) standard_deviation(stats.life_time, 32, 64, mean64);
	float sd128 = (float) standard_deviation(stats.life_time, 64, 96, mean128);
	float sd256 = (float) standard_deviation(stats.life_time, 96, 128, mean256);

	printf("\n SD of Create-to-End of 32 matrices of size 32x32 = %f usec \n", sd32);
	printf("\n SD of Create-to-End of 32 matrices of size 64x64 = %f usec \n", sd64);
	printf("\n SD of Create-to-End of 32 matrices of size 128x128 = %f usec \n", sd128);
	printf("\n SD of Create-to-End of 32 matrices of size 256x256 = %f usec \n", sd256);


	sd32 = (float) standard_deviation(stats.running_time, 0, 32, mean32);
	sd64 = (float) standard_deviation(stats.running_time, 32, 64, mean64);
	sd128 = (float) standard_deviation(stats.running_time, 64, 96, mean128);
	sd256 = (float) standard_deviation(stats.running_time, 96, 128, mean256);
	printf("\n -------------------------------- \n");

	printf("\n SD of CPU runtimes of 32 matrices of size 32x32 = %f usec \n", sd32);
	printf("\n SD of CPU runtimes of 32 matrices of size 64x64 = %f usec \n", sd64);
	printf("\n SD of CPU runtimes of 32 matrices of size 128x128 = %f usec \n", sd128);
	printf("\n SD of CPU runtimes of 32 matrices of size 256x256 = %f usec \n", sd256);

printf("\n******************** END OF THIS SET *********************\n");

	return;
}
/*When a new application function comes to the dispatcher as threads (after the CPUS/kthreads have been initialized), it gets it's nice values and initializes itself based on sizes)*/

void get_size_and_nice_values(int inx, int* SIZE, int* nice_value)
{
		if(*nice_value == 19)
		*nice_value = -19;

		if(inx <32)
		{
			*SIZE = 32;
			*nice_value = 10;
		}
		else if(inx > 31 && inx < 64)
		{
			*SIZE = 64;
			*nice_value = 5;
		}
		else if(inx > 63 && inx < 96)
		{
			*SIZE = 128;
			*nice_value = 0;
		}
		else
		{
			*SIZE = 256;
			*nice_value = -5;
		}
}

uthread_arg_t uargs[NUM_THREADS];
uthread_t utids[NUM_THREADS];

int main()
{
	uthread_arg_t *uarg;
	int inx;
	int nice_value = -19; 


	gtthread_app_init();

	SIZE = 256;
	init_matrices(SIZE);

	gettimeofday(&tv1,NULL);

	for(inx=0; inx<NUM_THREADS; inx++)
	{
		get_size_and_nice_values(inx, &SIZE, &nice_value);

		uarg = &uargs[inx];
		uarg->_A = &A;
		uarg->_B = &B;
		uarg->_C = &C;

		uarg->tid = inx;
		uarg->gid = (inx % NUM_GROUPS);

		uarg->start_row = (inx * PER_THREAD_ROWS);
#ifdef GT_GROUP_SPLIT
		/* Wanted to split the columns by groups !!! */
		uarg->start_col = (uarg->gid * PER_GROUP_COLS);
#endif


		uarg->start_row = 0;//(inx * PER_THREAD_ROWS);
		uarg->end_row = SIZE;
		uarg->start_col = 0;
		uarg->end_col = SIZE;
		uthread_create(&utids[inx], uthread_mulmat, uarg, uarg->gid, nice_value);
	}

	gtthread_app_exit();

	print_final_stats(temp_array);
	// print_matrix(&C);
	// fprintf(stderr, "********************************");
	return(0);
}
