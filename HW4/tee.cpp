/*
 features: reads from standard input and prints
 to standard output and to the given file. Exits
 if the word 'exit' is written.
 
 usage under Linux:
 g++ tee.cpp -o tee
 tee file
 
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

#define EXIT    "exit"
#define INDEX_STDOUT 0
#define INDEX_FILEOUT 1

pthread_mutex_t mutex;    /* mutex lock for critical calculation section */

std::vector<std::string> inputLines;
int writes[2] = {0}; /* counter for writes for both the file and standard output */
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
    if(argc != 2) {
        fprintf(stderr, "Usage: tee FILENAME\n");
        exit(1);
    }
    
    FILE *f = fopen(argv[1], "w");
    if (f == NULL)
    {
        printf("Failed to open file: %s for writing!\n", argv[1]);
        exit(1);
    }
    
    /* start the writer for standard output and for the file*/
    pthread_create(&fileWriter, &attr, Writer, f);
    pthread_create(&outWriter, &attr, Writer, stdout);
    
    /* the main thread handles the standard input */
    while(true){
        std::getline(std::cin,line);
        /* when new input comes, make sure to lock since we are both adding the
         new line and also erasing old lines */
        pthread_mutex_lock(&mutex);
        inputLines.push_back(line);
        if(line == EXIT) {
            /* we exit if the exit command has been written */
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        /* clear all input that has been written allready */
        min = std::min(writes[INDEX_FILEOUT], writes[INDEX_STDOUT]);
        if(min > 0) {
            writes[INDEX_STDOUT] -= min;
            writes[INDEX_FILEOUT] -= min;
            inputLines.erase(inputLines.begin(), inputLines.begin() + min);
        }
        /* unlock to let the writers write */
        pthread_mutex_unlock(&mutex);
    }
    /* make sure that the writers are finished before exiting */
    pthread_join(outWriter, NULL);
    pthread_join(fileWriter, NULL);
    exit(0);
}

/* a writer writes to some file. The file can represent the standard output but does not have to */
void *Writer(void *arg) {
    int index;
    if(arg == stdout) {
        index = INDEX_STDOUT;
    } else {
        index = INDEX_FILEOUT;
    }
    FILE* f = (FILE*) arg;
    std::string line;
    while(true) {
        /* make sure to lock since the reader might alter the inputLines */
        pthread_mutex_lock(&mutex);
        if(writes[index] < inputLines.size()) {
            /* take the next line and increase the fileWrites index */
            line = inputLines[writes[index]];
            writes[index]++;
            pthread_mutex_unlock(&mutex);
            if(line == EXIT) {
                /* exit if the exit command was written */
                break;
            }
            fprintf(f, "%s", line.c_str());
        } else {
            /* no new lines so we can unlock and yield our time */
            pthread_mutex_unlock(&mutex);
            pthread_yield_np();
        }
        
    }
    /* make sure to close the file before the thread exits */
    fclose(f);
    pthread_exit(NULL);
}