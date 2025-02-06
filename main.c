#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

extern void find_prime(uint64_t i, bool* result, float* v_256);

typedef struct {
    size_t stack_size;
    uint64_t* id;
    uint64_t start;
    uint64_t end;
    uint64_t* primes;
    uint64_t* count;
    pthread_mutex_t* lock;
} ThreadData;

typedef struct {
    uint32_t number;
    uint32_t next_divisor;
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
pthread_t* workers = NULL;

uint64_t x, y, m, a, p;

void read_config() {
    FILE* file = fopen("config.txt", "r");
    if (!file) {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "threads=", 8) == 0) {
            x = strtoul(line + 8, NULL, 10);
        } else if (strncmp(line, "ceiling=", 8) == 0) {
            y = strtoul(line + 8, NULL, 10);
        } else if (strncmp(line, "mode=", 5) == 0) {
            m = strtoul(line + 5, NULL, 10);
        } else if (strncmp(line, "avx=", 4) == 0) {
            a = strtoul(line + 4, NULL, 10);
        } else if (strncmp(line, "print=", 5) == 0) {
            p = strtoul(line + 5, NULL, 10);
        }
    }

    fclose(file);
}

void print_primes(uint64_t* primes, uint64_t y) {
    printf("\n%lu primes found", y);
    printf("\n%lu", primes[0]);
    for (uint64_t i=1; i<y; i++) {
        // printf(", %lu", primes[i]);
    }
    printf("\n");
}



void* worker_function(void* arg) {
    float divisors[8];  // AVX divisors

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
        uint32_t divisor;

        while (1) {
            pthread_mutex_lock(&task->lock);
            divisor = task->next_divisor;
            task->next_divisor += 16;
            pthread_mutex_unlock(&task->lock);

            if (divisor > limit) break;

            int count = 0;
            for (int i = 0; i < 8; i++) {
                if (divisor + (i * 2) > limit) break;
                divisors[i] = (float)(divisor + (i * 2));
                count++;
            }

            find_prime(n, &task->is_composite, divisors);

            if (task->is_composite) break;
        }
    }
    return NULL;
}

void check_primes(uint32_t end, uint64_t** primes, uint64_t* count) {
    queue.tasks = malloc((end + 1) * sizeof(Task));
    queue.task_count = end + 1;
    queue.next_task = 0;
    pthread_mutex_init(&queue.lock, NULL);

    for (int i = 0; i <= end; i++) {
        queue.tasks[i].number = i;
        queue.tasks[i].is_composite = false;
        queue.tasks[i].next_divisor = 3;
        pthread_mutex_init(&queue.tasks[i].lock, NULL);
    }

    for (int i = 0; i < x; i++) {
        pthread_create(&workers[i], NULL, worker_function, NULL);
    }

    for (int i = 0; i < x; i++) {
        pthread_join(workers[i], NULL);
    }

    *primes = malloc((end + 1) * sizeof(uint64_t));
    (*primes)[0] = 2;
    *count = 1;

    for (int i = 3; i <= end; i += 2) {
        if (!queue.tasks[i].is_composite) {
            (*primes)[(*count)++] = i;
        }
    }

    for (int i = 0; i <= end; i++) {
        pthread_mutex_destroy(&queue.tasks[i].lock);
    }
    free(queue.tasks);
    pthread_mutex_destroy(&queue.lock);
}






bool find_prime_avx(uint64_t i) {
    uint32_t lower = 3;
    uint32_t upper = lower + (2*7);
    bool result;
    int u, l;
    while (upper*upper<=i) {
        float v_256[8] = {lower, lower+2, lower+4, lower+6, lower+8, lower+10, lower+12, upper};
        result = true;

        find_prime(i, &result, v_256);
        if (!result)
            return true;
        lower+=16;
        upper+=16;
    }
    if (upper * upper > i) {
        if (i % lower == 0) return true;
        if (i % (lower + 2) == 0) return true;
        if (i % (lower + 4) == 0) return true;
        if (i % (lower + 6) == 0) return true;
        if (i % (lower + 8) == 0) return true;
        if (i % (lower + 10) == 0) return true;
        if (i % (lower + 12) == 0) return true;
    }
    return false;
}

bool find_prime_seq(uint64_t i) {
    uint64_t k = 3;
    // int m = sqrt(i);
    while (k*k<=i) {
        // if (i>750)
        // printf("Thread ID: %lu\tNumber: %d\tDivisor: %d\n", pthread_self(), i, k);
        if (!(i%k)) {
            return true;
        }
        k+=2;
    }
    // for (int k=3; k*k<=i; k+=2) {
    //     if (!(i%k))
    //         return true;
    // }
    return false;
}

uint64_t loop_to_y_seq(uint64_t* primes, uint64_t y) {
    uint64_t n = 1;

    if (a==1) {
        for (uint64_t i=3; i<y; i+=2) {
            if (find_prime_avx(i))
                continue;
            primes[n] = i;
            n++;
        }
    } else {
        for (uint64_t i=3; i<y; i+=2) {
            // if (find_prime_avx(i))
            if (find_prime_seq(i))
                continue;
            primes[n] = i;
            n++;
        }
    }
    return n;
}

void* loop_to_y_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    uint64_t local_count = 0;
    uint64_t* local_primes;
    bool heap = false;

    data->id = malloc(sizeof(uint64_t));
    *(data->id) = pthread_self();
    
    if ((data->end - data->start) * sizeof(uint64_t) < data->stack_size / 2) {
        local_primes = alloca((data->end - data->start) * sizeof(uint64_t));
    } else {
        local_primes = malloc((data->end - data->start) * sizeof(uint64_t));
        heap = true;
    }

    // printf("Thread ID: %lu\tStart: %lu\tEnd: %lu\n", pthread_self(), data->start, data->end);

    if (a==1) {
        for (uint64_t i=data->start; i<data->end; i+=2) {
            if (find_prime_avx(i))
                continue;
            // printf("Thread ID: %lu\tNumber: %lu\n", pthread_self(), i);
            local_primes[local_count++] = i;
        }
    } else {
        for (uint64_t i=data->start; i<data->end; i+=2) {
            if (find_prime_seq(i))
                continue;
            local_primes[local_count++] = i;
        }
    }

    // Store results in shared array with mutex protection
    pthread_mutex_lock(data->lock);
    for (uint64_t i = 0; i < local_count; i++) {
        data->primes[*(data->count)] = local_primes[i];
        (*(data->count))++;
    }
    pthread_mutex_unlock(data->lock);

    // printf("Thread %lu", *(data->id));
    //     if (local_primes[0])
    //         print_primes(local_primes, local_count);
    //     printf("\n");

    if (heap)
        free(local_primes);

    return NULL;
}

int main() {
    read_config();

    if (x%2 || y>11000000)
        a = 0;
    if (m==1) {
        workers = malloc(x * sizeof(pthread_t));
        if (!workers) {
            perror("Failed to allocate workers");
            exit(EXIT_FAILURE);
        }
    }
        

    uint64_t n = 0;
    clock_t start, end;
    double time;

    pthread_t threads[x];
    ThreadData thread_data[x];
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    uint64_t* primes = malloc(y * sizeof(uint64_t));
    uint64_t chunk_size = (y / x);
    uint64_t* partitions = malloc(x * sizeof(uint64_t));;
    uint64_t count = 0;

    // Get stack memory
    size_t stack_size;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    pthread_attr_getstacksize(&attr, &stack_size);
    pthread_attr_destroy(&attr);

    partitions[0] = 0;
    for (int i = 1; i <= x; i++) {
        partitions[i] = partitions[i-1] + chunk_size;
        if (!(partitions[i]%2) && partitions[i]!=y) {
            partitions[i]++;
        }
        if (partitions[i]!=y && i==x) {
            partitions[i] = y;
            break;
        }
    }
    partitions[0] = 3;
    for (int j = 0; j <= x; j++) {
        printf("Partition %d: %lu\n", j, partitions[j]);
    }
    printf("\n");

    start = clock();
    if (y>=2) {
        primes[0] = 2;
        n++;
        if (y!=2) {
            // n = loop_to_y_seq(primes, y);
            for (int i = 0; i < x; i++) {
                thread_data[i].stack_size = stack_size;
                thread_data[i].start = partitions[i];
                thread_data[i].end = partitions[i+1];
                thread_data[i].primes = primes;
                thread_data[i].count = &n;
                thread_data[i].lock = &lock;
                thread_data[i].id;

                pthread_create(&threads[i], NULL, loop_to_y_thread, &thread_data[i]);
            }

            for (int i = 0; i < x; i++) {
                pthread_join(threads[i], NULL);
                printf("Thread %lu\n", *(thread_data[i].id));
            }
        }
    }

    end = clock();
    
    if (primes[0])
        print_primes(primes, n);
    printf("\n");

    if (primes[0])
        // print_primes(primes, n);

    printf("Execution time: %f\n", ((end - start)) * 1.0 / CLOCKS_PER_SEC);

    if (m==1)
        free(workers);
    free(primes);
    free(partitions);
    pthread_mutex_destroy(&lock);

    return 0;
}