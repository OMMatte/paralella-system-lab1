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
#include <string>
#include <iostream>
#include <queue>

#define EXIT    "exit"

pthread_mutex_t mutex;    /* mutex lock for critical calculation section */

std::vector<std::string> inputLines;
int outWrites = 0;
int fileWrites = 0;

void *Writer(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
    int min;
    std::string line;
    std::string command;
    
    pthread_attr_t attr;
    pthread_t outWriter;
    pthread_t fileWriter;
    
    /* set global thread attributes */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    
    /* initialize mutex */
    pthread_mutex_init(&mutex, NULL);
    
    /* read command line args */
    if(argc < 2) {
        fprintf(stderr, "You did not write any command.\n");
        return 0;
    }

    command = argv[1];
    if(command != "tee" ) {
        fprintf(stderr, "Only command 'tee' exist.\n");
        exit(1);
    }

    if(argc != 3) {
        fprintf(stderr, "Usage: tee FILENAME\n");
        exit(1);
    }
    
    FILE *f = fopen(argv[2], "w");
    if (f == NULL)
    {
        printf("Failed to open file: %s for writing!\n", argv[2]);
        exit(1);
    }
    
    pthread_create(&fileWriter, &attr, Writer, f);
    pthread_create(&outWriter, &attr, Writer, stdout);
    
    
    while(true){
        std::getline(std::cin,line);
        pthread_mutex_lock(&mutex);
        inputLines.push_back(line);
        if(line == EXIT) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        min = std::min(outWrites, fileWrites);
        if(min > 0) {
            outWrites -= min;
            fileWrites -= min;
            inputLines.erase(inputLines.begin(), inputLines.begin() + min);
        }
        inputLines.push_back(line);
        pthread_mutex_unlock(&mutex);
    }
    pthread_join(outWriter, NULL);
    pthread_join(fileWriter, NULL);
    exit(0);
}

/* Each worker sums the values in one strip of the matrix.
 After a barrier, worker(0) computes and prints the total */
void *Writer(void *arg) {
    FILE* f = (FILE*) arg;
    std::string line;
    while(true) {
        pthread_mutex_lock(&mutex);
        if(fileWrites < inputLines.size()) {
            line = inputLines[fileWrites];
            if(line == EXIT) {
                pthread_mutex_unlock(&mutex);
                break;
            }
            fprintf(f, "%s", line.c_str());
            fileWrites++;
        }
        pthread_mutex_unlock(&mutex);
    }
    fclose(f);
    pthread_exit(NULL);
}