/* matrix summation using pthreads
 
 features: uses a barrier; the Worker[0] computes
 the total sum from partial sums computed by Workers
 and prints the total sum to the standard output
 
 usage under Linux:
 gcc matrixSum.c -lpthread
 a.out size numWorkers
 
 */
#ifndef _REENTRANT
#define _REENTRANT
#endif
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#define MAXSIZE 1000      /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

#define MINMAX_ARRAY_SIZE 6 /* look at minMaxValues */
#define MAXVAL 0
#define MAXROW 1
#define MAXCOL 2
#define MINVAL 3
#define MINROW 4
#define MINCOL 5

pthread_mutex_t mutex;    /* mutex lock for critical calculation section */
int numWorkers;           /* number of workers */
int numArrived = 0;       /* number who have arrived */

/* timer */
double read_timer() {
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if( !initialized )
    {
        gettimeofday( &start, NULL );
        initialized = true;
    }
    gettimeofday( &end, NULL );
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

double start_time, end_time; /* start and end times */
int size, stripSize;  /* assume size is multiple of numWorkers */
int totalSum; /* total sum for the matrix */
int minMaxValues[MINMAX_ARRAY_SIZE]; /* Storage for min and max values for the matrix */
int matrix[MAXSIZE][MAXSIZE]; /* matrix */

void *Worker(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
    int i, j;
    long l; /* use long in case of a 64-bit system */
    pthread_attr_t attr;
    pthread_t workerid[MAXWORKERS];
    
    /* set global thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    
    /* initialize mutex */
    pthread_mutex_init(&mutex, NULL);
    
    /* read command line args if any */
    size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
    numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
    if (size > MAXSIZE) size = MAXSIZE;
    if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
    stripSize = size/numWorkers;
    
    /* initialize the matrix */
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            matrix[i][j] = rand()%99;
        }
    }
    
    /* print the matrix */
#ifdef DEBUG
    for (i = 0; i < size; i++) {
        printf("[ ");
        for (j = 0; j < size; j++) {
            printf(" %d", matrix[i][j]);
        }
        printf(" ]\n");
    }
#endif
    
    /* Initiating min and max values to the first value in the matrix.
     If empty matrix than zero values */
    bool isNotEmpty = size > 0;
    minMaxValues[MAXVAL] = minMaxValues[MINVAL] = isNotEmpty ? matrix[0][0]: 0;
    
    /* Initiate pos of minVal and maxVal. Set pos -1  if matrix is empty */
    minMaxValues[MAXROW] = minMaxValues[MAXCOL] = minMaxValues[MINROW] = minMaxValues[MINCOL] = isNotEmpty ? 0 : -1;
    
    /* do the parallel work: create the workers */
    start_time = read_timer();
    for (l = 0; l < numWorkers; l++)
        pthread_create(&workerid[l], &attr, Worker, (void *) l);
    for (l = 0; l < numWorkers; l++)
        pthread_join (workerid[l], NULL);
    
    /* get end time */
    end_time = read_timer();
    /* print results */
    printf("Maximum element value is %d at row/col position %d/%d\n", minMaxValues[MAXVAL], minMaxValues[MAXROW], minMaxValues[MAXCOL]);
    printf("Minimum element value is %d at row/col position %d/%d\n", minMaxValues[MINVAL], minMaxValues[MINROW], minMaxValues[MINCOL]);
    printf("The total is %d\n", totalSum);
    printf("The execution time is %g sec\n", end_time - start_time);
    pthread_exit(NULL);
}

/* Each worker sums the values in one strip of the matrix.
 After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) {
    long myid = (long) arg;
    int val, i, j, first, last, subTotal;
    
#ifdef DEBUG
    printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif
    
    /* determine first and last rows of my strip */
    first = myid*stripSize;
    last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);
    
    subTotal = 0;
    int subMinMaxValues[MINMAX_ARRAY_SIZE];
    
    /* Initiating min and max values to the first value in the matrix.
     If empty matrix than zero values */
    bool isNotEmpty = size > 0;
    subMinMaxValues[MAXVAL] = subMinMaxValues[MINVAL] = matrix[first][0];
    
    /* Initiate pos of minVal and maxVal. Set pos -1  if matrix is empty */
    subMinMaxValues[MAXROW] = subMinMaxValues[MINROW] = first;
    subMinMaxValues[MAXCOL] = subMinMaxValues[MINCOL] = 0;
    
    for (i = first; i <= last; i++)
        for (j = 0; j < size; j++) {
            val = matrix[i][j];
            subTotal += val;
            /* Update the min, max and pos. Note that can use if-else like this
             since we initiate minVal and maxVal to the same value, there for
             both if-statements can never be true at the same time */
            if(val < subMinMaxValues[MINVAL]) {
                subMinMaxValues[MINVAL] = val;
                subMinMaxValues[MINROW] = i;
                subMinMaxValues[MINCOL] = j;
            } else if(val > subMinMaxValues[MAXVAL]) {
                subMinMaxValues[MAXVAL] = val;
                subMinMaxValues[MAXROW] = i;
                subMinMaxValues[MAXCOL] = j;
            }
        }
    
    pthread_mutex_lock(&mutex);
    totalSum += subTotal;
    if(subMinMaxValues[MINVAL] < minMaxValues[MINVAL]) {
        minMaxValues[MINVAL] = val;
        minMaxValues[MINROW] = i;
        minMaxValues[MINCOL] = j;
    } else if(subMinMaxValues[MAXVAL] > minMaxValues[MAXVAL]) {
        minMaxValues[MAXVAL] = val;
        minMaxValues[MAXROW] = i;
        minMaxValues[MAXCOL] = j;
    }
    
    pthread_mutex_unlock(&mutex);
    
    pthread_exit(NULL);
}