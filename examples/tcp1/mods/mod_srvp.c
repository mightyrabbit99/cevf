#include <errno.h>
#include <stdio.h>

#include "mod.h"
#include "srvctx.h"

#define lge(...) fprintf(stderr, __VA_ARGS__)
#define WRITEBUF_SZ 15

static int a1_reply_handle(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  int ret = 0;
  ssize_t result;
  struct sock_toreply_s *rpy = (struct sock_toreply_s *)data;
  struct srv_conn_ctx_s *srvconn = (struct srv_conn_ctx_s *)rpy->ctx;
  char writebuf[WRITEBUF_SZ];
  size_t writesz;
  if (rpy->rcvdata_len >= 5 && memcmp(rpy->rcvdata, "hello", 5) == 0)
    memcpy(writebuf, "world\n", writesz = 6);
  else
    memcpy(writebuf, "fuck you\n", writesz = 9);

  result = write(rpy->sd, writebuf, writesz);
  if (result < 0) {
    if (errno == EAGAIN) goto end;
    ret = -1;
    goto end;
  }

end:
  delete_sock_toreply_s(rpy);
  return ret;
}

static void mod_srvp_init(void) {
  cevf_mod_add_consumer(evt_a1_toreply, a1_reply_handle);
}

cevf_mod_init(mod_srvp_init)

