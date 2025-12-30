#include <stdio.h>
#include "mod.h"

#define console_log(...) printf(__VA_ARGS__)

static int a1_init_handle(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  console_log("Hello World!\n");
  return 0;
}

static int a1_end_handle(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  console_log("End of the World!\n");
  return 0;
}

static void mod_pre_init(void) {
  cevf_mod_add_consumer(_reserved_start, a1_init_handle);
  cevf_mod_add_consumer(_reserved_end, a1_end_handle);
}

cevf_mod_init(mod_pre_init)
