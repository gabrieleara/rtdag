#ifndef __MULTI_QUEUE_H__
#define __MULTI_QUEUE_H__

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

static int multi_queue_init(multi_queue_t *this, int num_elems) {
  this->elems = malloc(sizeof(*this->elems) * num_elems);
  if (this->elems == NULL)
    goto oom1;
  this->cv_busy = malloc(sizeof(*this->cv_busy) * num_elems);
  if (this->cv_busy == NULL)
    goto oom2;
  this->waiting = malloc(sizeof(*this->waiting) * num_elems);
  if (this->waiting == NULL)
    goto oom3;
  this->num_elems = num_elems;
  this->busy_mask = 0;
  pthread_mutex_init(&this->mtx, NULL);
  pthread_cond_init(&this->cv_ready, NULL);
  for (int i = 0; i < num_elems; i++) {
    pthread_cond_init(&this->cv_busy[i], NULL);
    this->waiting[i] = 0;
  }
  return 1;

 oom3:
  free(this->cv_busy);
 oom2:
  free(this->elems);
 oom1:
  return 0;
}

// may block if the i-th elem is busy
static void multi_queue_push(multi_queue_t *this, int i, void *elem) {
  pthread_mutex_lock(&this->mtx);
  while (this->busy_mask & (1 << i)) {
    this->waiting[i]++;
    dbg_printf("push() suspending...\n");
    pthread_cond_wait(&this->cv_busy[i], &this->mtx);
    dbg_printf("push() woken up...\n");
    this->waiting[i]--;
  }
  this->elems[i] = elem;
  this->busy_mask |= (1 << i);
  if (this->busy_mask == (1 << this->num_elems) - 1)
    pthread_cond_signal(&this->cv_ready);
  pthread_mutex_unlock(&this->mtx);
}

// only unblock once all size elems have been popped
static void multi_queue_pop(multi_queue_t *this, void **elems, int num_elems) {
  assert(num_elems == this->num_elems);
  pthread_mutex_lock(&this->mtx);
  while (this->busy_mask != (1 << this->num_elems) - 1) {
    dbg_printf("pop() suspending (busy_mask=%x)...\n", this->busy_mask);
    pthread_cond_wait(&this->cv_ready, &this->mtx);
    dbg_printf("pop() woken up...\n");
  }
  memcpy(elems, this->elems, sizeof(*elems) * num_elems);
  this->busy_mask = 0;
  for (int i = 0; i < num_elems; i++)
    if (this->waiting[i] > 0)
      pthread_cond_signal(&this->cv_busy[i]);
  pthread_mutex_unlock(&this->mtx);
}

static void multi_queue_cleanup(multi_queue_t *this) {
  free(this->waiting);
  free(this->cv_busy);
  free(this->elems);
}

#ifdef __cplusplus
}
#endif

#endif // __MULTI_QUEUE_H__
