#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 4  // Fixed number of worker threads

typedef struct {
    uint32_t number;
    bool is_composite;
} Task;

typedef struct {
    Task* tasks;
    int task_count;
    int next_task;
    pthread_mutex_t lock;
} TaskQueue;

TaskQueue queue;
pthread_t workers[NUM_THREADS];

void* worker_function(void* arg) {
    while (1) {
        pthread_mutex_lock(&queue.lock);

        if (queue.next_task >= queue.task_count) {
            pthread_mutex_unlock(&queue.lock);
            break;
        }

        Task* task = &queue.tasks[queue.next_task++];
        pthread_mutex_unlock(&queue.lock);

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
                task->is_composite = true;
                break;
            }
        }
    }
    return NULL;
}

void check_primes(uint32_t start, uint32_t end) {
    int count = end - start + 1;
    queue.tasks = malloc(count * sizeof(Task));
    queue.task_count = count;
    queue.next_task = 0;
    pthread_mutex_init(&queue.lock, NULL);

    for (int i = 0; i < count; i++) {
        queue.tasks[i].number = start + i;
        queue.tasks[i].is_composite = false;
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&workers[i], NULL, worker_function, NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(workers[i], NULL);
    }

    for (int i = 0; i < count; i++) {
        printf("Number %u is %s\n", queue.tasks[i].number, queue.tasks[i].is_composite ? "COMPOSITE" : "PRIME");
    }

    free(queue.tasks);
    pthread_mutex_destroy(&queue.lock);
}

int main() {
    uint32_t start, end;
    printf("Enter start of range: ");
    scanf("%u", &start);
    printf("Enter end of range: ");
    scanf("%u", &end);

    if (start > end) {
        printf("Invalid range. Start should be <= end.\n");
        return 1;
    }

    check_primes(start, end);
    return 0;
}
