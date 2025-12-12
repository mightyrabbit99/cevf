#include <stdio.h>
#include <stdlib.h>

#include "mod.h"

#define console_log(...) printf(__VA_ARGS__)

static cevf_mq_t pcap_mq = NULL;
static char *pcap_string = "pcap_string";
static CEVF_THENDDECL(a1_pcap_loop, arg1) {
  cevf_qmsg_enq(pcap_mq, NULL);
}

static CEVF_THFDECL(a1_pcap_loop, arg1) {
  int i = 0;
  uint8_t *data;
  size_t datalen = 30;
  console_log("%s: starting generator loop...\n", __FUNCTION__);
  pcap_mq = cevf_qmsg_new_mq(5);
  if (pcap_mq == NULL) {
    console_log("%s: msq init failed!\n", __FUNCTION__);
    return NULL;
  }
  //for (int j = 0;j < 4;j++) {
  for (;;) {
    void *pollres = pcap_string;
    console_log("%s: polling\n", __FUNCTION__);
    if (cevf_qmsg_poll(pcap_mq, &pollres, 1)) {
      console_log("%s: msq poll failed!\n", __FUNCTION__);
      break;
    }
    if (pollres == NULL) {
      console_log("%s: stopping generator loop...\n", __FUNCTION__);
      break;
    }
    data = (uint8_t *)malloc(datalen);
    sprintf(data, "hello world %d", i++);
    console_log("%s: Generated event: %p %ld\n", __FUNCTION__, data, datalen);
    cevf_generic_enqueue(data, datalen, evt_a1_pcap);
  }
  cevf_qmsg_del_mq(pcap_mq);
  pcap_mq = NULL;
  return NULL;
}

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
  cevf_mod_add_producer(a1_pcap_loop, 0);
  cevf_mod_add_consumer(evt_a1_pcap, a1_pcap_handle);
  cevf_mod_add_consumer(evt_a1_process1, a1_process1_handle);
  cevf_mod_add_consumer(evt_a1_destruct, a1_destruct_handle);
}

cevf_mod_init(mod_kern_init)
