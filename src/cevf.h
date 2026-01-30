/*
 * Copyright (c) 2025 Tan Wah Ken
 *
 * License: The MIT License (MIT)
 */

#ifndef CEVF_H
#define CEVF_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

typedef enum { CEVF_SOCKEVENT_TYPE_READ, CEVF_SOCKEVENT_TYPE_WRITE, CEVF_SOCKEVENT_TYPE_EXCEPTION } cevf_sockevent_t;

typedef enum {
  cevf_qmsg2_res_error = -1,
  cevf_qmsg2_res_ok,
  cevf_qmsg2_res_timeout,
  cevf_qmsg2_res_interrupt,
  cevf_qmsg2_res_softblocked,
} cevf_qmsg2_res_t;

typedef enum {
  cevf_log_level_error,
  cevf_log_level_warning,
  cevf_log_level_debug,
} cevf_log_level_t;

typedef void *(*cevf_thfunc_t)(void *tharg);
typedef void (*cevf_thendf_t)(void *context);
typedef int (*cevf_initfunc_t)(int argc, char *argv[]);
typedef void (*cevf_deinitfunc_t)(void);
typedef void (*cevf_sock_handler_t)(int sock, void *arg1, void *arg2);
typedef void (*cevf_timeout_handler_t)(void *ctx);
typedef void (*cevf_signal_handler)(int sig);

#define CEVF_PASTER(x, y) x##_##y
#define CEVF_CONCAT(x, y) CEVF_PASTER(x, y)

#ifndef CEVF_EV_DTYP
#define CEVF_EV_DTYP uint16_t
#endif  // CEVF_EV_DTYP
typedef CEVF_EV_DTYP cevf_evtyp_t;
typedef uint16_t cevf_asz_t;
typedef void *cevf_mq_t;
typedef uint8_t cevf_producer_id_t;
typedef int (*cevf_consumer_handler_t1_t)(void *data, cevf_evtyp_t evtyp);
typedef int (*cevf_consumer_handler_t2_t)(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp);

struct cevf_initialiser_s {
  cevf_initfunc_t init_f;
  cevf_deinitfunc_t deinit_f;
};

struct cevf_producer_s {
  cevf_thfunc_t thstart;
  ssize_t stack_size;
  cevf_thendf_t terminator;
  void *context;
};

typedef enum {
  cevf_consumer_typ_t1,
  cevf_consumer_typ_t2,
} cevf_consumer_typ_t;

struct cevf_consumer_t1_s {
  cevf_evtyp_t evtyp;
  cevf_consumer_handler_t1_t handler;
};

struct cevf_consumer_t2_s {
  cevf_evtyp_t evtyp;
  cevf_consumer_handler_t2_t handler;
};

#define CEVF_INI_PRIO_MAX 100

cevf_producer_id_t _cevf_add_producer(struct cevf_producer_s pd);
int cevf_rm_producer(cevf_producer_id_t id);
cevf_qmsg2_res_t cevf_generic_enqueue(void *data, cevf_evtyp_t evtyp);
cevf_qmsg2_res_t cevf_generic_enqueue_soft(void *data, cevf_evtyp_t evtyp);
cevf_qmsg2_res_t cevf_copy_enqueue(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp);
cevf_qmsg2_res_t cevf_copy_enqueue_soft(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp);
int cevf_init(void);
int cevf_run(int argc, char *argv[], struct cevf_initialiser_s *ini_arr[CEVF_INI_PRIO_MAX], cevf_asz_t ini_num[CEVF_INI_PRIO_MAX], struct cevf_producer_s *pd_arr, cevf_asz_t pd_num,
             struct cevf_consumer_t1_s *cm_t1_arr, cevf_asz_t cm_t1_num, struct cevf_consumer_t2_s *cm_t2_arr, cevf_asz_t cm_t2_num, uint8_t cm_thr_cnt);
cevf_mq_t cevf_qmsg_new_mq(size_t sz);
int cevf_qmsg_enq(cevf_mq_t mt, void *item);
cevf_qmsg2_res_t cevf_qmsg_deq(cevf_mq_t mt, void **buf);
cevf_qmsg2_res_t cevf_qmsg_poll(cevf_mq_t mt, void *buf, time_t tv_sec, long tv_nsec);
cevf_qmsg2_res_t cevf_qmsg_poll_nointr(cevf_mq_t mt, void *buf, time_t tv_sec, long tv_nsec);
void cevf_qmsg_del_mq(cevf_mq_t mt);
int cevf_register_sock(int sock, cevf_sockevent_t typ, cevf_sock_handler_t handler, void *arg1, void *arg2);
void cevf_unregister_sock(int sock, cevf_sockevent_t typ);
int cevf_register_signal_terminate(cevf_signal_handler handler, void *user_data);
int cevf_register_timeout(time_t tv_sec, long tv_nsec, cevf_timeout_handler_t handler, void *ctx);
int cevf_cancel_timeout(cevf_timeout_handler_t handler, void *ctx);
int cevf_cancel_timeout_one(cevf_timeout_handler_t handler, void *ctx, struct timespec *remaining);
int cevf_is_timeout_registered(cevf_timeout_handler_t handler, void *ctx);
void cevf_terminate(void);
void cevf_log_set(cevf_log_level_t level, FILE *stream);
void cevf_deinit(void);

#define CEVF_MON_INTERVAL 1
#define CEVF_RESERVED_EV_THEND ((cevf_evtyp_t) - 1)
#define CEVF_THFDECL(fname, argname) void *CEVF_THFNAME(fname)(void *argname)
#define CEVF_THFNAME(fname) CEVF_CONCAT(fname, th)
#define CEVF_THENDDECL(fname, argname) void CEVF_THENDNAME(fname)(void *argname)
#define CEVF_THENDNAME(fname) CEVF_CONCAT(fname, terminate)
#define cevf_add_producer(_thname, _context, _stack_size) \
  _cevf_add_producer((struct cevf_producer_s){            \
      .thstart = CEVF_THFNAME(_thname),                   \
      .stack_size = _stack_size,                          \
      .terminator = CEVF_THENDNAME(_thname),              \
      .context = _context,                                \
  })

#endif  // CEVF_H
