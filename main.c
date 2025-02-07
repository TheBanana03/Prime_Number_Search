#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <stdatomic.h>

extern void find_prime(uint64_t i, bool* result, float* v_256);

typedef struct {
    size_t stack_size;
    uint64_t* id;
    uint64_t start;
    uint64_t end;
    uint64_t* primes;
    uint64_t* timestamps;
    uint64_t* threadids;
    uint64_t* count;
    pthread_mutex_t* lock;
} ThreadData;

typedef struct {
    uint32_t number;
    uint32_t next_divisor;
    bool is_composite;
    pthread_mutex_t lock;
    uint64_t threadid;
    uint64_t timestamp;
} Task;

typedef struct {
    Task* tasks;
    int task_count;
    int next_task;
    pthread_mutex_t lock;
} TaskQueue;

TaskQueue queue;
pthread_t* workers = NULL;

int64_t x, y, m, a, p;

void read_config() {
    FILE* file = fopen("config.txt", "r");
    if (!file) {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }

    bool inputGood = true;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = 0;
        char* endptr;

        if (strncmp(line, "threads=", 8) == 0) {
            x = strtoul(line + 8, &endptr, 10);
            if (*endptr != '\0' || x <= 0) {  // Ensure it's a valid integer and positive
                fprintf(stderr, "Invalid value for threads: %s (must be a positive integer)\n", line + 8);
                inputGood = false;
            }
        } else if (strncmp(line, "ceiling=", 8) == 0) {
            y = strtoul(line + 8, &endptr, 10);
            if (*endptr != '\0' || y <= 0) {  // Ensure it's a valid integer and positive
                fprintf(stderr, "Invalid value for ceiling: %s (must be a positive integer)\n", line + 8);
                inputGood = false;
            } else if (y > 1000000000) {
                fprintf(stderr, "Invalid value for ceiling: %s (must be less than or equal to 1,000,000,000)\n", line + 8);
                inputGood = false;
            }
        } else if (strncmp(line, "mode=", 5) == 0) {
            m = strtoul(line + 5, &endptr, 10);
            if (*endptr != '\0' || m != 0 && m != 1) {
                fprintf(stderr, "Invalid value for mode: %s (must be 0 or 1)\n", line + 5);
                inputGood = false;
            }
        } else if (strncmp(line, "avx=", 4) == 0) {
            a = strtoul(line + 4, &endptr, 10);
            if (*endptr != '\0' || a != 0 && a != 1) {
                fprintf(stderr, "Invalid value for avx: %s (must be 0 or 1)\n", line + 4);
                inputGood = false;
            }
        } else if (strncmp(line, "print=", 6) == 0) {
            p = strtoul(line + 6, &endptr, 10);
            if (*endptr != '\0' || p != 0 && p != 1) {
                fprintf(stderr, "Invalid value for print: %s (must be 0 or 1)\n", line + 6);
                inputGood = false;
            }
        }
    }

    fclose(file);

    if (!inputGood) {
        exit(EXIT_FAILURE);
    }
}

void print_primes(uint64_t* primes, uint64_t* timestamps, uint64_t* threadids, uint64_t y) {
    printf("\n");
    for (uint64_t i=0; i<y; i++) {
        printf("Thread %lu:\t%lu\t(Timestamp: %f seconds)\n",
                threadids[i], primes[i], timestamps[i] / 1e9);
    }
}



void* worker_function(void* arg) {
    float divisors[8];
    struct timespec ts;

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
            if (task->is_composite)
                break;

            pthread_mutex_lock(&task->lock);
            divisor = task->next_divisor;
            if ((task->next_divisor+16)>y)
                task->next_divisor=y;
            else {
                task->next_divisor+=16;
            }
            pthread_mutex_unlock(&task->lock);

            if (divisor > limit) {
                clock_gettime(CLOCK_MONOTONIC, &ts);
                if (p==0) {
                    printf("Thread %lu:\t%d\t(Timestamp: %ld.%2ld seconds)\n", pthread_self(), n, ts.tv_sec, ts.tv_nsec);
                } else if (p==1) {
                    task->timestamp = ts.tv_sec * 1e9 + ts.tv_nsec;
                    task->threadid = pthread_self();
                }
                break;
            }

            int count = 0;
            for (int i = 0; i < 8; i++) {
                if (divisor+(i*2) > limit) break;
                divisors[i] = (float)(divisor + (i * 2));
                count++;
            }
            
            if (count==8 && a==1) {
                find_prime(n, &task->is_composite, divisors);
                task->is_composite = !(task->is_composite);
            }
            else {
                for (int i = 0; i < count; i++) {
                    if (!(n%(int)divisors[i])) {
                        task->is_composite = true;
                    }
                }
            }
        }
    }
    return NULL;
}

uint64_t check_primes(uint32_t end, uint64_t* primes, uint64_t* timestamps, uint64_t* threadids) {
    queue.tasks = malloc((end + 1) * sizeof(Task));
    queue.task_count = end + 1;
    queue.next_task = 0;
    pthread_mutex_init(&queue.lock, NULL);

    for (int i = 3; i <= end; i+=2) {
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
    
    uint64_t count = 1;

    for (int i = 3; i <= end; i += 2) {
        if (!queue.tasks[i].is_composite) {
            if (p==1) {
                timestamps[count] = queue.tasks[i].timestamp;
                threadids[count] = queue.tasks[i].threadid;
            }
            primes[(count)++] = i;
        }
    }

    for (int i = 0; i <= end; i++) {
        pthread_mutex_destroy(&queue.tasks[i].lock);
    }
    free(queue.tasks);
    pthread_mutex_destroy(&queue.lock);

    return count;
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
    if (upper*upper > i && i != lower) {
        if (i%lower == 0) return true;
        if (i == lower+2) return false;
        if (i%(lower+2) == 0) return true;
        if (i == lower+4) return false;
        if (i%(lower+4) == 0) return true;
        if (i == lower+6) return false;
        if (i%(lower+6) == 0) return true;
        if (i == lower+8) return false;
        if (i%(lower+8) == 0) return true;
        if (i == lower+10) return false;
        if (i%(lower+10) == 0) return true;
        if (i == lower+12) return false;
        if (i%(lower+12) == 0) return true;
    }
    return false;
}

bool find_prime_seq(uint64_t i) {
    uint64_t k = 3;
    while (k*k<=i) {
        if (!(i%k)) {
            return true;
        }
        k+=2;
    }
    return false;
}

void* loop_to_y_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    uint64_t local_count = 0;
    uint64_t* local_primes;
    uint64_t* local_times;
    bool heap = false;
    struct timespec ts;

    data->id = malloc(sizeof(uint64_t));
    *(data->id) = pthread_self();
    
    if ((data->end - data->start) * sizeof(uint64_t) < data->stack_size / 2) {
        local_primes = alloca((data->end - data->start) * sizeof(uint64_t));
        if (p==1)
            local_times = alloca((data->end - data->start) * sizeof(uint64_t));
    } else {
        local_primes = malloc((data->end - data->start) * sizeof(uint64_t));
        if (p==1)
            local_times = malloc((data->end - data->start) * sizeof(uint64_t));\
        heap = true;
    }

    if (a==1) {
        for (uint64_t i=data->start; i<data->end; i+=2) {
            if (find_prime_avx(i))
                continue;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            if (p==0) {
                printf("Thread %lu:\t%lu\t(Timestamp: %ld.%2ld seconds)\n", pthread_self(), i, ts.tv_sec, ts.tv_nsec);
            } else if (p==1) {
                local_times[local_count] = ts.tv_sec * 1e9 + ts.tv_nsec;
            }
            local_primes[local_count++] = i;
        }
    } else {
        for (uint64_t i=data->start; i<=data->end; i+=2) {
            if (find_prime_seq(i))
                continue;
            if (p==0) {
                clock_gettime(CLOCK_MONOTONIC, &ts);
                printf("Thread %lu:\t%lu\t(Timestamp: %ld.%2ld seconds)\n", pthread_self(), i, ts.tv_sec, ts.tv_nsec);
            } else if (p==1) {
                local_times[local_count] = ts.tv_sec * 1e9 + ts.tv_nsec;
            }
            local_primes[local_count++] = i;
        }
    }

    pthread_mutex_lock(data->lock);
    for (uint64_t i = 0; i < local_count; i++) {
        data->primes[*(data->count)] = local_primes[i];
        if (p==1) {
            data->threadids[*(data->count)] = *(data->id);
            data->timestamps[*(data->count)] = local_times[i];
        }
        (*(data->count))++;
    }
    pthread_mutex_unlock(data->lock);
    if (heap) {
        if (p==1) {
            free(local_times);
        }
        free(local_primes);
    }

    return NULL;
}

int main() {
    read_config();

    if (y>16000000)
        a = 0;
    if (m==1) {
        workers = malloc(x * sizeof(pthread_t));
        if (!workers) {
            perror("Failed to allocate workers");
            exit(EXIT_FAILURE);
        }
    }

    uint64_t n = 0;
    struct timespec start, end, two;
    double time;

    uint64_t* primes = malloc(y * sizeof(uint64_t));
    uint64_t* timestamps;
    uint64_t* threadids;
    if (p==1)  {
        timestamps = malloc(y * sizeof(uint64_t));
        threadids = malloc(y * sizeof(uint64_t));
    }
    uint64_t chunk_size = (y / x);
    uint64_t count = 0;

    // Get stack memory
    size_t stack_size;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    pthread_attr_getstacksize(&attr, &stack_size);
    pthread_attr_destroy(&attr);

    if (chunk_size < 2) {
        x = y/2 - 1;
        chunk_size = (y / x);
    }
    if (x%2) {
        x--;
        chunk_size = (y / x);
    }

    pthread_t threads[x];
    ThreadData thread_data[x];
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    uint64_t* partitions = malloc(x * sizeof(uint64_t));;

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

    clock_gettime(CLOCK_MONOTONIC, &start);
    if (y>=2) {
        primes[0] = 2;
        if (p==1) {
            clock_gettime(CLOCK_MONOTONIC, &two);
            timestamps[0] = two.tv_sec * 1e9 + two.tv_nsec;
            threadids[0] = pthread_self();
        }
        n++;
        if (y!=2) {
            if (m==0) {
                for (int i = 0; i < x; i++) {
                    thread_data[i].stack_size = stack_size;
                    thread_data[i].start = partitions[i];
                    if (partitions[i+1]==y) {
                        partitions[i+1]++;
                    }
                    thread_data[i].end = partitions[i+1];
                    thread_data[i].primes = primes;
                    if (p==1) {
                        thread_data[i].timestamps = timestamps;
                        thread_data[i].threadids = threadids;
                    }
                    thread_data[i].count = &n;
                    thread_data[i].lock = &lock;
                    thread_data[i].id;

                    pthread_create(&threads[i], NULL, loop_to_y_thread, &thread_data[i]);
                }

                for (int i = 0; i < x; i++) {
                    pthread_join(threads[i], NULL);
                    // printf("Thread %lu\n", *(thread_data[i].id));
                }
            } else {
                n = check_primes(y, primes, timestamps, threadids);
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("\n%lu primes found\n", n);
    if (primes[0] && p==1)
        print_primes(primes, timestamps, threadids, n);
    printf("\n");

    uint64_t elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    double elapsed_s = elapsed_ns / 1e9;
    double start_time = (start.tv_sec * 1e9 + start.tv_nsec) / 1e9;
    double end_time = (end.tv_sec * 1e9 + end.tv_nsec) / 1e9;

    printf("Execution time: %luns (%.2fs)\nStart time: %f seconds\t\tEnd time: %f seconds\n",
            elapsed_ns, elapsed_s, start_time, end_time);

    if (m==1)
        free(workers);
    if (p==1) {
        free(timestamps);
        free(threadids);
    }
    free(primes);
    free(partitions);
    pthread_mutex_destroy(&lock);

    return 0;
}