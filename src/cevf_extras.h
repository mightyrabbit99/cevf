#ifndef CEVF_EXTRAS_H
#define CEVF_EXTRAS_H

#include <stdlib.h>
#include <string.h>

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

struct cevf_tcpsrv_write_resume_s {
  int sd;
  void *data;
  size_t len;
  char *p;
};

static inline void destroy_cevf_tcpsrv_write_resume_s(struct cevf_tcpsrv_write_resume_s *s) {
  if (s == NULL) return;
  free(s->data);
}

static inline void delete_cevf_tcpsrv_write_resume_s(struct cevf_tcpsrv_write_resume_s *s) {
  if (s == NULL) return;
  destroy_cevf_tcpsrv_write_resume_s(s);
  free(s);
}

static inline struct cevf_tcpsrv_write_resume_s *new_cevf_tcpsrv_write_resume_s(int sd, const void *data, size_t len) {
  struct cevf_tcpsrv_write_resume_s *ans = (struct cevf_tcpsrv_write_resume_s *)malloc(sizeof(struct cevf_tcpsrv_write_resume_s));
  if (ans == NULL) return NULL;
  ans->sd = sd;
  ans->data = malloc(len);
  if (ans->data == NULL) goto fail;
  memcpy(ans->data, data, len);
  ans->len = len;
  ans->p = ans->data;
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
