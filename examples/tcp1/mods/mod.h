#ifndef MOD_H
#define MOD_H
#include "cevf_mod.h"
#include <stdlib.h>
#include <string.h>

enum _evtyp {
  evt_a1_toreply,
};

struct sock_toreply_s {
  int sd;
  char *rcvdata;
  size_t rcvdata_len;
};

inline static struct sock_toreply_s *new_sock_toreply_s(char *readbuf, size_t len, int sd) {
  struct sock_toreply_s *ans = (struct sock_toreply_s *)malloc(sizeof(struct sock_toreply_s));
  if (ans == NULL) return NULL;
  ans->rcvdata_len = len;
  ans->rcvdata = (char *)malloc(ans->rcvdata_len);
  if (ans->rcvdata == NULL) goto fail;
  memcpy(ans->rcvdata, readbuf, ans->rcvdata_len);
  ans->sd = sd;
  return ans;
fail:
  free(ans);
  return NULL;
}

inline static void delete_sock_toreply_s(struct sock_toreply_s *s) {
  free(s->rcvdata);
  free(s);
}

#endif // MOD_H
