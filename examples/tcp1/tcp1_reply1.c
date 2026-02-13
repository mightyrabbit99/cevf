#include "cevf_mod.h"
#include "cevf_extras.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

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

  struct cevf_resumew_write_s *rsm = new_cevf_resumew_write_s(rpy->sd, writebuf, writesz, NULL, NULL, NULL);
  cevf_generic_enqueue((void *)rsm, CEVFE_RESUMEW_WRITE_EVENT_NO);

  delete_cevf_tcpsrv_rcv_s(rpy);
  return 0;
}

static void mod_reply1_init(void) {
  cevf_mod_add_consumer_t1(CEVFE_TCPSRV_RCV_EVENT_NO, a1_reply_handle);
}

cevf_mod_init(mod_reply1_init)
