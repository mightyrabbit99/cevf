/*
 * Copyright (c) 2025 Tan Wah Ken
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of my_library.
 *
 * Author:          mightyrabbit99 <t.1999ken@gmail.com>
 * Version:         v1.0.0
 */

#include "cevf.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>

#include "cevf_ev.h"
#include "cevf_main_pre.h"
#include "cvector.h"
#include "cvector_utils.h"
#include "eloop/eloop.h"
#include "heap.h"
#include "map.h"
#include "qmsg2.h"
#include "timespec.h"

/////////////////////////////////////
#define lge(...) fprintf(stderr, __VA_ARGS__)

#define CEVF_DEFAULT_CTRL_MQ_SZ 10
#define CEVF_DEFAULT_DATA_MQ_SZ 10
#define CEVF_DEFAULT_CM_THR_CNT 2
#define CEVF_TIMEOUT_HP_SZ 10
#define CEVF_CONSUMER_STACKSIZE 0

static inline cevf_qmsg2_res_t conv_qmsgres_res2(qmsg2_res_t typ) {
  switch (typ) {
    case qmsg2_res_ok:
      return cevf_qmsg2_res_ok;
    case qmsg2_res_timeout:
      return cevf_qmsg2_res_timeout;
    case qmsg2_res_softblocked:
      return cevf_qmsg2_res_softblocked;
    case qmsg2_res_interrupt:
      return cevf_qmsg2_res_interrupt;
    case qmsg2_res_error:
      return cevf_qmsg2_res_error;
    default:
      return cevf_qmsg2_res_error;
  }
}

static pthread_mutex_t log_mutex;
static cevf_log_level_t log_level = cevf_log_level_error;
static FILE *log_stream;

void cevf_log_set(cevf_log_level_t level, FILE *stream) {
  pthread_mutex_lock(&log_mutex);
  log_level = level;
  log_stream = stream;
  pthread_mutex_unlock(&log_mutex);
}

///////////////////////////////

static struct qmsg2_s *data_mq = NULL;

struct _data_s {
  cevf_evtyp_t evtyp;
  size_t datalen;
  uint8_t *data;
  uint8_t need_freed;
};

static inline struct _data_s *new_data_s(cevf_evtyp_t evtyp, uint8_t *data, size_t datalen) {
  struct _data_s *ans = (struct _data_s *)malloc(sizeof(struct _data_s));
  if (ans == NULL) return NULL;
  ans->evtyp = evtyp;
  ans->data = data;
  ans->datalen = datalen;
  ans->need_freed = 0;
  return ans;
}

static inline uint8_t *_copy_data(struct _data_s *d, const uint8_t *data, size_t datalen) {
  d->data = (uint8_t *)malloc(datalen);
  if (d->data == NULL) return NULL;
  memcpy(d->data, data, datalen);
  d->need_freed = 1;
  return d->data;
}

static inline void delete_data_s(struct _data_s *d) {
  if (d == NULL) return;
  free(d);
}

static CEVF_EV_THFDECL(cevf_generic_consumer_loopf, arg1);

static cevf_qmsg2_res_t _cevf_evq_enqueue(struct _data_s *d, cevf_evtyp_t evtyp) {
  qmsg2_res_t ret;
  if (data_mq == NULL) return cevf_qmsg2_res_error;
  if (ev_is_running(CEVF_EV_THFNAME(cevf_generic_consumer_loopf))) {
    ret = qmsg2_enq(data_mq, (void *)d);
  } else {
    ret = qmsg2_enq_soft(data_mq, (void *)d);
  }

  return conv_qmsgres_res2(ret);
}

static cevf_qmsg2_res_t _cevf_evq_enqueue_soft(struct _data_s *d, cevf_evtyp_t evtyp) {
  qmsg2_res_t ret;
  if (data_mq == NULL) return cevf_qmsg2_res_error;
  ret = qmsg2_enq_soft(data_mq, (void *)d);
  return conv_qmsgres_res2(ret);
}

static inline cevf_qmsg2_res_t _cevf_generic_enq(void *data, cevf_evtyp_t evtyp, cevf_qmsg2_res_t (*f)(struct _data_s *d, cevf_evtyp_t evtyp)) {
  struct _data_s *d = new_data_s(evtyp, data, 0);
  if (d == NULL) return cevf_qmsg2_res_error;
  cevf_qmsg2_res_t res = f(d, evtyp);
  if (res != cevf_qmsg2_res_ok) delete_data_s(d);
  return conv_qmsgres_res2(res);
}

cevf_qmsg2_res_t cevf_generic_enqueue(void *data, cevf_evtyp_t evtyp) {
  if (evtyp == CEVF_RESERVED_EV_THEND) return cevf_qmsg2_res_error;
  return _cevf_generic_enq(data, evtyp, _cevf_evq_enqueue);
}

cevf_qmsg2_res_t cevf_generic_enqueue_soft(void *data, cevf_evtyp_t evtyp) {
  if (evtyp == CEVF_RESERVED_EV_THEND) return cevf_qmsg2_res_error;
  return _cevf_generic_enq(data, evtyp, _cevf_evq_enqueue_soft);
}

static inline cevf_qmsg2_res_t _cevf_copy_enq(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp, cevf_qmsg2_res_t (*f)(struct _data_s *d, cevf_evtyp_t evtyp)) {
  struct _data_s *d = NULL;
  uint8_t *data2 = NULL;
  cevf_qmsg2_res_t res = cevf_qmsg2_res_error;
  d = new_data_s(evtyp, NULL, 0);
  if (d == NULL) goto fail;
  data2 = _copy_data(d, data, datalen);
  if (data2 == NULL) goto fail;
  res = _cevf_evq_enqueue(d, evtyp);
  if (res != cevf_qmsg2_res_ok) goto fail;
  return conv_qmsgres_res2(res);
fail:
  if (d) delete_data_s(d);
  if (data2) free(data2);
  return res;
}

cevf_qmsg2_res_t cevf_copy_enqueue(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  if (evtyp == CEVF_RESERVED_EV_THEND) return cevf_qmsg2_res_error;
  return _cevf_copy_enq(data, datalen, evtyp, _cevf_evq_enqueue);
}

cevf_qmsg2_res_t cevf_copy_enqueue_soft(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  if (evtyp == CEVF_RESERVED_EV_THEND) return cevf_qmsg2_res_error;
  return _cevf_copy_enq(data, datalen, evtyp, _cevf_evq_enqueue_soft);
}

static ssize_t cevf_generic_dequeue(uint8_t **data, cevf_evtyp_t *evtyp, uint8_t *need_freed) {
  size_t datalen;
  void *msg = NULL;
  if (data_mq == NULL) return -1;
  qmsg2_deq(data_mq, &msg);
  if (msg == NULL) return -1;
  struct _data_s *d = (struct _data_s *)msg;
  *evtyp = d->evtyp;
  *data = d->data;
  *need_freed = d->need_freed;
  datalen = d->datalen;
  delete_data_s(d);
  return datalen;
}

static int cevf_generic_enqueue_1(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp, void *context) {
  return cevf_generic_enqueue(data, evtyp);
}

struct _handle_ev_farr_s {
  cevf_consumer_typ_t typ;
  void *handler_arr;
};

#define _cevf_exec_f(ftyp, farr, res, evtyp, ...) \
  do {                                            \
    ftyp *f__;                                    \
    if (cvector_empty(farr)) {                    \
      if (evtyp != CEVF_RESERVED_EV_THEND) {      \
        lge("No handler for evtyp=%d!\n", evtyp); \
      }                                           \
    }                                             \
    cvector_for_each_in(f__, farr) {              \
      res = (*f__)(__VA_ARGS__);                  \
    }                                             \
  } while (0)

struct _cevf_consumer_fbox_s {
  cvector_vector_type(cevf_consumer_handler_t1_t) handler_t1_arr;
  cvector_vector_type(cevf_consumer_handler_t2_t) handler_t2_arr;
};

static inline int _handle_event(hashmap *m_evtyp_handler, uint8_t *data, size_t datalen, cevf_evtyp_t evtyp, cevf_evtyp_t evtyp2, uint8_t need_freed) {
  int res = 0;
  struct _cevf_consumer_fbox_s *fbox;
  if (!hashmap_get(m_evtyp_handler, &evtyp, sizeof(evtyp), (uintptr_t *)&fbox)) {
    if (evtyp != CEVF_RESERVED_EV_THEND) {
      lge("No handler for evtyp=%d!\n", evtyp);
    }
    return 0;
  }
  if (need_freed == 0) {
    _cevf_exec_f(cevf_consumer_handler_t1_t, fbox->handler_t1_arr, res, evtyp, data, evtyp2);
  } else {
    _cevf_exec_f(cevf_consumer_handler_t2_t, fbox->handler_t2_arr, res, evtyp, data, datalen, evtyp2);
  }
  return res;
}

static void _data_msg_free(void *d) {
  if (d == NULL) return;
  struct _data_s *dd = (struct _data_s *)d;
  if (dd->need_freed) free(dd->data);
  delete_data_s(dd);
}

static CEVF_EV_THENDDECL(cevf_generic_consumer_loopf, arg1) {
  _cevf_generic_enq(NULL, CEVF_RESERVED_EV_THEND, _cevf_evq_enqueue);
}

static CEVF_EV_THFDECL(cevf_generic_consumer_loopf, arg1) {
  int ret = 0;
  uint8_t *data;
  ssize_t datalen;
  uint8_t need_freed = 0;
  cevf_evtyp_t evtyp;
  for (;;) {
    datalen = cevf_generic_dequeue(&data, &evtyp, &need_freed);
    if (-1 == datalen) return NULL;
    if (evtyp == CEVF_RESERVED_EV_THEND) break;
    hashmap *m_evtyp_handler = (hashmap *)ev_getcontext(arg1);
    ret = _handle_event(m_evtyp_handler, data, datalen, evtyp, evtyp, need_freed);
    // global event handler
    ret = _handle_event(m_evtyp_handler, data, datalen, CEVF_RESERVED_EV_THEND, evtyp, need_freed);
    if (need_freed) free(data);
    need_freed = 0;
  }
  if (need_freed) free(data);
  ev_setret(arg1, ret);
  return NULL;
}

#define _compile_handler(m_evtyp_handler, fbox, fbox_arr_name, cm_arr, cm_num)                            \
  do {                                                                                                    \
    for (cevf_asz_t i = 0; i < cm_num; i++) {                                                             \
      if (!hashmap_get(m_evtyp_handler, &cm_arr[i].evtyp, sizeof(cm_arr[i].evtyp), (uintptr_t *)&fbox)) { \
        fbox = (struct _cevf_consumer_fbox_s *)malloc(sizeof(struct _cevf_consumer_fbox_s));              \
        *fbox = (struct _cevf_consumer_fbox_s){0};                                                        \
        hashmap_set(m_evtyp_handler, &cm_arr[i].evtyp, sizeof(cm_arr[i].evtyp), (uintptr_t)fbox);         \
      }                                                                                                   \
      cvector_push_back(fbox->fbox_arr_name, cm_arr[i].handler);                                          \
    }                                                                                                     \
  } while (0)

static hashmap *compile_m_evtyp_handler(struct cevf_consumer_t1_s *cm_t1_arr, cevf_asz_t cm_t1_num, struct cevf_consumer_t2_s *cm_t2_arr, cevf_asz_t cm_t2_num) {
  hashmap *m_evtyp_handler = hashmap_create();
  if (m_evtyp_handler == NULL) return NULL;
  cevf_asz_t i;
  struct _cevf_consumer_fbox_s *fbox;
  _compile_handler(m_evtyp_handler, fbox, handler_t1_arr, cm_t1_arr, cm_t1_num);
  _compile_handler(m_evtyp_handler, fbox, handler_t2_arr, cm_t2_arr, cm_t2_num);
  return m_evtyp_handler;
}

static void _delete_m_evtyp_handler_it(void *key, size_t ksize, uintptr_t value, void *usr) {
  struct _cevf_consumer_fbox_s *fbox = (struct _cevf_consumer_fbox_s *)value;
  cvector_free(fbox->handler_t1_arr);
  cvector_free(fbox->handler_t2_arr);
  free(fbox);
}

static void delete_m_evtyp_handler(hashmap *m_evtyp_handler) {
  if (m_evtyp_handler == NULL) return;
  hashmap_iterate(m_evtyp_handler, _delete_m_evtyp_handler_it, NULL);
  hashmap_free(m_evtyp_handler);
}

enum thtype_e {
  thtyp_producer,
  thtyp_consumer,
};

cevf_producer_id_t _cevf_add_producer(struct cevf_producer_s pd) {
  return (cevf_producer_id_t)ev_add_th((struct thprop_s){
      .thstart = pd.thstart,
      .stack_size = pd.stack_size,
      .thtyp = thtyp_producer,
      .evtyp = -1,  // not being used
      .terminator = pd.terminator,
      .context = pd.context,
  });
}

int cevf_rm_producer(cevf_producer_id_t id) {
  return ev_rm_th((ev_th_id_t)id);
}

static hashmap *m_pcdno_handler_t1 = NULL;
static pthread_mutex_t m_pcdno_handler_t1_mutex;
static hashmap *m_pcdno_handler_t2 = NULL;
static pthread_mutex_t m_pcdno_handler_t2_mutex;

struct _procd_t1_s {
  struct cevf_procedure_t1_s pcd;
  pthread_mutex_t mut;
};

static inline void delete_procd_t1_s(struct _procd_t1_s *s) {
  pthread_mutex_destroy(&s->mut);
  free(s);
}

static inline struct _procd_t1_s *new_procd_t1_s(struct cevf_procedure_t1_s pcd) {
  struct _procd_t1_s *ans = (struct _procd_t1_s *)malloc(sizeof(struct _procd_t1_s));
  if (ans == NULL) return NULL;
  ans->pcd = pcd;
  pthread_mutex_init(&ans->mut, NULL);
  return ans;
}

struct _procd_t2_s {
  struct cevf_procedure_t2_s pcd;
  pthread_mutex_t mut;
};

static inline void delete_procd_t2_s(struct _procd_t2_s *s) {
  pthread_mutex_destroy(&s->mut);
  free(s);
}

static inline struct _procd_t2_s *new_procd_t2_s(struct cevf_procedure_t2_s pcd) {
  struct _procd_t2_s *ans = (struct _procd_t2_s *)malloc(sizeof(struct _procd_t2_s));
  if (ans == NULL) return NULL;
  ans->pcd = pcd;
  pthread_mutex_init(&ans->mut, NULL);
  return ans;
}

static void _deinit_m_pcdno_handler_t1_it(void *key, size_t ksize, uintptr_t value, void *usr) {
  struct _procd_t1_s *s = (struct _procd_t1_s *)value;
  delete_procd_t1_s(s);
}

static void _deinit_m_pcdno_handler_t2_it(void *key, size_t ksize, uintptr_t value, void *usr) {
  struct _procd_t2_s *s = (struct _procd_t2_s *)value;
  delete_procd_t2_s(s);
}

static inline void _deinit_m_pcdno_handler(hashmap *m_pcdno_handler, hashmap_callback destroy_f) {
  if (m_pcdno_handler == NULL) return;
  hashmap_iterate(m_pcdno_handler, destroy_f, NULL);
  hashmap_free(m_pcdno_handler);
}

static inline void cevf_init_m_pcdno_handler(void) {
  m_pcdno_handler_t1 = hashmap_create();
  pthread_mutex_init(&m_pcdno_handler_t1_mutex, NULL);
  m_pcdno_handler_t2 = hashmap_create();
  pthread_mutex_init(&m_pcdno_handler_t2_mutex, NULL);
}

static inline void cevf_deinit_m_pcdno_handler(void) {
  _deinit_m_pcdno_handler(m_pcdno_handler_t1, _deinit_m_pcdno_handler_t1_it);
  pthread_mutex_destroy(&m_pcdno_handler_t1_mutex);
  _deinit_m_pcdno_handler(m_pcdno_handler_t2, _deinit_m_pcdno_handler_t2_it);
  pthread_mutex_destroy(&m_pcdno_handler_t2_mutex);
}

void *_cevf_exec_procedure_t1(cevf_pcdno_t pcdno, int argc, ...) {
  struct _procd_t1_s *s;
  if (m_pcdno_handler_t1 == NULL) return NULL;
  pthread_mutex_lock(&m_pcdno_handler_t1_mutex);
  if (!hashmap_get(m_pcdno_handler_t1, &pcdno, sizeof(pcdno), (uintptr_t *)&s)) {
    pthread_mutex_unlock(&m_pcdno_handler_t1_mutex);
    lge("No procedure for pcdno=%d!\n", pcdno);
    return NULL;
  }
  pthread_mutex_unlock(&m_pcdno_handler_t1_mutex);

  va_list argp;
  va_start(argp, argc);
  pthread_mutex_lock(&s->mut);
  void *ans = s->pcd.vfunc(argc, argp);
  pthread_mutex_unlock(&s->mut);
  va_end(argp);

  return ans;
}

uint8_t *cevf_exec_procedure_t2(cevf_pcdno_t pcdno, const uint8_t *input, size_t input_len, size_t *output_len) {
  struct _procd_t2_s *s;
  if (m_pcdno_handler_t2 == NULL) return NULL;
  pthread_mutex_lock(&m_pcdno_handler_t2_mutex);
  if (!hashmap_get(m_pcdno_handler_t2, &pcdno, sizeof(pcdno), (uintptr_t *)&s)) {
    pthread_mutex_unlock(&m_pcdno_handler_t2_mutex);
    lge("No procedure for pcdno=%d!\n", pcdno);
    return NULL;
  }
  pthread_mutex_unlock(&m_pcdno_handler_t2_mutex);
  
  pthread_mutex_lock(&s->mut);
  uint8_t *ans = s->pcd.func(input, input_len, output_len, s->pcd.ctx);
  pthread_mutex_unlock(&s->mut);

  return ans;
}

int cevf_add_procedure_t1(struct cevf_procedure_t1_s pcd) {
  struct _procd_t1_s *p = NULL, *p2;
  if (m_pcdno_handler_t1 == NULL) return -1;
  pthread_mutex_lock(&m_pcdno_handler_t1_mutex);
  if (hashmap_get(m_pcdno_handler_t1, &pcd.pcdno, sizeof(pcd.pcdno), (uintptr_t *)&p)) {
    pthread_mutex_unlock(&m_pcdno_handler_t1_mutex);
  } else {
    pthread_mutex_unlock(&m_pcdno_handler_t1_mutex);
  }

  p2 = new_procd_t1_s(pcd);
  if (p2 == NULL) return -1;
  if (p) delete_procd_t1_s(p);

  pthread_mutex_lock(&m_pcdno_handler_t1_mutex);
  hashmap_set(m_pcdno_handler_t1, &pcd.pcdno, sizeof(pcd.pcdno), (uintptr_t)p2);
  pthread_mutex_unlock(&m_pcdno_handler_t1_mutex);
  return 0;
}

int cevf_add_procedure_t2(struct cevf_procedure_t2_s pcd) {
  struct _procd_t2_s *p, *p2 = NULL;
  if (m_pcdno_handler_t2 == NULL) return -1;
  pthread_mutex_lock(&m_pcdno_handler_t2_mutex);
  if (hashmap_get(m_pcdno_handler_t2, &pcd.pcdno, sizeof(pcd.pcdno), (uintptr_t *)&p)) {
    pthread_mutex_unlock(&m_pcdno_handler_t2_mutex);
  } else {
    pthread_mutex_unlock(&m_pcdno_handler_t2_mutex);
  }

  p2 = new_procd_t2_s(pcd);
  if (p2 == NULL) return -1;
  if (p) delete_procd_t2_s(p);

  pthread_mutex_lock(&m_pcdno_handler_t2_mutex);
  hashmap_set(m_pcdno_handler_t2, &pcd.pcdno, sizeof(pcd.pcdno), (uintptr_t)p2);
  pthread_mutex_unlock(&m_pcdno_handler_t2_mutex);
  return 0;
}

int cevf_rm_procedure_t1(cevf_pcdno_t pcdno) {
  struct _procd_t1_s *p = NULL;
  pthread_mutex_lock(&m_pcdno_handler_t1_mutex);
  if (!hashmap_get(m_pcdno_handler_t1, &pcdno, sizeof(pcdno), (uintptr_t *)&p)) {
    pthread_mutex_unlock(&m_pcdno_handler_t1_mutex);
    return 0;
  }
  pthread_mutex_unlock(&m_pcdno_handler_t1_mutex);
  delete_procd_t1_s(p);
  pthread_mutex_lock(&m_pcdno_handler_t1_mutex);
  hashmap_remove(m_pcdno_handler_t1, &pcdno, sizeof(pcdno));
  pthread_mutex_unlock(&m_pcdno_handler_t1_mutex);
  return 0;
}

int cevf_rm_procedure_t2(cevf_pcdno_t pcdno) {
  struct _procd_t2_s *p = NULL;
  pthread_mutex_lock(&m_pcdno_handler_t2_mutex);
  if (!hashmap_get(m_pcdno_handler_t2, &pcdno, sizeof(pcdno), (uintptr_t *)&p)) {
    pthread_mutex_unlock(&m_pcdno_handler_t2_mutex);
    return 0;
  }
  pthread_mutex_unlock(&m_pcdno_handler_t2_mutex);
  delete_procd_t2_s(p);
  pthread_mutex_lock(&m_pcdno_handler_t2_mutex);
  hashmap_remove(m_pcdno_handler_t2, &pcdno, sizeof(pcdno));
  pthread_mutex_unlock(&m_pcdno_handler_t2_mutex);
  return 0;
}

/////////////////////////

cevf_mq_t cevf_qmsg_new_mq(size_t sz) {
  return (cevf_mq_t)qmsg2_new_mq(sz);
}

int cevf_qmsg_enq(cevf_mq_t mt, void *item) {
  if (mt == NULL) return cevf_qmsg2_res_error;
  return conv_qmsgres_res2(qmsg2_enq((struct qmsg2_s *)mt, item));
}

cevf_qmsg2_res_t cevf_qmsg_deq(cevf_mq_t mt, void **buf) {
  return conv_qmsgres_res2(qmsg2_deq((struct qmsg2_s *)mt, buf));
}

cevf_qmsg2_res_t cevf_qmsg_poll(cevf_mq_t mt, void *buf, time_t tv_sec, long tv_nsec) {
  return conv_qmsgres_res2(qmsg2_poll((struct qmsg2_s *)mt, buf, tv_sec, tv_nsec));
}

cevf_qmsg2_res_t cevf_qmsg_poll_nointr(cevf_mq_t mt, void *buf, time_t tv_sec, long tv_nsec) {
  return conv_qmsgres_res2(qmsg2_poll_nointr((struct qmsg2_s *)mt, buf, tv_sec, tv_nsec));
}

void cevf_qmsg_del_mq(cevf_mq_t mt) {
  qmsg2_del_mq((struct qmsg2_s *)mt);
}

/////////////////////////

static eloop_event_type conv_sockevent_eloope(cevf_sockevent_t typ) {
  switch (typ) {
    case CEVF_SOCKEVENT_TYPE_READ:
      return EVENT_TYPE_READ;
    case CEVF_SOCKEVENT_TYPE_WRITE:
      return EVENT_TYPE_WRITE;
    case CEVF_SOCKEVENT_TYPE_EXCEPTION:
      return EVENT_TYPE_EXCEPTION;
  }
}

static uint8_t cevf_register_sock_cnt = 0;
int cevf_register_sock(int sock, cevf_sockevent_t typ, cevf_sock_handler_t handler, void *arg1, void *arg2) {
  cevf_register_sock_cnt = 1;
  return eloop_register_sock(sock, conv_sockevent_eloope(typ), handler, arg1, arg2);
}

void cevf_unregister_sock(int sock, cevf_sockevent_t typ) {
  return eloop_unregister_sock(sock, conv_sockevent_eloope(typ));
}

int cevf_register_signal_terminate(cevf_signal_handler handler, void *user_data) {
  // eloop_register_signal(SIGINT, (eloop_signal_handler)handler, user_data);
  // eloop_register_signal(SIGTERM, (eloop_signal_handler)handler, user_data);
  signal(SIGINT, handler);
  signal(SIGTERM, handler);
  return 0;
}

/////////////////////////

struct cevf_timeout_s {
  cevf_timeout_handler_t handler;
  struct timespec tm;
  void *ctx;
  uint8_t cancelled;
};

static inline struct cevf_timeout_s *new_cevf_timeout_s(cevf_timeout_handler_t handler, struct timespec tm, void *ctx) {
  struct cevf_timeout_s *ans = (struct cevf_timeout_s *)malloc(sizeof(struct cevf_timeout_s));
  if (ans == NULL) return NULL;
  ans->handler = handler;
  ans->tm = tm;
  ans->ctx = ctx;
  ans->cancelled = 0;
  return ans;
}

static inline void delete_cevf_timeout_s(struct cevf_timeout_s *s) {
  if (s == NULL) return;
  free(s);
}

static int _timeout_cmpf(const void *timeout_a, const void *timeout_b, const void *ctx) {
  const struct cevf_timeout_s *a = (struct cevf_timeout_s *)timeout_a;
  const struct cevf_timeout_s *b = (struct cevf_timeout_s *)timeout_b;
  return timespec_cmp(a->tm, b->tm);
}

static inline int _timeout_exec(struct cevf_timeout_s *tmout) {
  tmout->handler(tmout->ctx);
}

typedef enum {
  cevf_tmouta_add,
  cevf_tmouta_delete,
  cevf_tmouta_deleteone,
} cevf_tmouta_typ_t;

struct cevf_tmouta_s {
  cevf_tmouta_typ_t typ;
  struct cevf_timeout_s *tmout;
  struct timespec *remaining;
  pthread_cond_t cv;
  pthread_mutex_t mutex;
  struct timespec tm;
};

static inline struct cevf_tmouta_s *new_cevf_tmouta_s(cevf_tmouta_typ_t typ, struct cevf_timeout_s *tmout, struct timespec *remaining) {
  struct cevf_tmouta_s *tmouta;
  tmouta = (struct cevf_tmouta_s *)malloc(sizeof(struct cevf_tmouta_s));
  if (tmouta == NULL) return NULL;
  tmouta->typ = typ;
  tmouta->tmout = tmout;
  tmouta->remaining = remaining;
  if (clock_gettime(CLOCK_REALTIME, &tmouta->tm) == -1) {
    perror("clock_gettime");
    tmouta->tm = (struct timespec){0};
  }
  return tmouta;
}

#define new_cevf_tmouta_s_add(tmout) new_cevf_tmouta_s(cevf_tmouta_add, tmout, NULL)
#define new_cevf_tmouta_s_delete(tmout) new_cevf_tmouta_s(cevf_tmouta_delete, tmout, NULL)
#define new_cevf_tmouta_s_deleteone(tmout, remaining) new_cevf_tmouta_s(cevf_tmouta_deleteone, tmout, remaining)

static inline void delete_cevf_tmouta_s(struct cevf_tmouta_s *s) {
  if (s == NULL) return;
  free(s);
}

// all timeouts in a heap, mainloop withdraw from this the closest timeout to execute
static heap_t *tmout_hp = NULL;
static pthread_mutex_t tmout_hp_mutex;

// map a timeout handler to all realted timeouts in a heap
static hashmap *m_tmok_tmouthp = NULL;
static pthread_mutex_t m_tmok_tmouthp_mutex;

struct tmok_s {
  cevf_timeout_handler_t handler;
  void *ctx;
} __attribute__((__packed__));

static inline struct tmok_s create_tmok_s(struct cevf_timeout_s *tmout) {
  return (struct tmok_s){
      .handler = tmout->handler,
      .ctx = tmout->ctx,
  };
}

static inline int _timeout_add_to_mainheap(struct cevf_timeout_s *tmout) {
  int ret;
  pthread_mutex_lock(&tmout_hp_mutex);
  ret = heap_offer(&tmout_hp, tmout);
  pthread_mutex_unlock(&tmout_hp_mutex);
  return ret;
}

static inline int _timeout_add_to_hashmap(struct cevf_timeout_s *tmout) {
  int ret;
  heap_t *tmouthp2 = NULL;
  struct tmok_s tmok = create_tmok_s(tmout);
  pthread_mutex_lock(&m_tmok_tmouthp_mutex);
  if (!hashmap_get(m_tmok_tmouthp, &tmok, sizeof(tmok), (uintptr_t *)&tmouthp2)) {
    tmouthp2 = heap_new(_timeout_cmpf, NULL);
  }
  ret = heap_offer(&tmouthp2, tmout);
  hashmap_set(m_tmok_tmouthp, &tmok, sizeof(tmok), (uintptr_t)tmouthp2);
  pthread_mutex_unlock(&m_tmok_tmouthp_mutex);
  return ret;
}

static size_t timeout_cnt = 0;
static pthread_mutex_t timeout_cnt_mutex;

static inline int _timeout_add(struct cevf_timeout_s *tmout) {
  pthread_mutex_lock(&timeout_cnt_mutex);
  timeout_cnt++;
  pthread_mutex_unlock(&timeout_cnt_mutex);
  _timeout_add_to_mainheap(tmout);
  _timeout_add_to_hashmap(tmout);
  return 0;
}

static inline void _timeout_remove_from_mainheap(struct cevf_timeout_s *tmout) {
  if (tmout == NULL) return;
  pthread_mutex_lock(&tmout_hp_mutex);
  heap_remove_item(tmout_hp, tmout);
  pthread_mutex_unlock(&tmout_hp_mutex);
}

static inline void _timeout_remove_from_hashmap(struct cevf_timeout_s *tmout) {
  struct cevf_timeout_s *tmout2;
  heap_t *tmouthp2 = NULL;

  if (tmout == NULL) return;
  struct tmok_s tmok = create_tmok_s(tmout);
  pthread_mutex_lock(&m_tmok_tmouthp_mutex);
  if (!hashmap_get(m_tmok_tmouthp, &tmok, sizeof(tmok), (uintptr_t *)&tmouthp2)) {
    pthread_mutex_unlock(&m_tmok_tmouthp_mutex);
    return;
  }
  heap_remove_item(tmouthp2, tmout);
  if (heap_count(tmouthp2) == 0) {
    heap_free(tmouthp2);
    hashmap_remove(m_tmok_tmouthp, &tmok, sizeof(tmok));
  }
  pthread_mutex_unlock(&m_tmok_tmouthp_mutex);
  return;
}

static inline void _timeout_remove(struct cevf_timeout_s *tmout) {
  _timeout_remove_from_hashmap(tmout);
  _timeout_remove_from_mainheap(tmout);
}

static inline struct cevf_timeout_s *_timeout_poll_from_mainheap(void) {
  struct cevf_timeout_s *ans = NULL;
  pthread_mutex_lock(&tmout_hp_mutex);
  ans = (struct cevf_timeout_s *)heap_poll(tmout_hp);
  pthread_mutex_unlock(&tmout_hp_mutex);
  return ans;
}

static inline struct cevf_timeout_s *_timeout_peek_from_mainheap(void) {
  struct cevf_timeout_s *ans = NULL;
  pthread_mutex_lock(&tmout_hp_mutex);
  ans = (struct cevf_timeout_s *)heap_peek(tmout_hp);
  pthread_mutex_unlock(&tmout_hp_mutex);
  return ans;
}

static inline int _timeout_flush_mainheap(void) {
  struct cevf_timeout_s *tmout;
  for (;;) {
    pthread_mutex_lock(&tmout_hp_mutex);
    tmout = (struct cevf_timeout_s *)heap_poll(tmout_hp);
    pthread_mutex_unlock(&tmout_hp_mutex);
    if (tmout == NULL) break;
    delete_cevf_timeout_s(tmout);
  }
}

static void _timeout_free_hashmap(void *key, size_t ksize, uintptr_t value, void *usr) {
  struct cevf_timeout_s *tmout2;
  heap_t *tmouthp2 = (heap_t *)value;
  while (tmout2 = (struct cevf_timeout_s *)heap_poll(tmouthp2)) {
    delete_cevf_timeout_s(tmout2);
  }
  heap_free(tmouthp2);
}

static inline void _timeout_flush_hashmap(void) {
  pthread_mutex_lock(&m_tmok_tmouthp_mutex);
  hashmap_iterate(m_tmok_tmouthp, _timeout_free_hashmap, NULL);
  pthread_mutex_unlock(&m_tmok_tmouthp_mutex);
}

static inline void _timeout_flush(void) {
  _timeout_flush_mainheap();
  _timeout_flush_hashmap();
}

static inline struct cevf_timeout_s *_timeout_next(void) {
  struct cevf_timeout_s *tmout = NULL;
  for (;;) {
    tmout = _timeout_poll_from_mainheap();
    if (tmout == NULL) break;
    if (!tmout->cancelled) break;
    _timeout_remove_from_hashmap(tmout);
    delete_cevf_timeout_s(tmout);
  }
  _timeout_remove_from_hashmap(tmout);
  return tmout;
}

static inline int _timeout_batchexec(void) {
  struct timespec tm;
  struct cevf_timeout_s *tmout;
  int ret = 0;
  for (;;) {
    tmout = _timeout_peek_from_mainheap();
    if (tmout == NULL) break;
    if (clock_gettime(CLOCK_REALTIME, &tm) == -1) {
      perror("clock_gettime");
      return -1;
    }
    if (tmout->cancelled) {
      _timeout_poll_from_mainheap();
      _timeout_remove_from_hashmap(tmout);
      delete_cevf_timeout_s(tmout);
    } else if (timespec_cmp(tm, tmout->tm) > 0) {
      _timeout_poll_from_mainheap();
      _timeout_exec(tmout);
      _timeout_remove_from_hashmap(tmout);
      delete_cevf_timeout_s(tmout);
      ret++;
    } else {
      break;
    }
  }
  return ret;
}

static inline int _timeout_delete(struct cevf_timeout_s *tmout) {
  struct cevf_timeout_s *tmout2;
  heap_t *tmouthp2 = NULL;
  struct tmok_s tmok = create_tmok_s(tmout);
  size_t ret = 0;

  pthread_mutex_lock(&m_tmok_tmouthp_mutex);
  if (!hashmap_get(m_tmok_tmouthp, &tmok, sizeof(tmok), (uintptr_t *)&tmouthp2)) {
    pthread_mutex_unlock(&m_tmok_tmouthp_mutex);
    return ret;
  }
  pthread_mutex_unlock(&m_tmok_tmouthp_mutex);
  pthread_mutex_lock(&timeout_cnt_mutex);
  timeout_cnt -= heap_count(tmouthp2);
  pthread_mutex_unlock(&timeout_cnt_mutex);
  pthread_mutex_lock(&m_tmok_tmouthp_mutex);
  hashmap_remove(m_tmok_tmouthp, &tmok, sizeof(tmok));
  pthread_mutex_unlock(&m_tmok_tmouthp_mutex);

  for (ret = 0; tmout2 = (struct cevf_timeout_s *)heap_poll(tmouthp2); ret++) {
    _timeout_remove_from_mainheap(tmout2);
    delete_cevf_timeout_s(tmout2);
  }

  heap_free(tmouthp2);
  return ret;
}

static inline int _timeout_deleteone(struct cevf_timeout_s *tmout, struct timespec tm, struct timespec *remaining) {
  struct cevf_timeout_s *tmout2;
  heap_t *tmouthp2 = NULL;
  struct tmok_s tmok = create_tmok_s(tmout);
  size_t ret = 0;
  pthread_mutex_lock(&m_tmok_tmouthp_mutex);
  if (!hashmap_get(m_tmok_tmouthp, &tmok, sizeof(tmok), (uintptr_t *)&tmouthp2)) {
    pthread_mutex_unlock(&m_tmok_tmouthp_mutex);
    return ret;
  }
  if (heap_count(tmouthp2) == 1) {
    hashmap_remove(m_tmok_tmouthp, &tmok, sizeof(tmok));
  }
  pthread_mutex_unlock(&m_tmok_tmouthp_mutex);

  if (tmout2 = (struct cevf_timeout_s *)heap_poll(tmouthp2)) {
    _timeout_remove_from_mainheap(tmout2);
    if (remaining) *remaining = timespec_sub(tmout2->tm, tm);
    delete_cevf_timeout_s(tmout2);
    ret = 1;
  }

  if (heap_count(tmouthp2) == 0) {
    heap_free(tmouthp2);
  }

  pthread_mutex_lock(&timeout_cnt_mutex);
  timeout_cnt--;
  pthread_mutex_unlock(&timeout_cnt_mutex);
  return ret;
}

static inline int _tmouta_exec(struct cevf_tmouta_s *tmouta, uint8_t need_signal) {
  if (tmouta == NULL) return 0;
  if (tmouta->tmout == NULL) return 0;
  switch (tmouta->typ) {
    case cevf_tmouta_add:
      if (_timeout_add(tmouta->tmout)) {
        // TODO
      }
      break;
    case cevf_tmouta_delete:
      if (_timeout_delete(tmouta->tmout)) {
        // TODO
      }
      delete_cevf_timeout_s(tmouta->tmout);
      break;
    case cevf_tmouta_deleteone:
      if (_timeout_deleteone(tmouta->tmout, tmouta->tm, tmouta->remaining)) {
        // TODO
      }
      delete_cevf_timeout_s(tmouta->tmout);
      break;
    default:
      break;
  }

  if (need_signal) pthread_cond_signal(&tmouta->cv);
}

static inline int _timeout_init(void) {
  pthread_mutex_init(&tmout_hp_mutex, NULL);
  pthread_mutex_init(&m_tmok_tmouthp_mutex, NULL);
  tmout_hp = heap_new(_timeout_cmpf, NULL);
  if (tmout_hp == NULL) {
    lge("timeout heap init failed\n");
    goto fail;
  }
  m_tmok_tmouthp = hashmap_create();
  if (m_tmok_tmouthp == NULL) {
    lge("timeout hashmap init failed\n");
    goto fail;
  }
  return 0;
fail:
  heap_free(tmout_hp);
  hashmap_free(m_tmok_tmouthp);
  return -1;
}

static inline void _timeout_deinit(void) {
  _timeout_flush();
  heap_free(tmout_hp);
  hashmap_free(m_tmok_tmouthp);
  pthread_mutex_destroy(&tmout_hp_mutex);
  pthread_mutex_destroy(&m_tmok_tmouthp_mutex);
}

static struct qmsg2_s *ctrl_mq;

static inline int _tmouta_enq_waitexec(struct cevf_tmouta_s *tmouta) {
  int ret = 0;

  if (pthread_mutex_init(&tmouta->mutex, NULL)) goto fail;
  if (pthread_cond_init(&tmouta->cv, NULL)) goto fail;
  pthread_mutex_lock(&tmouta->mutex);
  if (qmsg2_enq(ctrl_mq, tmouta))
    ret = -1;
  else
    pthread_cond_wait(&tmouta->cv, &tmouta->mutex);
  pthread_mutex_unlock(&tmouta->mutex);
  pthread_mutex_destroy(&tmouta->mutex);
  return ret;
fail:
  if (tmouta) pthread_mutex_unlock(&tmouta->mutex);
  if (tmouta) pthread_mutex_destroy(&tmouta->mutex);
  return -1;
}

static char *ctrl_string = "controller_string";
static uint8_t mainloop_start = 0;
static void cevf_mainloop(void) {
  struct cevf_tmouta_s *tmouta;
  struct cevf_timeout_s *tmout;
  qmsg2_res_t res;
  for (;;) {
    res = qmsg2_res_ok;
    _timeout_batchexec();
    void *msg = ctrl_string;
    tmout = _timeout_next();
    mainloop_start = 1;
    if (tmout == NULL)
      res = qmsg2_deq(ctrl_mq, &msg);
    else
      res = qmsg2_poll2_nointr(ctrl_mq, &msg, tmout->tm);
    mainloop_start = 0;
    if (res == qmsg2_res_timeout) {
      _timeout_exec(tmout);
      delete_cevf_timeout_s(tmout);
    } else if (res == qmsg2_res_ok) {
      if (msg == NULL) {
        _timeout_flush();
        delete_cevf_timeout_s(tmout);
        break;
      } else {
        if (_timeout_add(tmout)) {
          // TODO
        }
        _tmouta_exec((struct cevf_tmouta_s *)msg, 1);
      }
    } else {
      if (_timeout_add(tmout)) {
        // TODO
      }
    }
  }
}

int cevf_register_timeout(time_t tv_sec, long tv_nsec, cevf_timeout_handler_t handler, void *ctx) {
  int ret = 0;
  struct timespec tm;
  struct cevf_timeout_s *tmout = NULL;
  struct cevf_tmouta_s *tmouta = NULL;
  if (clock_gettime(CLOCK_REALTIME, &tm) == -1) {
    perror("clock_gettime");
    return -1;
  }
  tm = timespec_add(tm, (struct timespec){.tv_sec = tv_sec, .tv_nsec = tv_nsec});
  if (handler == NULL) return ret;
  tmout = new_cevf_timeout_s(handler, tm, ctx);
  if (tmout == NULL) goto fail;
  tmouta = new_cevf_tmouta_s_add(tmout);
  if (tmouta == NULL) goto fail;
  if (!mainloop_start) {
    if (_tmouta_exec(tmouta, 0)) goto fail;
  } else {
    if (_tmouta_enq_waitexec(tmouta)) goto fail;
  }
  delete_cevf_tmouta_s(tmouta);
  return ret;
fail:
  delete_cevf_timeout_s(tmout);
  delete_cevf_tmouta_s(tmouta);
  return -1;
}

int cevf_cancel_timeout(cevf_timeout_handler_t handler, void *ctx) {
  int ret = 0;
  struct cevf_timeout_s *tmout = NULL;
  struct cevf_tmouta_s *tmouta = NULL;
  if (handler == NULL) return 0;
  tmout = new_cevf_timeout_s(handler, (struct timespec){0}, ctx);
  if (tmout == NULL) goto fail;
  tmouta = new_cevf_tmouta_s_delete(tmout);
  if (tmouta == NULL) goto fail;
  if (!mainloop_start) {
    if (_tmouta_exec(tmouta, 0)) goto fail;
    delete_cevf_timeout_s(tmout);
  } else {
    if (_tmouta_enq_waitexec(tmouta)) goto fail;
  }
  delete_cevf_tmouta_s(tmouta);
  return ret;
fail:
  delete_cevf_timeout_s(tmout);
  delete_cevf_tmouta_s(tmouta);
  return -1;
}

int cevf_cancel_timeout_one(cevf_timeout_handler_t handler, void *ctx, struct timespec *remaining) {
  int ret = 0;
  struct cevf_timeout_s *tmout = NULL;
  struct cevf_tmouta_s *tmouta = NULL;
  if (handler == NULL) return 0;
  tmout = new_cevf_timeout_s(handler, (struct timespec){0}, ctx);
  if (tmout == NULL) goto fail;
  tmouta = new_cevf_tmouta_s_deleteone(tmout, remaining);
  if (tmouta == NULL) goto fail;
  if (!mainloop_start) {
    if (_tmouta_exec(tmouta, 0)) goto fail;
    delete_cevf_timeout_s(tmout);
  } else {
    if (_tmouta_enq_waitexec(tmouta)) goto fail;
  }
  delete_cevf_tmouta_s(tmouta);
  return ret;
fail:
  delete_cevf_timeout_s(tmout);
  delete_cevf_tmouta_s(tmouta);
  return -1;
}

int cevf_is_timeout_registered(cevf_timeout_handler_t handler, void *ctx) {
  heap_t *tmouthp2 = NULL;
  struct tmok_s tmok = (struct tmok_s){
      .handler = handler,
      .ctx = ctx,
  };
  pthread_mutex_lock(&m_tmok_tmouthp_mutex);
  if (!hashmap_get(m_tmok_tmouthp, &tmok, sizeof(tmok), (uintptr_t *)&tmouthp2)) {
    pthread_mutex_unlock(&m_tmok_tmouthp_mutex);
    return 0;
  }
  pthread_mutex_unlock(&m_tmok_tmouthp_mutex);
  return 1;
}

/////////////////////////

static CEVF_EV_THFDECL(cevf_mainloop_f, arg1) {
  cevf_mainloop();
}

static struct thpillar_s cevf_pillars[] = {{
                                               .thtyp = thtyp_producer,
                                               .handler = cevf_generic_enqueue_1,
                                           },
                                           {
                                               .thtyp = thtyp_consumer,
                                               .handler = NULL,
                                           }};

static struct thstat_s *cevf_run_ev(struct cevf_producer_s *pd_arr, cevf_asz_t pd_num, hashmap *m_evtyp_handler, uint8_t cm_thr_cnt) {
  struct thprop_s props[pd_num + cm_thr_cnt + 1];
  cevf_asz_t props_len = 0;
  for (cevf_asz_t i = 0; i < cm_thr_cnt; i++) {
    props[props_len++] = (struct thprop_s){
        .thstart = CEVF_EV_THFNAME(cevf_generic_consumer_loopf),
        .stack_size = CEVF_CONSUMER_STACKSIZE,
        .thtyp = thtyp_consumer,
        .evtyp = -1,  // not being used
        .terminator = CEVF_EV_THENDNAME(cevf_generic_consumer_loopf),
        .context = (void *)m_evtyp_handler,
    };
  }
  for (cevf_asz_t i = 0; i < pd_num; i++) {
    props[props_len++] = (struct thprop_s){
        .thstart = pd_arr[i].thstart,
        .stack_size = pd_arr[i].stack_size,
        .thtyp = thtyp_producer,
        .evtyp = -1,  // not being used
        .terminator = pd_arr[i].terminator,
        .context = pd_arr[i].context,
    };
  }
  if (cevf_register_sock_cnt > 0) {
    props[props_len++] = (struct thprop_s){
        .thstart = CEVF_EV_THFNAME(cevf_mainloop_f),
        .stack_size = CEVF_CONSUMER_STACKSIZE,
        .thtyp = thtyp_producer,
        .evtyp = -1,  // not being used
        .terminator = NULL,
        .context = NULL,
    };
  }

  return ev_run(props, props_len);
}

static uint32_t i_ini, j_ini;

static int cevf_run_initialisers(int argc, char *argv[], struct cevf_initialiser_s *ini_arr[CEVF_INI_PRIO_MAX], cevf_asz_t ini_num[CEVF_INI_PRIO_MAX]) {
  int res = 0;
  for (uint32_t i = 0; i < CEVF_INI_PRIO_MAX; i_ini = i++) {
    if (ini_num[i] == 0) continue;
    if (ini_arr[i] == NULL) continue;
    for (uint32_t j = 0; j < ini_num[i]; j_ini = j++) {
      if (ini_arr[i][j].init_f == NULL) continue;
      if (res = ini_arr[i][j].init_f(argc, argv)) return res;
    }
  }
  return res;
}

static void cevf_run_deinitialisers(struct cevf_initialiser_s *ini_arr[CEVF_INI_PRIO_MAX], cevf_asz_t ini_num[CEVF_INI_PRIO_MAX]) {
  for (uint32_t i = 0; i <= i_ini; i++) {
    if (ini_num[i_ini - i] == 0) continue;
    if (ini_arr[i_ini - i] == NULL) continue;
    for (uint32_t j = 0; j < ini_num[i_ini - i]; j++) {
      if (i == 0 && j > j_ini) break;
      if (ini_arr[i_ini - i][j].deinit_f) ini_arr[i_ini - i][j].deinit_f();
    }
  }
}

static inline int cevf_is_idle(void) {
  return cevf_register_sock_cnt == 0
    && qmsg2_count(ctrl_mq) == 0
    && qmsg2_count(data_mq) == 0
    && timeout_cnt == 0
    && ev_get_running_thr_cnt() == 0;
}

int cevf_init(size_t evqsz) {
  pthread_mutex_init(&timeout_cnt_mutex, NULL);
  pthread_mutex_init(&log_mutex, NULL);
  qmsg2_init();
  if (_timeout_init()) return -1;
  data_mq = qmsg2_new_mq(evqsz > 1 ? evqsz : CEVF_DEFAULT_DATA_MQ_SZ);
  if (data_mq == QMSG2_ERR_MQ) {
    lge("data_mq init failed\n");
    return -1;
  }
  cevf_init_m_pcdno_handler();
  if (eloop_init()) return -1;
  return 0;
}

int cevf_run(int argc, char *argv[], struct cevf_run_arg_s a) {
  int ret = 0;
  hashmap *m_evtyp_handler = compile_m_evtyp_handler(a.cm_t1_arr, a.cm_t1_num, a.cm_t2_arr, a.cm_t2_num);
  if (m_evtyp_handler == NULL) goto fail;

  ctrl_mq = qmsg2_new_mq(a.ctrlmqsz > 1 ? a.ctrlmqsz : CEVF_DEFAULT_CTRL_MQ_SZ);
  if (ctrl_mq == QMSG2_ERR_MQ) {
    lge("ctrl_msq init failed!\n");
    goto fail;
  }

  if (ret = cevf_run_initialisers(argc, argv, a.ini_arr, a.ini_num)) goto fail2;
  if (cevf_is_idle() && a.pd_num == 0) goto fail2;
  ev_init(cevf_pillars, sizeof(cevf_pillars) / sizeof(struct thpillar_s));
  struct thstat_s *thstat = cevf_run_ev(a.pd_arr, a.pd_num, m_evtyp_handler, a.cm_thr_cnt > 0 ? a.cm_thr_cnt : CEVF_DEFAULT_CM_THR_CNT);
  if (cevf_register_sock_cnt > 0)
    eloop_run();
  else
    cevf_mainloop();
  ev_join(thstat);
  ev_deinit();
fail2:
  cevf_run_deinitialisers(a.ini_arr, a.ini_num);
fail:
  qmsg2_del_mq(ctrl_mq);
  delete_m_evtyp_handler(m_evtyp_handler);
  return ret;
}

void cevf_terminate(void) {
  eloop_terminate();
  qmsg2_enq(ctrl_mq, NULL);
}

void cevf_deinit(void) {
  eloop_destroy();
  qmsg2_flush(data_mq, _data_msg_free);
  qmsg2_del_mq(data_mq);
  cevf_deinit_m_pcdno_handler();
  _timeout_deinit();
  qmsg2_deinit();
  pthread_mutex_destroy(&log_mutex);
  pthread_mutex_destroy(&timeout_cnt_mutex);
}
