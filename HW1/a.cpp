/* matrix summation using pthreads
 
 features: uses a barrier; the Worker[0] computes
 the total sum, minumum element value and maxmimum
 element value from partial values computed by Workers
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
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

#define MINMAX_ARRAY_SIZE 6 /* fixed size of the array minMaxValues */
#define MAXVAL 0
#define MAXROW 1
#define MAXCOL 2
#define MINVAL 3
#define MINROW 4
#define MINCOL 5

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
int numWorkers;           /* number of workers */
int numArrived = 0;       /* number who have arrived */

/* a reusable counter barrier */
void Barrier() {
    pthread_mutex_lock(&barrier);
    numArrived++;
    if (numArrived == numWorkers) {
        numArrived = 0;
        pthread_cond_broadcast(&go);
    } else
        pthread_cond_wait(&go, &barrier);
    pthread_mutex_unlock(&barrier);
}

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
int sums[MAXWORKERS]; /* partial sums */
int minMaxValues[MAXWORKERS][MINMAX_ARRAY_SIZE]; /* partial min and max values. (MAXVAL, MAXROW, MAXCOL, MINVAL, MINROW, MINCOL) */
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
    
    /* initialize mutex and condition variable */
    pthread_mutex_init(&barrier, NULL);
    pthread_cond_init(&go, NULL);
    
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
    
    /* do the parallel work: create the workers */
    start_time = read_timer();
    for (l = 0; l < numWorkers; l++)
        pthread_create(&workerid[l], &attr, Worker, (void *) l);
    pthread_exit(NULL);
    return 0;
}

/* Each worker sums the values in one strip of the matrix.
 After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) {
    long myid = (long) arg;
    int val, total, i, j, first, last;
    
#ifdef DEBUG
    printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif
    
    /* determine first and last rows of my strip */
    first = myid*stripSize;
    last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);
    
    /* sum values in my strip */
    total = 0;
    
    int localMinMaxValues[MINMAX_ARRAY_SIZE];
    
    /* Initiating min and max values to the first value in the (sub) matrix. */
    localMinMaxValues[MAXVAL] = localMinMaxValues[MINVAL] = matrix[first][0];
    
    /* Initiate pos of minVal and maxVal */
    localMinMaxValues[MAXROW] = localMinMaxValues[MINROW] = first;
    localMinMaxValues[MAXCOL] = localMinMaxValues[MINCOL] = 0;
    
    for (i = first; i <= last; i++)
        for (j = 0; j < size; j++) {
            val = matrix[i][j];
            total += val;
            /* Update the min, max and pos. Note that can use if-else like this
             since we initiate minVal and maxVal to the same value, there for
             both if-statements can never be true at the same time */
            if(val < localMinMaxValues[MINVAL]) {
                localMinMaxValues[MINVAL] = val;
                localMinMaxValues[MINROW] = i;
                localMinMaxValues[MINCOL] = j;
            } else if(val > localMinMaxValues[MAXVAL]) {
                localMinMaxValues[MAXVAL] = val;
                localMinMaxValues[MAXROW] = i;
                localMinMaxValues[MAXCOL] = j;
            }
        }
    
    /* Save the local values for min and max */
    for(int i = 0; i < MINMAX_ARRAY_SIZE; i++) {
        minMaxValues[myid][i] = localMinMaxValues[i];
    }
    
    sums[myid] = total;
    Barrier();
    if (myid == 0) {
        total = 0;
        int minIndex = 0;
        int maxIndex = 0;
        /* find the index for lowest min and highest max within the partial variables */
        for (i = 0; i < numWorkers; i++) {
            total += sums[i];
            if(minMaxValues[i][MAXVAL] > minMaxValues[maxIndex][MAXVAL]) {
                maxIndex = i;
            }
            if(minMaxValues[i][MINVAL] < minMaxValues[minIndex][MINVAL]) {
                minIndex = i;
            }
        }
        /* get end time */
        end_time = read_timer();
        /* print results */
        printf("Maximum element value is %d at row/col position %d/%d\n", minMaxValues[maxIndex][MAXVAL], minMaxValues[maxIndex][MAXROW], minMaxValues[maxIndex][MAXCOL]);
        printf("Minimum element value is %d at row/col position %d/%d\n", minMaxValues[minIndex][MINVAL], minMaxValues[minIndex][MINROW], minMaxValues[minIndex][MINCOL]);
        printf("The total is %d\n", total);
        printf("The execution time is %g sec\n", end_time - start_time);
    }
    return 0;
}