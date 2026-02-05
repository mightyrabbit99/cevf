/*
 * Copyright (c) 2025 Tan Wah Ken
 *
 * License: The MIT License (MIT)
 */

#ifndef CEVF_EV_H
#define CEVF_EV_H
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

typedef void *(*cevf_thfunc_t)(void *tharg);
typedef void (*cevf_thendf_t)(void *context);

#define CEVF_EV_PASTER(x, y) x##_##y
#define CEVF_EV_CONCAT(x, y) CEVF_EV_PASTER(x,y)

#define CEVF_EV_THFNAME(fname) CEVF_EV_CONCAT(fname, th)
#define CEVF_EV_THFDECL(fname, argname) void *CEVF_EV_THFNAME(fname)(void *argname)
#define CEVF_EV_THENDNAME(fname) CEVF_EV_CONCAT(fname, terminate)
#define CEVF_EV_THENDDECL(fname, argname) void CEVF_EV_THENDNAME(fname)(void *argname)

#ifndef CEVF_EV_DTYP
#define CEVF_EV_DTYP uint16_t
#endif // CEVF_EV_DTYP

typedef uint8_t thtyp_t;
typedef CEVF_EV_DTYP evtyp_t;
typedef uint8_t ev_th_id_t;
typedef int (*evhandler_t)(uint8_t *data, size_t datalen, evtyp_t evtyp, void *context);

struct thpillar_s {
  thtyp_t thtyp;
  evhandler_t handler;
};

struct thprop_s {
  cevf_thfunc_t thstart;
  ssize_t stack_size;
  thtyp_t thtyp;
  evtyp_t evtyp;
  cevf_thendf_t terminator;
  void *context;
};

struct tharg_s {
  evhandler_t handler; // handler(data, datalen, evtyp, context)
  evtyp_t evtyp;
  void *context;
  int ret;
};

struct thstat_s;

ev_th_id_t ev_add_th(struct thprop_s pd);
int ev_rm_th(ev_th_id_t id);
void ev_init(struct thpillar_s pillars[], size_t pillars_len);
void ev_deinit(void);
struct thstat_s *ev_run(struct thprop_s props[], uint8_t props_len);
int ev_is_running(cevf_thfunc_t thstart);
size_t ev_get_running_thr_cnt(void);
int ev_join(struct thstat_s *thstat);
#define ev_handle3(argname, data, datalen, evtyp, ctx) ((struct tharg_s *)argname)->handler(data, datalen, evtyp, ctx)
#define ev_handle2(argname, data, datalen, evtyp) ev_handle3(argname, data, datalen, evtyp, ((struct tharg_s *)argname)->context)
#define ev_handle(argname, data, datalen) ev_handle2(argname, data, datalen, ((struct tharg_s *)argname)->evtyp)
#define ev_setret(argname, _ret) (((struct tharg_s *)argname)->ret = _ret)
#define ev_gethandl(argname) (((struct tharg_s *)argname)->handler)
#define ev_getcontext(argname) (((struct tharg_s *)argname)->context)
#define ev_asserthandlrf(argname, f) assert(ev_gethandl(argname) == f)

#endif // CEVF_EV_H
