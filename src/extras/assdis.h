#ifndef ASSDIS_H
#define ASSDIS_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cvector.h"
#include "cvector_utils.h"
#include "map.h"

#ifndef ASSDIS1_GC_EXPIRE_CNT
#define ASSDIS1_GC_EXPIRE_CNT 10
#endif  // ASSDIS1_GC_EXPIRE_CNT

#define mod_ceil(a, b) ((a) % (b) == 0 ? (b) : (a) % (b))

static inline void _szcpy(uint8_t *dst, const uint8_t *src, size_t count) {
  size_t i;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  for (i = 0; i < count; i++)
    dst[i] = src[i];
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  for (i = 0; i < count; i++)
    dst[i] = src[count - i - 1];
#else
#error byte order not defined
#endif
}

#define szcpy_w(p, data, datalen, maxl)               \
  do {                                                \
    if ((maxl) < (datalen))                           \
      return -1;                                      \
    _szcpy((uint8_t *)(p), (uint8_t *)data, datalen); \
    (p) += (datalen);                                 \
    (maxl) -= (datalen);                              \
  } while (0)

#define szcv_w(p, a, maxl)                              \
  do {                                                  \
    if ((maxl) < sizeof(a))                             \
      return -1;                                        \
    _szcpy((uint8_t *)(p), (uint8_t *)&(a), sizeof(a)); \
    (p) += sizeof(a);                                   \
    (maxl) -= sizeof(a);                                \
  } while (0)

#define szcv_r(a, p, maxl)                              \
  do {                                                  \
    if ((maxl) < sizeof(a))                             \
      return -1;                                        \
    _szcpy((uint8_t *)&(a), (uint8_t *)(p), sizeof(a)); \
    (p) += sizeof(a);                                   \
    (maxl) -= sizeof(a);                                \
  } while (0)

struct assdis_frag_s;
struct assdis_assembler_ctx_s;
struct assdis_data_s;
struct assdis_disassembler_ctx_s;

///////////////////////////////

struct assdis1_data_s {
  void *data;
  size_t datalen;
  void *ctx;
  void (*fragment_handler)(const uint8_t *, size_t, void *);
  void (*on_done)(void *);
  struct {
    uint16_t id;
    uint16_t fragno;
    uint8_t is_last_fragment;
  };
};

static inline void assdis1_destory_assdis_data_s(struct assdis1_data_s *d) {
  struct assdis1_data_s *dd = (struct assdis1_data_s *)d;
  if (dd == NULL)
    return;
  free(dd->data);
}

static inline ssize_t assdis1_repack_data(struct assdis_data_s *d, size_t fragsz, uint8_t *buf, size_t bufsz) {
  struct assdis1_data_s *dd = (struct assdis1_data_s *)d;
  uint8_t *p = buf;
  szcv_w(p, dd->id, bufsz);
  szcv_w(p, dd->is_last_fragment, bufsz);
  szcv_w(p, dd->fragno, bufsz);
  if (dd->fragno == 0) {
    szcv_w(p, dd->datalen, bufsz);
  }
  szcpy_w(p, dd->data + (dd->fragno * fragsz), dd->is_last_fragment ? dd->datalen - (dd->fragno * fragsz) : fragsz, bufsz);
  return (size_t)(p - buf);
}

static inline int assdis1_unpack_data(struct assdis_data_s *d, size_t fragsz, uint8_t *data, size_t datasz) {
  struct assdis1_data_s *dd = (struct assdis1_data_s *)d;
  uint8_t *p = data;
  size_t len2;
  szcv_r(dd->id, p, datasz);
  szcv_r(dd->is_last_fragment, p, datasz);
  szcv_r(dd->fragno, p, datasz);
  if (dd->fragno == 0) {
    szcv_r(dd->datalen, p, datasz);
  }
  len2 = datasz - (p - data);
  dd->data = malloc(len2);
  if (dd->data == NULL)
    return -1;
  memcpy(dd->data, p, len2);
  return 0;
}

struct assdis1_disassembler_ctx_s {
  uint16_t id;
  pthread_mutex_t id_mut;
};

static inline uint16_t _assdis1_get_new_id(struct assdis1_disassembler_ctx_s *ss) {
  pthread_mutex_lock(&ss->id_mut);
  if (++(ss->id) == 0)
    ss->id++;
  uint16_t ans = ss->id;
  pthread_mutex_unlock(&ss->id_mut);
  return ans;
}

static struct assdis_disassembler_ctx_s *assdis1_new_disassembler_ctx(void) {
  struct assdis1_disassembler_ctx_s *ans = (struct assdis1_disassembler_ctx_s *)malloc(sizeof(struct assdis1_disassembler_ctx_s));
  if (ans == NULL)
    return NULL;
  ans->id = 0;
  pthread_mutex_init(&ans->id_mut, NULL);
  return (struct assdis_disassembler_ctx_s *)ans;
}

static void assdis1_delete_disassembler_ctx(struct assdis_disassembler_ctx_s *s) {
  struct assdis1_disassembler_ctx_s *ss = (struct assdis1_disassembler_ctx_s *)s;
  pthread_mutex_destroy(&ss->id_mut);
  free(s);
}

static int assdis1_disassemble(struct assdis_disassembler_ctx_s *s, struct assdis_data_s *d, size_t fragsz) {
  struct assdis1_disassembler_ctx_s *ss = (struct assdis1_disassembler_ctx_s *)s;
  struct assdis1_data_s *dd = (struct assdis1_data_s *)d;
  ssize_t len2;
  uint8_t data2[fragsz + 30];

  if (dd->id == 0) {
    dd->id = _assdis1_get_new_id(ss);
    dd->fragno = 0;
    dd->is_last_fragment = 0;
  }

  if (dd->datalen > fragsz) {
    len2 = assdis1_repack_data(d, fragsz, data2, sizeof(data2));
    if (len2 == -1)
      return -1;
    dd->fragment_handler(data2, len2, dd->ctx);
    dd->fragno++;
    dd->datalen -= fragsz;
    return 1;
  }

  if (dd->datalen > 0) {
    dd->is_last_fragment = 1;
    len2 = assdis1_repack_data(d, fragsz, data2, sizeof(data2));
    if (len2 == -1)
      return -1;
    dd->fragment_handler(data2, len2, dd->ctx);
  }
  dd->on_done(dd->ctx);
  return 0;
}

struct _assdis1_ass_data_s {
  uint8_t *rawdata;
  size_t datasz;
  size_t datalen;
  uint16_t expirecnt;
  void *ctx;
  void (*on_assembled)(const uint8_t *, size_t, void *);
  void (*on_timeout)(void *);
};

static inline void destroy_assdis1_ass_data_s(struct _assdis1_ass_data_s *s) {
  if (s == NULL)
    return;
  free(s->rawdata);
}

static inline void delete_assdis1_ass_data_s(struct _assdis1_ass_data_s *s) {
  if (s == NULL)
    return;
  destroy_assdis1_ass_data_s(s);
  free(s);
}

static inline struct _assdis1_ass_data_s *new_assdis1_ass_data_s(size_t len, void *ctx, void (*on_assembled)(const uint8_t *, size_t, void *), void (*on_timeout)(void *)) {
  struct _assdis1_ass_data_s *ans = (struct _assdis1_ass_data_s *)malloc(sizeof(struct _assdis1_ass_data_s));
  if (ans == NULL)
    return NULL;
  ans->rawdata = (uint8_t *)malloc(len);
  if (ans->rawdata == NULL)
    goto fail;
  ans->datasz = len;
  ans->datalen = len;
  ans->expirecnt = 0;
  ans->ctx = ctx;
  ans->on_assembled = on_assembled;
  ans->on_timeout = on_timeout;
  return ans;
fail:
  free(ans->rawdata);
  free(ans);
  return NULL;
}

struct assdis1_assembler_ctx_s {
  hashmap *m_id_data;
  pthread_mutex_t m_id_data_mut;
};

static struct assdis_assembler_ctx_s *assdis1_new_assembler_ctx(void) {
  struct assdis1_assembler_ctx_s *ans = (struct assdis1_assembler_ctx_s *)malloc(sizeof(struct assdis1_assembler_ctx_s));
  ans->m_id_data = hashmap_create();
  pthread_mutex_init(&ans->m_id_data_mut, NULL);
  return (struct assdis_assembler_ctx_s *)ans;
}

static void _assdis1_delete_m_id_data_it(void *key, size_t ksize, uintptr_t value, void *usr) {
  struct _assdis1_ass_data_s *d = (struct _assdis1_ass_data_s *)value;
  delete_assdis1_ass_data_s(d);
}

static void assdis1_delete_assembler_ctx(struct assdis_assembler_ctx_s *s) {
  struct assdis1_assembler_ctx_s *ss = (struct assdis1_assembler_ctx_s *)s;
  hashmap_iterate(ss->m_id_data, _assdis1_delete_m_id_data_it, NULL);
  hashmap_free(ss->m_id_data);
  pthread_mutex_destroy(&ss->m_id_data_mut);
  free(ss);
}

static void _assdis1_delete_expired_data_it(void *key, size_t ksize, uintptr_t value, void *usr) {
  struct _assdis1_ass_data_s *d = (struct _assdis1_ass_data_s *)value;
  cvector_vector_type(uint16_t) *to_delete_arr = (cvector_vector_type(uint16_t) *)usr;
  if (d->expirecnt++ == ASSDIS1_GC_EXPIRE_CNT) {
    if (d->on_timeout)
      d->on_timeout(d->ctx);
    delete_assdis1_ass_data_s(d);
    cvector_push_back(*to_delete_arr, *(uint16_t *)key);
  }
}

static void assdis1_periodic_exec(struct assdis_assembler_ctx_s *s_a, struct assdis_disassembler_ctx_s *s_d) {
  struct assdis1_assembler_ctx_s *ss_a = (struct assdis1_assembler_ctx_s *)s_a;
  cvector_vector_type(uint16_t) to_delete_arr;
  pthread_mutex_lock(&ss_a->m_id_data_mut);
  hashmap_iterate(ss_a->m_id_data, _assdis1_delete_expired_data_it, &to_delete_arr);
  pthread_mutex_unlock(&ss_a->m_id_data_mut);
  uint16_t *it;
  pthread_mutex_lock(&ss_a->m_id_data_mut);
  cvector_for_each_in(it, to_delete_arr) {
    hashmap_remove(ss_a->m_id_data, it, sizeof(*it));
  }
  pthread_mutex_unlock(&ss_a->m_id_data_mut);
  cvector_free(to_delete_arr);
}

struct assdis1_frag_s {
  uint8_t *data;
  size_t len;
  void *ctx;
  void (*on_assembled)(const uint8_t *, size_t, void *);
  void (*on_timeout)(void *);
};

static int assdis1_assemble(struct assdis_assembler_ctx_s *s, struct assdis_frag_s *_frag, size_t fragsz) {
  struct assdis1_assembler_ctx_s *ss = (struct assdis1_assembler_ctx_s *)s;
  struct assdis1_frag_s *frag = (struct assdis1_frag_s *)_frag;
  struct assdis1_data_s d2 = (struct assdis1_data_s){0};
  struct _assdis1_ass_data_s *reasm;
  size_t len2;
  if (assdis1_unpack_data((struct assdis_data_s *)&d2, fragsz, frag->data, frag->len)) {
    goto fail;
  }
  // we assume that the first packet to arrive is always the first fragment
  if (d2.fragno == 0) {
    reasm = new_assdis1_ass_data_s(d2.datalen, d2.ctx, frag->on_assembled, frag->on_timeout);
    if (reasm == NULL)
      goto fail;
    pthread_mutex_lock(&ss->m_id_data_mut);
    hashmap_set(ss->m_id_data, &d2.id, sizeof(d2.id), (uintptr_t)reasm);
    pthread_mutex_unlock(&ss->m_id_data_mut);
  }
  pthread_mutex_lock(&ss->m_id_data_mut);
  if (!hashmap_get(ss->m_id_data, &d2.id, sizeof(d2.id), (uintptr_t *)&reasm)) {
    pthread_mutex_unlock(&ss->m_id_data_mut);
    // TODO
    goto fail;
  }
  pthread_mutex_unlock(&ss->m_id_data_mut);
  if (d2.is_last_fragment) {
    len2 = mod_ceil(reasm->datasz, fragsz);
  } else {
    len2 = fragsz;
  }
  memcpy(reasm->rawdata + (fragsz * d2.fragno), d2.data, len2);
  reasm->datalen -= len2;
  if (reasm->datalen == 0) {
    if (reasm->on_assembled)
      reasm->on_assembled(reasm->rawdata, reasm->datasz, reasm->ctx);
    delete_assdis1_ass_data_s(reasm);
    pthread_mutex_lock(&ss->m_id_data_mut);
    hashmap_remove(ss->m_id_data, &d2.id, sizeof(d2.id));
    pthread_mutex_unlock(&ss->m_id_data_mut);
  }
  assdis1_destory_assdis_data_s(&d2);
  return 0;
fail:
  assdis1_destory_assdis_data_s(&d2);
  return -1;
}

///////////////////////////////////////////

struct assdis_kern_s {
  struct assdis_disassembler_ctx_s *(*assdis_new_disassembler_ctx)(void);
  void (*assdis_delete_disassembler_ctx)(struct assdis_disassembler_ctx_s *);
  struct assdis_assembler_ctx_s *(*assdis_new_assembler_ctx)(void);
  void (*assdis_delete_assembler_ctx)(struct assdis_assembler_ctx_s *);
  int (*assdis_assemble)(struct assdis_assembler_ctx_s *, struct assdis_frag_s *, size_t);
  int (*assdis_disassemble)(struct assdis_disassembler_ctx_s *, struct assdis_data_s *, size_t);
  void (*assdis_periodic_exec)(struct assdis_assembler_ctx_s *, struct assdis_disassembler_ctx_s *);
};

static struct assdis_kern_s assdis1_kern = (struct assdis_kern_s){
  .assdis_new_disassembler_ctx = assdis1_new_disassembler_ctx,
  .assdis_delete_disassembler_ctx = assdis1_delete_disassembler_ctx,
  .assdis_new_assembler_ctx = assdis1_new_assembler_ctx,
  .assdis_delete_assembler_ctx = assdis1_delete_assembler_ctx,
  .assdis_assemble = assdis1_assemble,
  .assdis_disassemble = assdis1_disassemble,
  .assdis_periodic_exec = assdis1_periodic_exec,
};

static struct assdis_kern_s *assdis_kern = &assdis1_kern;
#define assdis_new_disassembler_ctx() assdis_kern->assdis_new_disassembler_ctx()
#define assdis_delete_disassembler_ctx(ctx) assdis_kern->assdis_delete_disassembler_ctx(ctx)
#define assdis_new_assembler_ctx() assdis_kern->assdis_new_assembler_ctx()
#define assdis_delete_assembler_ctx(ctx) assdis_kern->assdis_delete_assembler_ctx(ctx)
#define assdis_assemble(ctx, frag, fraglen) assdis_kern->assdis_assemble(ctx, frag, fraglen)
#define assdis_disassemble(ctx, data, fraglen) assdis_kern->assdis_disassemble(ctx, data, fraglen)
#define assdis_periodic_exec(ctx_a, ctx_d) assdis_kern->assdis_periodic_exec(ctx_a, ctx_d)

#endif  // ASSDIS_H
