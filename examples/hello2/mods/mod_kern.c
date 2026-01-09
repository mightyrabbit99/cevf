#include <stdio.h>
#include <stdlib.h>

#include "mod.h"

#define console_log(...) printf(__VA_ARGS__)

static int a1_pcap_handle(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  console_log("%s: %p %ld\n", __FUNCTION__, data, datalen);
  console_log("PCAP content=%s\n", data);
  cevf_generic_enqueue(data, datalen, evt_a1_destruct);

  uint8_t *data2;
  size_t data2len = 30;

  data2 = (uint8_t *)malloc(data2len);
  sprintf(data2, "fuck you %p", data);
  cevf_generic_enqueue(data2, data2len, evt_a1_process1);
  return 0;
}

static int a1_process1_handle(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  console_log("%s: %p %ld\n", __FUNCTION__, data, datalen);
  console_log("PROCESS1 content=%s\n", data);

  cevf_generic_enqueue(data, datalen, evt_a1_destruct);
  return 0;
}

static int a1_destruct_handle(uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  console_log("%s: %p %ld\n", __FUNCTION__, data, datalen);
  free(data);
  return 0;
}

static void mod_kern_init(void) {
  cevf_mod_add_consumer(evt_a1_pcap, a1_pcap_handle);
  cevf_mod_add_consumer(evt_a1_process1, a1_process1_handle);
  cevf_mod_add_consumer(evt_a1_destruct, a1_destruct_handle);
}

cevf_mod_init(mod_kern_init)
