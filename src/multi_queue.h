#ifndef __MULTI_QUEUE_H__
#define __MULTI_QUEUE_H__

#include <cassert>

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

//#define dbg_printf(args...) printf(args)
#define dbg_printf(args...)

typedef struct {
  void **elems;
  int num_elems;      // incoming elements
  int busy_mask;     // bitmaks of elements currently in
  pthread_mutex_t mtx;
  pthread_cond_t cv_ready;
  pthread_cond_t *cv_busy;
  int *waiting;
} multi_queue_t;

static int multi_queue_init(multi_queue_t *that, int num_elems) {
  that->elems = (void**)malloc(sizeof(*that->elems) * num_elems);
  if (that->elems == NULL)
    goto oom1;
  that->cv_busy = (pthread_cond_t *)malloc(sizeof(*that->cv_busy) * num_elems);
  if (that->cv_busy == NULL)
    goto oom2;
  that->waiting = (int *)malloc(sizeof(*that->waiting) * num_elems);
  if (that->waiting == NULL)
    goto oom3;
  that->num_elems = num_elems;
  that->busy_mask = 0;
  pthread_mutex_init(&that->mtx, NULL);
  pthread_cond_init(&that->cv_ready, NULL);
  for (int i = 0; i < num_elems; i++) {
    pthread_cond_init(&that->cv_busy[i], NULL);
    that->waiting[i] = 0;
  }
  return 1;

 oom3:
  free(that->cv_busy);
 oom2:
  free(that->elems);
 oom1:
  return 0;
}

// may block if the i-th elem is busy
// returns 1 if all elems have been pushed as input to target (so it has been notified)
static int multi_queue_push(multi_queue_t *that, int i, void *elem) {
  int rv = 0;
  pthread_mutex_lock(&that->mtx);
  while (that->busy_mask & (1 << i)) {
    that->waiting[i]++;
    dbg_printf("push() suspending...\n");
    pthread_cond_wait(&that->cv_busy[i], &that->mtx);
    dbg_printf("push() woken up...\n");
    that->waiting[i]--;
  }
  that->elems[i] = elem;
  that->busy_mask |= (1 << i);
  if (that->busy_mask == (1 << that->num_elems) - 1) {
    pthread_cond_signal(&that->cv_ready);
    rv = 1;
  }
  pthread_mutex_unlock(&that->mtx);
  return rv;
}

// only unblock once all size elems have been popped
static void multi_queue_pop(multi_queue_t *that, void **elems, int num_elems) {
  assert(num_elems == that->num_elems);
  pthread_mutex_lock(&that->mtx);
  while (that->busy_mask != (1 << that->num_elems) - 1) {
    dbg_printf("pop() suspending (busy_mask=%x)...\n", that->busy_mask);
    pthread_cond_wait(&that->cv_ready, &that->mtx);
    dbg_printf("pop() woken up...\n");
  }
  if (elems != NULL)
    memcpy(elems, that->elems, sizeof(*elems) * num_elems);
  that->busy_mask = 0;
  for (int i = 0; i < num_elems; i++)
    if (that->waiting[i] > 0)
      pthread_cond_signal(&that->cv_busy[i]);
  pthread_mutex_unlock(&that->mtx);
}

static void multi_queue_cleanup(multi_queue_t *that) {
  free(that->waiting);
  free(that->cv_busy);
  free(that->elems);
}

#ifdef __cplusplus
}
#endif

#endif // __MULTI_QUEUE_H__
