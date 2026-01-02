/*
 * Copyright (c) 2025 mightyrabbit99
 *
 * License: The MIT License (MIT)
 */

#ifndef CEVF_H
#define CEVF_H

#include <stdint.h>
#include <sys/types.h>

typedef enum {
	CEVF_SOCKEVENT_TYPE_READ,
	CEVF_SOCKEVENT_TYPE_WRITE,
	CEVF_SOCKEVENT_TYPE_EXCEPTION
} cevf_sockevent_t;


typedef void *(*cevf_thfunc_t)(void *tharg);
typedef void (*cevf_thendf_t)(void *context);
typedef int (*cevf_initfunc_t)(void);
typedef void (*cevf_deinitfunc_t)(void);
typedef void (*cevf_sock_handler_t)(int sock, void *arg1, void *arg2);
typedef void (*cevf_timeout_handler_t)(void *eloop_ctx, void *user_ctx);
typedef void (*cevf_signal_handler)(int sig);

#define CEVF_EV_PASTER(x, y) x ## _ ## y
#define CEVF_EV_CONCAT(x, y) CEVF_EV_PASTER(x,y)

#define CEVF_EV_THFNAME(fname) CEVF_EV_CONCAT(fname, th)
#define CEVF_EV_THFDECL(fname, argname) void *CEVF_EV_THFNAME(fname)(void *argname)
#define CEVF_EV_THENDNAME(fname) CEVF_EV_CONCAT(fname, terminate)
#define CEVF_EV_THENDDECL(fname, argname) void CEVF_EV_THENDNAME(fname)(void *argname)

#ifndef CEVF_EV_DTYP
#define CEVF_EV_DTYP uint8_t
#endif // CEVF_EV_DTYP
typedef CEVF_EV_DTYP cevf_evtyp_t;
typedef uint8_t cevf_asz_t;
typedef void *cevf_mq_t;
typedef uint8_t cevf_producer_id_t;
typedef int (*cevf_consumer_handler_t)(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp);

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

struct cevf_consumer_s {
  cevf_evtyp_t evtyp;
  cevf_consumer_handler_t handler;
};

cevf_producer_id_t _cevf_add_producer(struct cevf_producer_s pd);
int cevf_rm_producer(cevf_producer_id_t id);
int cevf_generic_enqueue(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp);
int cevf_init(void);
int cevf_run(struct cevf_initialiser_s *ini_arr, uint8_t ini_num, struct cevf_producer_s *pd_arr, cevf_asz_t pd_num, struct cevf_consumer_s *cm_arr, cevf_asz_t cm_num, uint8_t cm_thr_cnt);
cevf_mq_t cevf_qmsg_new_mq(size_t sz);
int cevf_qmsg_enq(cevf_mq_t mt, void *item);
int cevf_qmsg_deq(cevf_mq_t mt, void **buf);
int cevf_qmsg_poll(cevf_mq_t mt, void *buf, int timeout);
void cevf_qmsg_del_mq(cevf_mq_t mt);
int cevf_register_sock(int sock, cevf_sockevent_t typ, cevf_sock_handler_t handler, void *arg1, void *arg2);
void cevf_unregister_sock(int sock, cevf_sockevent_t typ);
int cevf_register_signal_terminate(cevf_signal_handler handler, void *user_data);
void cevf_terminate(void);
void cevf_deinit(void);

#define CEVF_MON_INTERVAL 1
#define CEVF_RESERVED_EV_THEND ((cevf_evtyp_t)-1)
#define CEVF_THFDECL(fname, argname) CEVF_EV_THFDECL(fname, argname)
#define CEVF_THFNAME(fname) CEVF_EV_THFNAME(fname)
#define CEVF_THENDDECL(fname, argname) CEVF_EV_THENDDECL(fname, argname)
#define CEVF_THENDNAME(fname) CEVF_EV_THENDNAME(fname)
#define cevf_add_producer(_thname, _context, _stack_size) \
  _cevf_add_producer((struct cevf_producer_s){ \
    .thstart = CEVF_THFNAME(_thname), \
    .stack_size = _stack_size, \
    .terminator = CEVF_THENDNAME(_thname), \
    .context = _context, \
  })

#endif // CEVF_H
