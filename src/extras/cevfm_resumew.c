#include <cevf_mod.h>
#include <errno.h>
#include <unistd.h>

#include "cevf_extras.h"

static int resumew_resume_handle(void *data, cevf_evtyp_t evtyp) {
  int ret = 0;
  ssize_t result;
  struct cevf_resumew_write_s *rpy = (struct cevf_resumew_write_s *)data;

  result = write(rpy->sd, rpy->p, rpy->len);
  if (result < 0) {
    if (errno == EAGAIN) goto end;
    ret = -1;
    if (rpy->on_error) rpy->on_error(rpy->ctx);
    goto end;
  }

  if (result < rpy->len) {
    rpy->len -= result;
    rpy->p += result;
    cevf_generic_enqueue((void *)rpy, CEVFE_RESUMEW_WRITE_EVENT_NO);
    return 0;
  }

  delete_cevf_resumew_write_s(rpy);
  return 0;
end:
  if (rpy->on_end) rpy->on_end(rpy->ctx);
  delete_cevf_resumew_write_s(rpy);
  return ret;
}

static void mod_resumew_init(void) {
  cevf_mod_add_consumer_t1(CEVFE_RESUMEW_WRITE_EVENT_NO, resumew_resume_handle);
}

cevf_mod_init(mod_resumew_init)
