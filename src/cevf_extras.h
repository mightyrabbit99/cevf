#ifndef CEVF_EXTRAS_H
#define CEVF_EXTRAS_H

#include <stdlib.h>
#include <string.h>

#ifndef CEVF_RESERVED_EV_THEND
#define CEVF_RESERVED_EV_THEND ((cevf_evtyp_t) - 1)
#endif  // CEVF_RESERVED_EV_THEND
#define CEVFE_TCPSRV_RCV_EVENT_NO (CEVF_RESERVED_EV_THEND - 1)
#define CEVFE_TCPSRV_CLOSE_EVENT_NO (CEVF_RESERVED_EV_THEND - 2)
#define CEVFE_USOCK_RCV_EVENT_NO (CEVF_RESERVED_EV_THEND - 3)
#define CEVFE_USOCK_CLOSE_EVENT_NO (CEVF_RESERVED_EV_THEND - 4)
#define CEVFE_RESUMEW_WRITE_EVENT_NO (CEVF_RESERVED_EV_THEND - 5)

#ifndef CEVF_RESERVED_PCDNO_END
#define CEVF_RESERVED_PCDNO_END ((cevf_pcdno_t) - 1)
#endif  // CEVF_RESERVED_PCDNO_END
#define CEVFE_TCPSRV_CLOSE_PROCD_NO (CEVF_RESERVED_PCDNO_END - 0)
#define CEVFE_USOCK_CLOSE_PROCD_NO (CEVF_RESERVED_PCDNO_END - 1)
#define CEVFE_USOCK_CLOSESRV_PROCD_NO (CEVF_RESERVED_PCDNO_END - 2)
#define CEVFE_USOCK_OPENSRV_PROCD_NO (CEVF_RESERVED_PCDNO_END - 3)

struct cevf_tcpsrv_rcv_s {
  int sd;
  char *rcvdata;
  size_t rcvdata_len;
  struct {
    unsigned short af;
    const void *addr; // struct sockaddr_in or struct sockaddr_in6 depending on af
  } src;
};

static inline void destroy_cevf_tcpsrv_rcv_s(struct cevf_tcpsrv_rcv_s *s) {
  if (s == NULL) return;
  free(s->rcvdata);
}

static inline void delete_cevf_tcpsrv_rcv_s(struct cevf_tcpsrv_rcv_s *s) {
  if (s == NULL) return;
  destroy_cevf_tcpsrv_rcv_s(s);
  free(s);
}

static inline struct cevf_tcpsrv_rcv_s *new_cevf_tcpsrv_rcv_s(char *readbuf, size_t len, int sd, unsigned short af, void *addr) {
  struct cevf_tcpsrv_rcv_s *ans = (struct cevf_tcpsrv_rcv_s *)malloc(sizeof(struct cevf_tcpsrv_rcv_s));
  if (ans == NULL) return NULL;
  ans->rcvdata_len = len;
  ans->rcvdata = (char *)malloc(ans->rcvdata_len);
  if (ans->rcvdata == NULL) goto fail;
  memcpy(ans->rcvdata, readbuf, ans->rcvdata_len);
  ans->sd = sd;
  ans->src.af = af;
  ans->src.addr = addr;
  return ans;
fail:
  free(ans);
  return NULL;
}

struct cevf_resumew_write_s {
  int sd;
  void *data;
  size_t len;
  char *p;
  void (*on_error)(void *);
  void (*on_end)(void *);
  void *ctx;
};

static inline void destroy_cevf_resumew_write_s(struct cevf_resumew_write_s *s) {
  if (s == NULL) return;
  free(s->data);
}

static inline void delete_cevf_resumew_write_s(struct cevf_resumew_write_s *s) {
  if (s == NULL) return;
  destroy_cevf_resumew_write_s(s);
  free(s);
}

static inline struct cevf_resumew_write_s *new_cevf_resumew_write_s(int sd, const void *data, size_t len, void *ctx, void (*on_error)(void *), void (*on_end)(void *)) {
  struct cevf_resumew_write_s *ans = (struct cevf_resumew_write_s *)malloc(sizeof(struct cevf_resumew_write_s));
  if (ans == NULL) return NULL;
  ans->sd = sd;
  ans->data = malloc(len);
  if (ans->data == NULL) goto fail;
  memcpy(ans->data, data, len);
  ans->len = len;
  ans->p = ans->data;
  ans->on_error = on_error;
  ans->on_end = on_end;
  ans->ctx = ctx;
  return ans;
fail:
  free(ans->data);
  free(ans);
  return NULL;
}

struct cevf_usock_rcv_s {
  int sd;
  char *rcvdata;
  size_t rcvdata_len;
  const char *sockname;
};

static inline void destroy_cevf_usock_rcv_s(struct cevf_usock_rcv_s *s) {
  if (s == NULL) return;
  free(s->rcvdata);
}
static inline void delete_cevf_usock_rcv_s(struct cevf_usock_rcv_s *s) {
  if (s == NULL) return;
  destroy_cevf_usock_rcv_s(s);
  free(s);
}

static inline struct cevf_usock_rcv_s *new_cevf_usock_rcv_s(char *readbuf, size_t len, int sd, const char *sockname) {
  struct cevf_usock_rcv_s *ans = (struct cevf_usock_rcv_s *)malloc(sizeof(struct cevf_usock_rcv_s));
  if (ans == NULL) return NULL;
  ans->rcvdata_len = len;
  ans->rcvdata = (char *)malloc(ans->rcvdata_len);
  if (ans->rcvdata == NULL) goto fail;
  memcpy(ans->rcvdata, readbuf, ans->rcvdata_len);
  ans->sd = sd;
  ans->sockname = sockname;
  return ans;
fail:
  free(ans);
  return NULL;
}

#endif  // CEVF_EXTRAS_H
