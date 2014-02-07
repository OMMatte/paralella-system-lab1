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
#include <string>
#include <iostream>
#include <vector>
#include <fstream>

#define NR_COMPARERS 10 /* number of threads comparing lines */
#define UNCHECKED    0  /* represents an unchecked line */
#define EQUAL        1  /* represents a line that is equal */
#define UNEQUAL      2  /* represnets a line that is unequal */

pthread_mutex_t mutex;  /* mutex lock for critical calculation section */

std::vector<std::string> fileLines[2]; /* holder for lines for both files */
std::ifstream files[2];                /* holder for both the files */
std::vector<int> lineStatus;           /* the current status for a line, can be UNCHECKED, EQUAL or UNEQUAL */

long minLines;       /* will contain the minimum size of lines in regards to both files */
long nextLine = 0;   /* acts as a "bag". A comparer taking new lines begins at this point */
long slizeSize = 10; /* how many lines each comparer takes in one take */

void *Comparer(void *);
void *Reader(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
    long lineCounter = 0;
    std::string line;
    std::string command;
    
    pthread_attr_t attr;
    pthread_t fileReaders[2];
    pthread_t comparers[NR_COMPARERS];
    
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
    if(command != "diff" ) {
        fprintf(stderr, "Only command 'diff' exist.\n");
        exit(1);
    }

    if(argc != 4) {
        fprintf(stderr, "Usage: diff FILENAME FILENAME\n");
        exit(1);
    }
    
    /* try to open both files for reading */
    files[0].open(argv[2]);
    files[1].open(argv[3]);
    if (files[0].fail() || files[1].fail()) {
        fprintf(stderr, "Failed to open file: %s for reading!\n", (files[0].fail()) ? argv[2] : argv[3]);
        exit(1);
    }
    
    /* set to work the 2 reader workers for each file */
    for(long i = 0; i < 2; i++) {
        pthread_create(&fileReaders[i], &attr, Reader, (void*)i);
    }
    /* we wait until the readers are finished */
    for(long i = 0; i < 2; i++) {
        pthread_join(fileReaders[i], NULL);
    }
    
    minLines = std::min(fileLines[0].size(), fileLines[1].size());
    /* initiate att line statuses to UNCHECKED */
    lineStatus.resize(minLines, UNCHECKED);
    
    /* set the comparers in work to evaluate each line up to minLines */
    for(long i = 0; i < NR_COMPARERS; i++) {
        pthread_create(&comparers[i], &attr, Comparer, NULL);
    }
    
    /* while the comparers are working, the main thread evaluates the lines in order
     but only progresses if the lines have been checked. */
    while(lineCounter < minLines) {
        int status = lineStatus[lineCounter];
        if(status == UNCHECKED) {
            /* the line has not been checked yet, so we yield this timeslice and look again later */
            pthread_yield_np();
        } else {
            if(status == UNEQUAL) {
                /* print the lines */
                printf("(%lu): %s\n", lineCounter + 1, fileLines[0][lineCounter].c_str());
                printf("(%lu): %s\n", lineCounter + 1, fileLines[1][lineCounter].c_str());
            }
            lineCounter++;
        }
    }
    
    /* we print each line of the longest file, beginning at minLines. */
    std::vector<std::string> longestFileLines (fileLines[0].size() > fileLines[1].size() ? fileLines[0] : fileLines[1]);
    for( ; lineCounter < longestFileLines.size(); lineCounter++){
        printf("(%lu): %s\n", lineCounter + 1, longestFileLines[lineCounter].c_str());
    }
    exit(0);
}

void *Reader(void *arg) {
    long fileIndex = (long)arg;
    std::ifstream& readFile = files[fileIndex];
    std::string line;
    while(std::getline(readFile, line)) {
        fileLines[fileIndex].push_back(line);
    }
    pthread_exit(NULL);
}

void *Comparer(void *arg) {
    while(true){
        pthread_mutex_lock(&mutex);
        long startLine = nextLine;
        nextLine += slizeSize;
        pthread_mutex_unlock(&mutex);
        if(startLine >= minLines) {
            break;
        }
        for(long i = startLine; i < startLine + slizeSize && i < minLines; i++) {
            if(fileLines[0][i] != fileLines[1][i]) {
                lineStatus[i] = UNEQUAL;
            } else {
                lineStatus[i] = EQUAL;
            }
        }
    }
    pthread_exit(NULL);
}