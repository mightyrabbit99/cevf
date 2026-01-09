#include <stdio.h>
#include <stdlib.h>

#include "mod.h"

#define console_log(...) printf(__VA_ARGS__)

static uint8_t cnt1 = 0;
static void _produce_ev_1(void *ctx) {
  uint8_t *data;
  size_t datalen = 30;
  data = (uint8_t *)malloc(datalen);
  sprintf(data, "hello world %d", cnt1++);
  console_log("%s: Generated event: %p %ld\n", __FUNCTION__, data, datalen);
  cevf_generic_enqueue(data, datalen, evt_a1_pcap);
  cevf_register_timeout(1, 0, _produce_ev_1, NULL);
}

static int a1_init_1(void) {
  console_log("Hello World!\n");
  return 0;
}

static void a1_deinit_1(void) {
  console_log("End of the World!\n");
}

static int b1_init_1(void) {
  return cevf_register_timeout(1, 0, _produce_ev_1, NULL);
}

static void b1_deinit_1(void) {

}

static void mod_pre_init(void) {
  cevf_mod_add_initialiser(0, a1_init_1, a1_deinit_1);
  cevf_mod_add_initialiser(1, b1_init_1, b1_deinit_1);
}

cevf_mod_init(mod_pre_init)
