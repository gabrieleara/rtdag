#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include "multi_queue.h"

#define N 3
pthread_t threads[N];
void *values[N];
multi_queue_t mq;

void *thread_body(void *arg) {
  long i = (long)arg;
  printf("pushing %ld\n", i);
  multi_queue_push(&mq, i, (void*)i);
  printf("pushing %ld\n", 10+i);
  multi_queue_push(&mq, i, (void*)(10+i));
  printf("terminating %ld\n", i);
  return 0;
}

int main(int argc, char **argv) {
  printf("Initializing...\n");
  multi_queue_init(&mq, N);
  for (int i = 0; i < N; i++)
    assert(pthread_create(&threads[i], NULL, thread_body, (void*)(long)i) == 0);

  printf("Receiving...\n");
  multi_queue_pop(&mq, values, N);
  printf("Received: [");
  for (int i = 0; i < N; i++)
    printf("%ld%s", (long)values[i], i < N - 1 ? "," : "");
  printf("]\n");

  printf("Receiving...\n");
  multi_queue_pop(&mq, values, N);
  printf("Received: [");
  for (int i = 0; i < N; i++)
    printf("%ld%s", (long)values[i], i < N - 1 ? "," : "");
  printf("]\n");

  for (int i = 0; i < N; i++)
    pthread_join(threads[i], NULL);

  printf("Cleaning up...\n");
  multi_queue_cleanup(&mq);

  return 0;
}
