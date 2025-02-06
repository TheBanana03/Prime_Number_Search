#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 4  // Fixed number of worker threads

typedef struct {
    uint32_t number;
    bool is_composite;
    pthread_mutex_t lock;
} Task;

typedef struct {
    Task* tasks;
    int task_count;
    int next_task;
    pthread_mutex_t lock;
} TaskQueue;

TaskQueue queue;
pthread_t workers[NUM_THREADS];

// Worker function for each thread
void* worker_function(void* arg) {
    while (1) {
        pthread_mutex_lock(&queue.lock);

        // Get the next available task
        if (queue.next_task >= queue.task_count) {
            pthread_mutex_unlock(&queue.lock);
            break;
        }

        Task* task = &queue.tasks[queue.next_task++];
        pthread_mutex_unlock(&queue.lock);

        // Primality test for the assigned number
        uint32_t n = task->number;
        if (n < 2) {
            task->is_composite = true;
            continue;
        }
        if (n == 2) {
            task->is_composite = false;
            continue;
        }
        if (n % 2 == 0) {
            task->is_composite = true;
            continue;
        }

        uint32_t limit = sqrt(n);
        for (uint32_t d = 3; d <= limit; d += 2) {
            if (n % d == 0) {
                pthread_mutex_lock(&task->lock);
                task->is_composite = true;
                pthread_mutex_unlock(&task->lock);
                break;
            }
        }
    }
    return NULL;
}

void check_primes(uint32_t* numbers, int count) {
    queue.tasks = malloc(count * sizeof(Task));
    queue.task_count = count;
    queue.next_task = 0;
    pthread_mutex_init(&queue.lock, NULL);

    // Initialize task queue
    for (int i = 0; i < count; i++) {
        queue.tasks[i].number = numbers[i];
        queue.tasks[i].is_composite = false;
        pthread_mutex_init(&queue.tasks[i].lock, NULL);
    }

    // Create fixed number of worker threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&workers[i], NULL, worker_function, NULL);
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(workers[i], NULL);
    }

    // Print results
    for (int i = 0; i < count; i++) {
        printf("Number %u is %s\n", queue.tasks[i].number, queue.tasks[i].is_composite ? "COMPOSITE" : "PRIME");
    }

    // Cleanup
    free(queue.tasks);
    pthread_mutex_destroy(&queue.lock);
}

int main() {
    uint32_t numbers[] = {17, 19, 20, 23, 25, 97, 99, 101, 10007, 100003};
    int count = sizeof(numbers) / sizeof(numbers[0]);

    check_primes(numbers, count);
    return 0;
}
