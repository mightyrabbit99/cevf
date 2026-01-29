#ifndef TCP1_H
#define TCP1_H

#include <string.h>

enum _evtyp {
  evt_a1_toreply,
};

struct sock_toreply_s {
  int sd;
  char *rcvdata;
  size_t rcvdata_len;
  void *ctx;
};

inline static struct sock_toreply_s *new_sock_toreply_s(char *readbuf, size_t len, int sd, void *ctx) {
  struct sock_toreply_s *ans = (struct sock_toreply_s *)malloc(sizeof(struct sock_toreply_s));
  if (ans == NULL) return NULL;
  ans->rcvdata_len = len;
  ans->rcvdata = (char *)malloc(ans->rcvdata_len);
  if (ans->rcvdata == NULL) goto fail;
  memcpy(ans->rcvdata, readbuf, ans->rcvdata_len);
  ans->sd = sd;
  ans->ctx = ctx;
  return ans;
fail:
  free(ans);
  return NULL;
}

inline static void delete_sock_toreply_s(struct sock_toreply_s *s) {
  free(s->rcvdata);
  free(s);
}

#endif // TCP1_H
