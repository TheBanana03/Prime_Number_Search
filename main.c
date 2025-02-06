#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>

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

void print_primes(uint64_t* primes, uint64_t y) {
    printf("\n%lu primes found", y);
    printf("\n%lu", primes[0]);
    for (uint64_t i=1; i<y; i++) {
        // printf(", %lu", primes[i]);
    }
    printf("\n");
}

bool find_prime_avx(uint64_t i) {
    uint32_t lower = 3;
    uint32_t upper = lower + (2*7);
    int u, l;
    while (upper*upper<=i) {
        float v_256[8] = {lower, lower+2, lower+4, lower+6, lower+8, lower+10, lower+12, upper};
        // printf("%d: ", i);

        // uint32_t result[8];
        // float result[8];
        bool result;

        find_prime(i, &result, v_256);
        // if (i == 361) {
            // printf("%d: ", i);
            // for (int j = 0; j < 8; j++) {
                // printf("%d, ", result[j]);
                // printf("%f, ", result[j]);
            // }
            // printf("\n");
            // printf("%d\n", result);
        // }
        if (!result)
            return true;
        lower+=16;
        upper+=16;
    }
    if (upper*upper>i) {
        // printf("%d, ", i);
        while (lower*lower<=i) {
            if (!(i%lower))
                return true;
            lower+=2;
        }
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
    for (uint64_t i=3; i<y; i+=2) {
        // if (find_prime_avx(i))
        if (find_prime_avx(i))
            continue;
        primes[n] = i;
        n++;
    }
    return n;
}

void* loop_to_y_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    uint64_t local_count = 0;
    uint64_t* local_primes;

    data->id = malloc(sizeof(uint64_t));
    *(data->id) = pthread_self();
    
    if ((data->end - data->start) * sizeof(uint64_t) < data->stack_size / 2) {
        local_primes = alloca((data->end - data->start) * sizeof(uint64_t));
    } else {
        local_primes = malloc((data->end - data->start) * sizeof(uint64_t));
    }

    // printf("Thread ID: %lu\tStart: %lu\tEnd: %lu\n", pthread_self(), data->start, data->end);

    for (uint64_t i=data->start; i<data->end; i+=2) {
        if (find_prime_seq(i))
            continue;
        // printf("Thread ID: %lu\tNumber: %lu\n", pthread_self(), i);
        local_primes[local_count++] = i;
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

    return NULL;
}

int main() {
    uint64_t x = 1;
    uint64_t y = 10000000;
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
            n = loop_to_y_seq(primes, y);
            // for (int i = 0; i < x; i++) {
            //     thread_data[i].stack_size = stack_size;
            //     thread_data[i].start = partitions[i];
            //     thread_data[i].end = partitions[i+1];
            //     thread_data[i].primes = primes;
            //     thread_data[i].count = &n;
            //     thread_data[i].lock = &lock;
            //     thread_data[i].id;

            //     pthread_create(&threads[i], NULL, loop_to_y_thread, &thread_data[i]);
            // }
        }
    }
    
    // for (int i = 0; i < x; i++) {
    //     pthread_join(threads[i], NULL);
    // }

    end = clock();
    
    if (primes[0])
        print_primes(primes, n);
    printf("\n");

    if (primes[0])
        // print_primes(primes, n);

    printf("Execution time: %f\n", ((end - start)) * 1.0 / CLOCKS_PER_SEC);

    return 0;
}