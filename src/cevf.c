/*
 * Copyright (c) 2025 mightyrabbit99
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

#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>

#include "cevf_ev.h"
#include "cvector.h"
#include "cvector_utils.h"
#include "map.h"
#include "qmsg2.h"
#include "eloop/os.h"
#include "eloop/eloop.h"

/////////////////////////////////////
#define CEVF_CONSUMER_STACKSIZE 0
#define CEVF_DATA_MQ_SZ 10
#define CEVF_CTRL_MQ_SZ 2

static struct qmsg2_s *data_mq;

struct _data_s {
  cevf_evtyp_t evtyp;
  size_t datalen;
  uint8_t *data;
};

int cevf_generic_enqueue(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  int ret;
  struct _data_s *d = (struct _data_s *)malloc(sizeof(struct _data_s));
  d->evtyp = evtyp;
  d->data = data;
  d->datalen = datalen;
  if (ret = qmsg2_enq(data_mq, (void *)d) < 0) {
    free(d);
  }
  return ret;
}

static ssize_t cevf_generic_dequeue(uint8_t **data, cevf_evtyp_t *evtyp) {
  size_t datalen;
  void *msg = NULL;
  qmsg2_deq(data_mq, &msg);
  if (msg == NULL) return -1;
  struct _data_s *d = (struct _data_s *)msg;
  *evtyp = d->evtyp;
  *data = d->data;
  datalen = d->datalen;
  free(d);
  return datalen;
}

static int cevf_generic_enqueue_1(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp, void *context) {
  return cevf_generic_enqueue(data, datalen, evtyp);
}

static int cevf_handle_event_1(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp, void *context) {
  hashmap *m_evtyp_handler = (hashmap *)context;
  cvector_vector_type(cevf_consumer_handler_t) handler_arr = NULL;
  if (!hashmap_get(m_evtyp_handler, &evtyp, sizeof(evtyp), (uintptr_t *)&handler_arr)) {
    fprintf(stderr, "No handler for evtyp=%d!\n", evtyp);
    return -1;
  }

  int res = 0;
  cevf_consumer_handler_t *f;
  cvector_for_each_in(f, handler_arr) {
    if (res = (*f)(data, datalen, evtyp) < 0)
      break;
  }
  return res;
}

static CEVF_EV_THENDDECL(cevf_generic_consumer_loopf, arg1) {
  cevf_generic_enqueue(NULL, 0, CEVF_RESERVED_EV_THEND);
}

static CEVF_EV_THFDECL(cevf_generic_consumer_loopf, arg1) {
  ev_asserthandlrf(arg1, cevf_handle_event_1);
  int ret = 0;
  uint8_t *data;
  ssize_t datalen;
  cevf_evtyp_t evtyp;
  for (;;) {
    datalen = cevf_generic_dequeue(&data, &evtyp);
    if (-1 == datalen) return NULL;
    if (evtyp == CEVF_RESERVED_EV_THEND) break;
    if (ret = ev_handle2(arg1, data, datalen, evtyp)) break;
  }
  ev_setret(arg1, ret);
  return NULL;
}

static hashmap *compile_m_evtyp_handler(struct cevf_consumer_s *cm_arr, cevf_asz_t cm_num) {
  hashmap *m_evtyp_handler = hashmap_create();
  if (m_evtyp_handler == NULL) return NULL;
  cevf_asz_t i;
  cvector_vector_type(cevf_consumer_handler_t) handler_arr = NULL;
  for (i = 0; i < cm_num; i++) {
    if (!hashmap_get(m_evtyp_handler, &cm_arr[i].evtyp, sizeof(cm_arr[i].evtyp), (uintptr_t *)&handler_arr)) {
      handler_arr = NULL;
    }
    cvector_push_back(handler_arr, cm_arr[i].handler);
    hashmap_set(m_evtyp_handler, &cm_arr[i].evtyp, sizeof(cm_arr[i].evtyp), (uintptr_t)handler_arr);
  }
  return m_evtyp_handler;
}

static void _delete_m_evtyp_handler_it(void *key, size_t ksize, uintptr_t value, void *usr) {
  cvector_vector_type(cevf_consumer_handler_t) handler_arr = (cvector_vector_type(cevf_consumer_handler_t))value;
  cvector_free(handler_arr);
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
    .evtyp = -1, // not being used
    .terminator = pd.terminator,
    .context = pd.context,
  });
}

int cevf_rm_producer(cevf_producer_id_t id) {
  return ev_rm_th((ev_th_id_t)id);
}

/////////////////////////
cevf_mq_t cevf_qmsg_new_mq(size_t sz) {
  return (cevf_mq_t)qmsg2_new_mq(sz);
}

int cevf_qmsg_enq(cevf_mq_t mt, void *item) {
  if (mt == NULL) return -1;
  return qmsg2_enq((struct qmsg2_s *)mt, item);
}

int cevf_qmsg_deq(cevf_mq_t mt, void **buf) {
  return qmsg2_deq((struct qmsg2_s *)mt, buf);
}

int cevf_qmsg_poll(cevf_mq_t mt, void *buf, int timeout) {
  return qmsg2_poll((struct qmsg2_s *)mt, buf, timeout, 0);
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
  signal(SIGINT, handler);
  signal(SIGTERM, handler);
  return 0;
}

/////////////////////////

static struct qmsg2_s *ctrl_mq;
static char *ctrl_string = "controller_string";
static void cevf_mainloop(void) {
  for (;;) {
    void *msg = ctrl_string;
    //qmsg2_poll(ctrl_mq, &msg, 0, 100000000);
    qmsg2_deq(ctrl_mq, &msg);
    if (msg == NULL) break;
  }
}

static CEVF_EV_THFDECL(cevf_mainloop_f, arg1) {
  cevf_mainloop();
}

static struct thstat_s *cevf_run_ev(struct cevf_producer_s *pd_arr, cevf_asz_t pd_num, struct cevf_consumer_s *cm_arr, cevf_asz_t cm_num, uint8_t cm_thr_cnt, hashmap *m_evtyp_handler) {
  struct thpillar_s pillars[] = {
    {
      .thtyp = thtyp_producer,
      .handler = cevf_generic_enqueue_1,
    },
    {
      .thtyp = thtyp_consumer,
      .handler = cevf_handle_event_1,
    }
  };

  struct thprop_s props[pd_num + cm_thr_cnt + 1];
  cevf_asz_t props_len = 0;
  for (cevf_asz_t i = 0; i < cm_thr_cnt; i++) {
    props[props_len++] = (struct thprop_s){
      .thstart = CEVF_EV_THFNAME(cevf_generic_consumer_loopf),
      .stack_size = CEVF_CONSUMER_STACKSIZE,
      .thtyp = thtyp_consumer,
      .evtyp = -1, // not being used
      .terminator = CEVF_EV_THENDNAME(cevf_generic_consumer_loopf),
      .context = (void *)m_evtyp_handler,
    };
  }
  for (cevf_asz_t i = 0; i < pd_num; i++) {
    props[props_len++] = (struct thprop_s){
      .thstart = pd_arr[i].thstart,
      .stack_size = pd_arr[i].stack_size,
      .thtyp = thtyp_producer,
      .evtyp = -1, // not being used
      .terminator = pd_arr[i].terminator,
      .context = pd_arr[i].context,
    };
  }
  if (cevf_register_sock_cnt > 0) {
    props[props_len++] = (struct thprop_s){
      .thstart = CEVF_EV_THFNAME(cevf_mainloop_f),
      .stack_size = CEVF_CONSUMER_STACKSIZE,
      .thtyp = thtyp_producer,
      .evtyp = -1, // not being used
      .terminator = NULL,
      .context = NULL,
    };
  }

  ev_init(pillars, sizeof(pillars) / sizeof(struct thpillar_s));
  return ev_run(props, props_len);
}

static uint32_t i_ini, j_ini;

static int cevf_run_initialisers(struct cevf_initialiser_s *ini_arr[CEVF_INI_PRIO_MAX], cevf_asz_t ini_num[CEVF_INI_PRIO_MAX]) {
  int res = 0;
  for (uint32_t i = 0; i < CEVF_INI_PRIO_MAX; i_ini = i++) {
    if (ini_num[i] == 0) continue;
    if (ini_arr[i] == NULL) continue;
    for (uint32_t j = 0; j < ini_num[i]; j_ini = j++) {
      if (ini_arr[i][j].init_f == NULL) continue;
      if (res = ini_arr[i][j].init_f()) return res;
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

int cevf_init(void) {
  qmsg2_init();
  data_mq = qmsg2_new_mq(CEVF_DATA_MQ_SZ);
  if (data_mq == QMSG2_ERR_MQ) {
    fprintf(stderr, "data_mq init failed");
    return -1;
  }
  if (eloop_init()) return -1;
  return 0;
}

int cevf_run(struct cevf_initialiser_s *ini_arr[CEVF_INI_PRIO_MAX], cevf_asz_t ini_num[CEVF_INI_PRIO_MAX], struct cevf_producer_s *pd_arr, cevf_asz_t pd_num, struct cevf_consumer_s *cm_arr, cevf_asz_t cm_num, uint8_t cm_thr_cnt) {
  int ret = 0;
  hashmap *m_evtyp_handler = compile_m_evtyp_handler(cm_arr, cm_num);
  if (m_evtyp_handler == NULL) goto fail;

  ctrl_mq = qmsg2_new_mq(CEVF_CTRL_MQ_SZ);
  if (ctrl_mq == QMSG2_ERR_MQ) {
    fprintf(stderr, "ctrl_msq init failed!");
    goto fail;
  }

  if (ret = cevf_run_initialisers(ini_arr, ini_num)) goto fail2;
  struct thstat_s *thstat = cevf_run_ev(pd_arr, pd_num, cm_arr, cm_num, cm_thr_cnt, m_evtyp_handler);
  if (cevf_register_sock_cnt > 0)
    eloop_run();
  else
    cevf_mainloop();
  ev_join(thstat);
  ev_deinit();
fail2:
  cevf_run_deinitialisers(ini_arr, ini_num);
fail:
  eloop_destroy();
  qmsg2_del_mq(ctrl_mq);
  delete_m_evtyp_handler(m_evtyp_handler);
  return ret;
}

void cevf_terminate(void) {
  eloop_terminate();
  qmsg2_enq(ctrl_mq, NULL);
}

void cevf_deinit(void) {
  qmsg2_del_mq(data_mq);
  qmsg2_deinit();
}
