#include <errno.h>
#include <stdio.h>

#include "mod.h"
#include "srvctx.h"

static int a1_reply_handle(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  struct sock_toreply_s *rpy = (struct sock_toreply_s *)data;
  if (rpy->rcvdata_len >= 5 && memcmp(rpy->rcvdata, "hello", 5) == 0)
    write(rpy->sd, "world\n", 6);
  else
    write(rpy->sd, "fuck you\n", 9);

  delete_sock_toreply_s(rpy);
  return 0;
}

static void mod_srvp_init(void) {
  cevf_mod_add_consumer(evt_a1_toreply, a1_reply_handle);
}

cevf_mod_init(mod_srvp_init)

