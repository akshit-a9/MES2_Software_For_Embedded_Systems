#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <linux/futex.h>

#define NUM_READERS 7
#define NUM_WRITERS 2
#define NUM_ITERATIONS 2

static void fwait(uint32_t *futexp, const char *thread_name);
static void fpost(uint32_t *futexp, const char *thread_name);
void *writer(void *arg);
void *reader(void *arg);

static uint32_t *writer_futex, *reader_futex;
int readers = 0;

int main(int argc, char *argv[]) {
    pthread_t writer_thread[NUM_WRITERS];
    pthread_t reader_thread[NUM_READERS];
    int reader_id[NUM_READERS];
    int writer_id[NUM_WRITERS];

    uint32_t reader_futex_val = 1;
    uint32_t writer_futex_val = 1;

    reader_futex = &reader_futex_val;
    writer_futex = &writer_futex_val;

    setbuf(stdout, NULL);

    for(int i = 0; i < NUM_READERS; i++) {
        reader_id[i] = i;
        pthread_create(&reader_thread[i], NULL, reader, &reader_id[i]);
    }

    for(int i = 0; i < NUM_WRITERS; i++) {
        writer_id[i] = i;
        pthread_create(&writer_thread[i], NULL, writer, &writer_id[i]);
    }

    for(int i = 0; i < NUM_READERS; i++) {
        pthread_join(reader_thread[i], NULL);
    }
    for(int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writer_thread[i], NULL);
    }

    printf("All threads completed\n");

    return 0;
}

void *writer(void *arg) {
    int *id = (int *) arg;
    char thread_name[32];
    snprintf(thread_name, sizeof(thread_name), "Writer %d", *id);
    
    for(int i = 0; i < NUM_ITERATIONS; i++) {
        fwait(writer_futex, thread_name);

        printf("[Writer %d] is writing\n", *id);
        usleep(10000 * (10 + (rand() % 40)));

        printf("[Writer %d] done writing\n", *id);
        fpost(writer_futex, thread_name);
        usleep(10000 * (rand() % 100));
    }

    return NULL;
}

void *reader(void *arg) {
    int *id = (int *) arg;
    char thread_name[32];
    snprintf(thread_name, sizeof(thread_name), "Reader %d", *id);
    
    for(int i = 0; i < NUM_ITERATIONS; i++) {
        fwait(reader_futex, thread_name);
        __atomic_fetch_add(&readers, 1, __ATOMIC_SEQ_CST);
        if(readers == 1) {
            fwait(writer_futex, thread_name);
        }
        fpost(reader_futex, thread_name);

        printf("[Reader %d] is reading\n", *id);
        usleep(10000 * (10 + (rand() % 40)));

        fwait(reader_futex, thread_name);
        __atomic_fetch_sub(&readers, 1, __ATOMIC_SEQ_CST);
        if(readers == 0) {
            fpost(writer_futex, thread_name);
        }
        printf("[Reader %d] done reading\n", *id);
        fpost(reader_futex, thread_name);
        usleep(10000 * (rand() % 100));
    }

    return NULL;
}

static int futex(uint32_t *uaddr, int futex_op, uint32_t val, const struct timespec *timeout, uint32_t *uaddr2, uint32_t val3) {
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

static void fwait(uint32_t *futexp, const char *thread_name) {
    long ret;
    const uint32_t one = 1;

    printf("  [%s] requesting futex\n", thread_name);
    
    while(1) {
        if(atomic_compare_exchange_strong(futexp, &one, 0)) {
            printf("  [%s] acquired futex\n", thread_name);
            break;
        }

        printf("  [%s] blocked, waiting on futex\n", thread_name);
        ret = futex(futexp, FUTEX_WAIT, 0, NULL, NULL, 0);
        if(ret == -1 && errno != EAGAIN) {
            err(EXIT_FAILURE, "futex-FUTEX_WAIT");
        }
        printf("  [%s] woke up, retrying\n", thread_name);
    }
}

static void fpost(uint32_t *futexp, const char *thread_name) {
    long ret;
    const uint32_t zero = 0;

    printf("  [%s] releasing futex\n", thread_name);
    
    if (atomic_compare_exchange_strong(futexp, &zero, 1)) {
        ret = futex(futexp, FUTEX_WAKE, 1, NULL, NULL, 0);
        if (ret == -1) {
            err(EXIT_FAILURE, "futex-FUTEX_WAKE");
        }
        if (ret > 0) {
            printf("  [%s] woke up %ld waiting thread(s)\n", thread_name, ret);
        }
    }
        
}
