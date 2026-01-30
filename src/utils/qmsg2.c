/*
 * Copyright (c) 2025 Tan Wah Ken
 *
 * License: The MIT License (MIT)
 */

#include "qmsg2.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "timespec.h"

struct qmsg2_s {
  void **buffer;
  size_t bufsz;
  size_t inserts, removes;
  pthread_mutex_t mutex;
  sem_t empty, full;
};

inline static int _init_qmsg2_s(struct qmsg2_s *ans, size_t sz) {
  unsigned int rwd = sz;
  ans->buffer = (void **)calloc(sz, sizeof(void *));
  if (ans->buffer == NULL) return -1;
  ans->bufsz = sz;
  ans->inserts = 0;
  ans->removes = 0;
  if (pthread_mutex_init(&ans->mutex, NULL)) return -1;
  if (sem_init(&ans->empty, 0, rwd)) return -1;
  if (sem_init(&ans->full, 0, 0)) return -1;
  return 0;
}

inline static void _destroy_qmsg2_s(struct qmsg2_s *ans) {
  free(ans->buffer);
  pthread_mutex_destroy(&ans->mutex);
  sem_destroy(&ans->empty);
  sem_destroy(&ans->full);
}

inline static void _delete_qmsg2_s(struct qmsg2_s *mq) {
  _destroy_qmsg2_s(mq);
  free(mq);
}

inline static struct qmsg2_s *_new_qmsg2_s(size_t sz) {
  struct qmsg2_s *ans = (struct qmsg2_s *)malloc(sizeof(struct qmsg2_s));
  if (ans == NULL) return NULL;
  if (_init_qmsg2_s(ans, sz)) {
    _delete_qmsg2_s(ans);
    return NULL;
  };
  return ans;
}

int qmsg2_init(void) { return 0; }

void qmsg2_deinit(void) { return; }

struct qmsg2_s *qmsg2_new_mq(size_t sz) { return _new_qmsg2_s(sz); }

void qmsg2_del_mq(struct qmsg2_s *mq) {
  if (mq == NULL) return;
  _delete_qmsg2_s(mq);
}

qmsg2_res_t qmsg2_enq(struct qmsg2_s *mq, void *item) {
  if (mq == NULL) return qmsg2_res_error;
  if (sem_wait(&mq->empty)) {
    perror("sem_wait");
    return qmsg2_res_error;
  }
  if (pthread_mutex_lock(&mq->mutex)) {
    perror("pthread_mutex_lock");
    return qmsg2_res_error;
  }

  mq->buffer[mq->inserts++] = item;
  mq->inserts = mq->inserts % mq->bufsz;

  if (pthread_mutex_unlock(&mq->mutex)) {
    perror("pthread_mutex_unlock");
    return qmsg2_res_error;
  }
  if (sem_post(&mq->full)) {
    perror("sem_post");
    return qmsg2_res_error;
  }

  return qmsg2_res_ok;
}

ssize_t qmsg2_count(struct qmsg2_s *mq) {
  size_t ans = 0;
  if (mq == NULL) return -1;
  if (pthread_mutex_lock(&mq->mutex)) {
    perror("pthread_mutex_lock");
    return -1;
  }

  if (mq->inserts >= mq->removes)
    ans = mq->inserts - mq->removes;
  else
    ans = mq->bufsz - mq->removes + mq->inserts;

  if (pthread_mutex_unlock(&mq->mutex)) {
    perror("pthread_mutex_unlock");
    return -1;
  }
  return ans;
}

qmsg2_res_t qmsg2_enq_soft(struct qmsg2_s *mq, void *item) {
  if (mq == NULL) return qmsg2_res_error;
  if (sem_trywait(&mq->empty)) {
    if (errno == EAGAIN) return qmsg2_res_softblocked;
    perror("sem_trywait");
    return qmsg2_res_error;
  }
  if (pthread_mutex_lock(&mq->mutex)) {
    perror("pthread_mutex_lock");
    return qmsg2_res_error;
  }

  mq->buffer[mq->inserts++] = item;
  mq->inserts = mq->inserts % mq->bufsz;

  if (pthread_mutex_unlock(&mq->mutex)) {
    perror("pthread_mutex_unlock");
    return qmsg2_res_error;
  }
  if (sem_post(&mq->full)) {
    perror("sem_post");
    return qmsg2_res_error;
  }

  return qmsg2_res_ok;
}

qmsg2_res_t qmsg2_deq(struct qmsg2_s *mq, void **buf) {
  if (mq == NULL) return qmsg2_res_error;
  if (sem_wait(&mq->full)) {
    perror("sem_wait");
    return qmsg2_res_error;
  }
  if (pthread_mutex_lock(&mq->mutex)) {
    perror("pthread_mutex_lock");
    return qmsg2_res_error;
  }

  *buf = mq->buffer[mq->removes];
  mq->buffer[mq->removes++] = NULL;
  mq->removes = mq->removes % mq->bufsz;

  if (pthread_mutex_unlock(&mq->mutex)) {
    perror("pthread_mutex_unlock");
    return qmsg2_res_error;
  }
  if (sem_post(&mq->empty)) {
    perror("sem_post");
    return qmsg2_res_error;
  }

  return qmsg2_res_ok;
}

qmsg2_res_t qmsg2_poll2_nointr(struct qmsg2_s *mq, void **buf, struct timespec tm) {
  if (mq == NULL) return qmsg2_res_error;
  while (sem_timedwait(&mq->full, &tm)) {
    if (errno == EINTR)
      errno = 0;
    else if (errno == ETIMEDOUT)
      return qmsg2_res_timeout;
    else {
      perror("sem_timedwait");
      return qmsg2_res_error;
    }
  }
  if (pthread_mutex_lock(&mq->mutex)) {
    perror("pthread_mutex_lock");
    return qmsg2_res_error;
  }

  *buf = mq->buffer[mq->removes];
  mq->buffer[mq->removes++] = NULL;
  mq->removes = mq->removes % mq->bufsz;

  if (pthread_mutex_unlock(&mq->mutex)) {
    perror("pthread_mutex_unlock");
    return qmsg2_res_error;
  }
  if (sem_post(&mq->empty)) {
    perror("sem_post");
    return qmsg2_res_error;
  }

  return qmsg2_res_ok;
}

qmsg2_res_t qmsg2_poll2(struct qmsg2_s *mq, void **buf, struct timespec tm) {
  if (mq == NULL) return qmsg2_res_error;
  if (sem_timedwait(&mq->full, &tm)) {
    if (errno == ETIMEDOUT) return qmsg2_res_timeout;
    if (errno == EINTR) return qmsg2_res_interrupt;
    perror("sem_timedwait");
    return qmsg2_res_error;
  }
  if (pthread_mutex_lock(&mq->mutex)) {
    perror("pthread_mutex_lock");
    return qmsg2_res_error;
  }

  *buf = mq->buffer[mq->removes];
  mq->buffer[mq->removes++] = NULL;
  mq->removes = mq->removes % mq->bufsz;

  if (pthread_mutex_unlock(&mq->mutex)) {
    perror("pthread_mutex_unlock");
    return qmsg2_res_error;
  }
  if (sem_post(&mq->empty)) {
    perror("sem_post");
    return qmsg2_res_error;
  }

  return qmsg2_res_ok;
}

qmsg2_res_t qmsg2_poll(struct qmsg2_s *mq, void **buf, time_t tv_sec, long tv_nsec) {
  struct timespec tm;
  if (clock_gettime(CLOCK_REALTIME, &tm) == -1) {
    perror("clock_gettime");
    return qmsg2_res_error;
  }
  tm = timespec_add(tm, (struct timespec){.tv_sec = tv_sec, .tv_nsec = tv_nsec});
  return qmsg2_poll2(mq, buf, tm);
}

qmsg2_res_t qmsg2_poll_nointr(struct qmsg2_s *mq, void **buf, time_t tv_sec, long tv_nsec) {
  struct timespec tm;
  if (clock_gettime(CLOCK_REALTIME, &tm) == -1) {
    perror("clock_gettime");
    return qmsg2_res_error;
  }
  tm = timespec_add(tm, (struct timespec){.tv_sec = tv_sec, .tv_nsec = tv_nsec});
  return qmsg2_poll2_nointr(mq, buf, tm);
}

int qmsg2_flush(struct qmsg2_s *mq, typeof(void(void *)) destroyer) {
  if (mq == NULL) return -1;
  if (pthread_mutex_lock(&mq->mutex)) {
    perror("pthread_mutex_lock");
    return -1;
  }

  for (size_t i = 0; i < mq->bufsz; i++) {
    if (mq->buffer[i] == NULL) continue;
    destroyer(mq->buffer[i]);
    mq->buffer[i] = NULL;
  }
  mq->inserts = mq->removes = 0;

  if (pthread_mutex_unlock(&mq->mutex)) {
    perror("pthread_mutex_unlock");
    return -1;
  }

  return 0;
}
