#include "cevf_mod.h"
#include "cevf_extras.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#define CEVFE_TCPSRV_RCV_EVENT_NO 0
#define WRITEBUF_SZ 15

static int a1_reply_handle(void *data, cevf_evtyp_t evtyp) {
  int ret = 0;
  ssize_t result;
  struct cevf_tcpsrv_rcv_s *rpy = (struct cevf_tcpsrv_rcv_s *)data;

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
  delete_cevf_tcpsrv_rcv_s(rpy);
  return ret;
}

static void mod_reply1_init(void) {
  cevf_mod_add_consumer_t1(CEVFE_TCPSRV_RCV_EVENT_NO, a1_reply_handle);
}

cevf_mod_init(mod_reply1_init)
