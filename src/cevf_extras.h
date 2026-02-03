#ifndef CEVF_EXTRAS_H
#define CEVF_EXTRAS_H

#include <stdlib.h>
#include <string.h>

typedef void (*cevf_tcpsrv_on_close_t)(void *ctx);

struct cevf_tcpsrv_rcv_s {
  int sd;
  char *rcvdata;
  size_t rcvdata_len;
  cevf_tcpsrv_on_close_t on_close;
  void *ctx;
};

static inline struct cevf_tcpsrv_rcv_s *new_cevf_tcpsrv_rcv_s(char *readbuf, size_t len, int sd, void (*on_close)(void *), void *ctx) {
  struct cevf_tcpsrv_rcv_s *ans = (struct cevf_tcpsrv_rcv_s *)malloc(sizeof(struct cevf_tcpsrv_rcv_s));
  if (ans == NULL) return NULL;
  ans->rcvdata_len = len;
  ans->rcvdata = (char *)malloc(ans->rcvdata_len);
  if (ans->rcvdata == NULL) goto fail;
  memcpy(ans->rcvdata, readbuf, ans->rcvdata_len);
  ans->sd = sd;
  ans->on_close = on_close;
  ans->ctx = ctx;
  return ans;
fail:
  free(ans);
  return NULL;
}

static inline void delete_cevf_tcpsrv_rcv_s(struct cevf_tcpsrv_rcv_s *s) {
  free(s->rcvdata);
  free(s);
}

#endif  // CEVF_EXTRAS_H
