#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 3

typedef struct thread_args {
    int tid;
    int inc;
    int loop;
    int loc;
} thread_args;

int count = 0;
pthread_mutex_t count_mutex;

void *inc_count(void *arg) {
    int i;
    thread_args *my_args = (thread_args *)arg;

    my_args->loc = 0;
    for (i = 0; i < my_args->loop; i++) {
        pthread_mutex_lock(&count_mutex);
        count = count + my_args->inc;
        pthread_mutex_unlock(&count_mutex);
        my_args->loc = my_args->loc + my_args->inc;
    }

    pthread_exit((void*) my_args);
}

int main(int argc, char *argv[]) {
    int i, loop, inc;
    thread_args *targs;
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;

    if (argc != 3) {
        printf("Usage: ./ptcount_mutex LOOP_BOUND INCREMENT\n");
        exit(0);
    }

    loop = atoi(argv[1]);
    inc = atoi(argv[2]);

    pthread_mutex_init(&count_mutex, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < NUM_THREADS; i++) {
        targs = malloc(sizeof(thread_args));
        targs->tid = i;
        targs->loop = loop;
        targs->inc = inc;
        pthread_create(&threads[i], &attr, inc_count, (void *)targs);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        thread_args *output;
        pthread_join(threads[i], (void **) &output);
        printf("Thread: %d finished. Counted: %d\n", output->tid, output->loc);
    }

    printf("Main(): Waited on %d threads. Final value of count = %d. Done.\n", NUM_THREADS, count);

    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&count_mutex);
    pthread_exit(NULL);

    return 0; // Optional, as pthread_exit(NULL) will terminate the process.
}


