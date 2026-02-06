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

#define CEVF_ELEVENTH_ARGUMENT(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, ...) a11
#define CEVF_VAARGC(...) CEVF_ELEVENTH_ARGUMENT(dummy, ##__VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)


typedef enum {
  CEVF_SOCKEVENT_TYPE_READ,
  CEVF_SOCKEVENT_TYPE_WRITE,
  CEVF_SOCKEVENT_TYPE_EXCEPTION,
} cevf_sockevent_t;

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

typedef uint16_t cevf_evtyp_t;
typedef uint16_t cevf_asz_t;
typedef uint16_t cevf_pcdno_t;
typedef void *cevf_mq_t;
typedef uint8_t cevf_producer_id_t;
#define CEVF_RESERVED_EV_THEND ((cevf_evtyp_t) - 1)

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

struct cevf_procedure_t1_s {
  cevf_pcdno_t pcdno;
  uint8_t synchronized;
  void *(*vfunc)(int argc, va_list argp);
};

struct cevf_procedure_t2_s {
  cevf_pcdno_t pcdno;
  uint8_t synchronized;
  uint8_t *(*func)(const uint8_t *input, size_t input_len, size_t *output_len, void *ctx);
  void *ctx;
};

#define CEVF_INI_PRIO_MAX 100

int cevf_add_procedure_t1(struct cevf_procedure_t1_s pcd);
int cevf_add_procedure_t2(struct cevf_procedure_t2_s pcd);
void *_cevf_exec_procedure_t1(cevf_pcdno_t pcdno, int argc, ...);
uint8_t *cevf_exec_procedure_t2(cevf_pcdno_t pcdno, const uint8_t *input, size_t input_len, size_t *output_len);
int cevf_rm_procedure_t1(cevf_pcdno_t pcdno);
int cevf_rm_procedure_t2(cevf_pcdno_t pcdno);
cevf_producer_id_t _cevf_add_producer(struct cevf_producer_s pd);
int cevf_rm_producer(cevf_producer_id_t id);
cevf_qmsg2_res_t cevf_generic_enqueue(void *data, cevf_evtyp_t evtyp);
cevf_qmsg2_res_t cevf_generic_enqueue_soft(void *data, cevf_evtyp_t evtyp);
cevf_qmsg2_res_t cevf_copy_enqueue(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp);
cevf_qmsg2_res_t cevf_copy_enqueue_soft(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp);
cevf_mq_t cevf_qmsg_new_mq(size_t sz);
int cevf_qmsg_enq(cevf_mq_t mt, void *item);
cevf_qmsg2_res_t cevf_qmsg_deq(cevf_mq_t mt, void **buf);
cevf_qmsg2_res_t cevf_qmsg_poll(cevf_mq_t mt, void *buf, time_t tv_sec, long tv_nsec);
cevf_qmsg2_res_t cevf_qmsg_poll_nointr(cevf_mq_t mt, void *buf, time_t tv_sec, long tv_nsec);
void cevf_qmsg_del_mq(cevf_mq_t mt);
int cevf_register_sock(int sock, cevf_sockevent_t typ, cevf_sock_handler_t handler, void *arg1, void *arg2);
void cevf_unregister_sock(int sock, cevf_sockevent_t typ);
int cevf_register_timeout(time_t tv_sec, long tv_nsec, cevf_timeout_handler_t handler, void *ctx);
int cevf_cancel_timeout(cevf_timeout_handler_t handler, void *ctx);
int cevf_cancel_timeout_one(cevf_timeout_handler_t handler, void *ctx, struct timespec *remaining);
int cevf_is_timeout_registered(cevf_timeout_handler_t handler, void *ctx);
void cevf_terminate(void);
void cevf_log_set(cevf_log_level_t level, FILE *stream);

#define cevf_add_producer(_thname, _context, _stack_size) \
  _cevf_add_producer((struct cevf_producer_s){            \
      .thstart = CEVF_THFNAME(_thname),                   \
      .stack_size = _stack_size,                          \
      .terminator = CEVF_THENDNAME(_thname),              \
      .context = _context,                                \
  })
#define cevf_exec_procedure_t1(typ, ...) _cevf_exec_procedure_t1(typ, CEVF_VAARGC(__VA_ARGS__), __VA_ARGS__)
#endif  // CEVF_H
