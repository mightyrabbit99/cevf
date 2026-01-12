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

#include "cevf_ev.h"

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#include "map.h"
#include "ids.h"

#define lge(...) fprintf(stderr, __VA_ARGS__)

struct thinfo_s {
  cevf_thfunc_t thstart;
  pthread_t thread_id;
	struct tharg_s *tharg;
  cevf_thendf_t terminator;
  pthread_mutex_t term_mut;
  ev_th_id_t dynamic_id;
};

inline static void destroy_thinfo_s(struct thinfo_s thinfo) {
  free(thinfo.tharg);
}

inline static void delete_thinfo_s(struct thinfo_s *thinfo) {
  destroy_thinfo_s(*thinfo);
  free(thinfo);
}

inline static struct thinfo_s *new_thinfo_s(void) {
  struct thinfo_s *thinfo = (struct thinfo_s *)malloc(sizeof(struct thinfo_s));
  *thinfo = (struct thinfo_s){0};
  return thinfo;
}

struct thstat_s {
  struct thinfo_s **thinfo_arr;
  uint8_t thinfo_arr_len;
};

inline static hashmap *compile_m_thtyp_handler(struct thpillar_s pillars[], size_t pillars_len) {
  hashmap *m_thtyp_handler = hashmap_create();
  size_t i;
  for (i = 0; i < pillars_len; i++) {
    hashmap_set(m_thtyp_handler, &pillars[i].thtyp, sizeof(pillars[i].thtyp), (uintptr_t)pillars[i].handler);
  }
  return m_thtyp_handler;
}

inline static void delete_m_thtyp_handler(hashmap *m_thtyp_handler) {
  hashmap_free(m_thtyp_handler);
}

static hashmap *m_thtyp_handler = NULL;

static void *ev_generic_thread_f(void *arg);

static hashmap *m_thfunc_cnt = NULL;
pthread_mutex_t th_cnt_mutex;

inline static int th_cnt_inc(cevf_thfunc_t thstart) {
  uintptr_t ans = 0;
  pthread_mutex_lock(&th_cnt_mutex);
  hashmap_get(m_thfunc_cnt, &thstart, sizeof(thstart), &ans);
  pthread_mutex_unlock(&th_cnt_mutex);
  ans++;
  pthread_mutex_lock(&th_cnt_mutex);
  hashmap_set(m_thfunc_cnt, &thstart, sizeof(thstart), ans);
  pthread_mutex_unlock(&th_cnt_mutex);
  return 0;
}

inline static int th_cnt_dec(cevf_thfunc_t thstart) {
  uintptr_t ans = 0;
  pthread_mutex_lock(&th_cnt_mutex);
  if (!hashmap_get(m_thfunc_cnt, &thstart, sizeof(thstart), &ans)) {
    pthread_mutex_unlock(&th_cnt_mutex);
    return -1;
  }
  pthread_mutex_unlock(&th_cnt_mutex);
  if (ans == 0) return -1;
  ans--;
  pthread_mutex_lock(&th_cnt_mutex);
  hashmap_set(m_thfunc_cnt, &thstart, sizeof(thstart), ans);
  pthread_mutex_unlock(&th_cnt_mutex);
  return 0;
}

inline static ssize_t th_cnt_get(cevf_thfunc_t thstart) {
  uintptr_t ans = 0;
  pthread_mutex_lock(&th_cnt_mutex);
  if (!hashmap_get(m_thfunc_cnt, &thstart, sizeof(thstart), (uintptr_t *)&ans)) {
    pthread_mutex_unlock(&th_cnt_mutex);
    return -1;
  }
  pthread_mutex_unlock(&th_cnt_mutex);
  return (size_t)ans;
}

inline static void th_cnt_init(void) {
  m_thfunc_cnt = hashmap_create();
  pthread_mutex_init(&th_cnt_mutex, NULL);
}

inline static void th_cnt_deinit(void) {
  pthread_mutex_destroy(&th_cnt_mutex);
  hashmap_free(m_thfunc_cnt);
}

static struct thinfo_s *create_th(struct thprop_s pd) {
  pthread_attr_t attr;
  evhandler_t handler;
  int s;
  struct thinfo_s *thinfo = new_thinfo_s();
  if (thinfo == NULL) return NULL;

  if (!hashmap_get(m_thtyp_handler, &pd.thtyp, sizeof(pd.thtyp), (uintptr_t *)&handler)) {
    goto fail;
  }

  s = pthread_attr_init(&attr);
  if (pd.stack_size > 0) {
    s = pthread_attr_setstacksize(&attr, pd.stack_size);
    if (s != 0) goto fail;
  }

  thinfo->terminator = pd.terminator;
  thinfo->tharg = (struct tharg_s *)malloc(sizeof(struct tharg_s));
  if (thinfo->tharg == NULL) goto fail;
  thinfo->tharg->handler = handler;
  thinfo->tharg->evtyp = pd.evtyp;
  thinfo->tharg->context = pd.context;
  thinfo->thstart = pd.thstart;
  s = pthread_mutex_init(&thinfo->term_mut, NULL);
  if (s != 0) {
    lge("pthread_mutex failed!\n");
    goto fail;
  }
  thinfo->dynamic_id = (ev_th_id_t)-1;
  s = pthread_create(&thinfo->thread_id, &attr, ev_generic_thread_f, (void *)thinfo);
  if (s != 0) {
    lge("pthread_create failed!\n");
    goto fail;
  }

  return thinfo;

fail:
  s = pthread_attr_destroy(&attr);
  delete_thinfo_s(thinfo);
  return NULL;
}

static void terminate_th(struct thinfo_s *thinfo) {
  if (thinfo == NULL) return;
  cevf_thendf_t term;
  pthread_mutex_lock(&thinfo->term_mut);
  term = thinfo->terminator;
  thinfo->terminator = NULL;
  pthread_mutex_unlock(&thinfo->term_mut);
  if (term) term(thinfo->tharg->context);
}

static void _terminate_th_2(void *thinfo) {
  terminate_th((struct thinfo_s *)thinfo);
}

static int destroy_th(struct thinfo_s *thinfo) {
  void *res;
  int s;
  if (thinfo == NULL) return 0;
  s = pthread_join(thinfo->thread_id, &res);
  if (s != 0) {
    lge("pthread_join failed!\n");
    return s;
  }
  free(res);
  pthread_mutex_destroy(&thinfo->term_mut);
  delete_thinfo_s(thinfo);
  return s;
}

static void _destroy_th_2(void *thinfo) {
  destroy_th((struct thinfo_s *)thinfo);
}

static struct ids_s *th_ids = NULL;

ev_th_id_t ev_add_th(struct thprop_s pd) {
  ev_th_id_t id = (ev_th_id_t)-1;
  if (th_ids == NULL) goto fail;
  struct thinfo_s *thinfo = create_th(pd);
  if (thinfo == NULL) goto fail;

  int idsid = ids_add_item(th_ids, (void *)thinfo);
  if (idsid == -1) goto fail;
  id = (ev_th_id_t)idsid;
  thinfo->dynamic_id = id;
fail:
  return id;
}

int ev_rm_th(ev_th_id_t id) {
  if (th_ids == NULL) goto fail;
  struct thinfo_s *thinfo = (struct thinfo_s *)ids_get_item(th_ids, (ids_id_t)id);
  terminate_th(thinfo);
  destroy_th(thinfo);
  return ids_rm_item(th_ids, (ids_id_t)id);
fail:
  return -1;
}

static void *ev_generic_thread_f(void *arg) {
  struct thinfo_s *thinfo = (struct thinfo_s *)arg;
  cevf_thendf_t term;
  th_cnt_inc(thinfo->thstart);
  void *res = thinfo->thstart(thinfo->tharg);
  if (thinfo->dynamic_id != (ev_th_id_t)-1) ev_rm_th(thinfo->dynamic_id);
  else {
    pthread_mutex_lock(&thinfo->term_mut);
    term = thinfo->terminator;
    thinfo->terminator = NULL;
    pthread_mutex_unlock(&thinfo->term_mut);
    if (term) term(thinfo->tharg->context);
  }
  th_cnt_dec(thinfo->thstart);
  return res;
}

void ev_init(struct thpillar_s pillars[], size_t pillars_len) {
  th_ids = ids_new_idsys();
  th_cnt_init();
  m_thtyp_handler = compile_m_thtyp_handler(pillars, pillars_len);
}

void ev_deinit(void) {
  delete_m_thtyp_handler(m_thtyp_handler);
  ids_foreach(th_ids, _terminate_th_2);
  ids_foreach(th_ids, _destroy_th_2);
  ids_free_idsys(th_ids);
  th_cnt_deinit();
}

struct thstat_s *ev_run(struct thprop_s props[], uint8_t props_len) {
  struct thinfo_s **thinfo = (struct thinfo_s **)malloc(sizeof(struct thinfo_s *) * props_len);

  for (uint8_t i = 0; i < props_len; i++) {
    thinfo[i] = create_th(props[i]);
	}

  struct thstat_s *ret = (struct thstat_s *)malloc(sizeof(struct thstat_s));
  ret->thinfo_arr = thinfo;
  ret->thinfo_arr_len = props_len;
  return ret;
}

int ev_is_running(cevf_thfunc_t thstart) {
  return th_cnt_get(thstart) > 0;
}

int ev_join(struct thstat_s *thstat) {
  void *res;
  int s;
  uint8_t tnum;
  struct thinfo_s **thinfo_arr = thstat->thinfo_arr;
  for (uint8_t i = 0; i < thstat->thinfo_arr_len; i++) {
    tnum = thstat->thinfo_arr_len - i - 1;
    terminate_th(thinfo_arr[tnum]);
  }
  for (uint8_t i = 0; i < thstat->thinfo_arr_len; i++) {
    tnum = thstat->thinfo_arr_len - i - 1;
    destroy_th(thinfo_arr[tnum]);
  }
  free(thinfo_arr);
  free(thstat);
  return 0;
}
