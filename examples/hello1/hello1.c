#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "cevf_mod.h"

enum _evtyp {
  evt_a1_pcap,
  evt_a1_process1,
  evt_a1_destruct,
};

#define console_log(...) printf(__VA_ARGS__)

uint8_t log_level = 0;
char *config_file_path = NULL;

static void usage(const char *argv0, int exitval, const char *errmsg) {
  FILE *fp = stdout;
  if (exitval != 0) fp = stderr;
  if (errmsg != NULL) fprintf(fp, "ERROR: %s\n\n", errmsg);
  fprintf(fp, "Usage: %s -s <config_file> \n", argv0);
  fprintf(fp, "  -s - Fuck you\n");
}

static int parse_flags(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, "vs:")) != -1) {
    switch (c) {
      case 's':
        config_file_path = strdup(optarg);
        break;
      case 'v':
        log_level++;
        break;
      default: /* '?' */
        usage(argv[0], EXIT_FAILURE, "Unknown arguments");
    }
  }

  int index;
  for (index = optind; index < argc; index++)
    printf("Non-option argument %s\n", argv[index]);

  if (!config_file_path) {
    usage(argv[0], EXIT_FAILURE, "config file required");
    return 1;
  }
  return 0;
}

static cevf_mq_t pcap_mq = NULL;
static char *pcap_string = "pcap_string";
static CEVF_THENDDECL(a1_pcap_loop, arg1) {
  cevf_qmsg_enq(pcap_mq, NULL);
}

static CEVF_THFDECL(a1_pcap_loop, arg1) {
  int i = 0;
  uint8_t *data;
  size_t datalen = 30;
  cevf_qmsg2_res_t ret;
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
    ret = cevf_qmsg_poll_nointr(pcap_mq, &pollres, 1, 0);
    if (ret == cevf_qmsg2_res_ok && pollres == NULL) {
      console_log("%s: stopping generator loop...\n", __FUNCTION__);
      break;
    }
    if (ret != cevf_qmsg2_res_timeout) {
      console_log("%s: msq poll failed!\n", __FUNCTION__);
      break;
    }
    data = (uint8_t *)malloc(datalen);
    sprintf(data, "hello world %d", i++);
    console_log("%s: Generated event: %p %ld\n", __FUNCTION__, data, datalen);
    cevf_generic_enqueue(data, evt_a1_pcap);
  }
  cevf_qmsg_del_mq(pcap_mq);
  pcap_mq = NULL;
  return NULL;
}

static int a1_pcap_handle(void *data, cevf_evtyp_t evtyp) {
  char *d = (char *)data;
  console_log("%s: %p %ld\n", __FUNCTION__, d, strlen(d));
  console_log("PCAP content=%s\n", d);
  cevf_generic_enqueue(d, evt_a1_destruct);

  uint8_t *data2;
  size_t data2len = 30;

  data2 = (uint8_t *)malloc(data2len);
  sprintf(data2, "fuck you %p", d);
  cevf_generic_enqueue(data2, evt_a1_process1);
  return 0;
}

static int a1_process1_handle(void *data, cevf_evtyp_t evtyp) {
  char *d = (char *)data;
  console_log("%s: %p %ld\n", __FUNCTION__, d, strlen(d));
  console_log("PROCESS1 content=%s\n", d);

  cevf_generic_enqueue(data, evt_a1_destruct);
  return 0;
}

static int a1_destruct_handle(void *data, cevf_evtyp_t evtyp) {
  char *d = (char *)data;
  console_log("%s: %p %ld\n", __FUNCTION__, data, strlen(d));
  free(data);
  return 0;
}

static int a1_init_1(int argc, char *argv[]) {
  console_log("Hello World!\n");
  if (parse_flags(argc, argv)) return 1;
  return 0;
}

static void a1_deinit_1(void) {
  console_log("End of the World!\n");
  free(config_file_path);
}

static void mod_hello1_init(void) {
  cevf_mod_add_initialiser(0, a1_init_1, a1_deinit_1);
  cevf_mod_add_producer(a1_pcap_loop, 0);
  cevf_mod_add_consumer_t1(evt_a1_pcap, a1_pcap_handle);
  cevf_mod_add_consumer_t1(evt_a1_process1, a1_process1_handle);
  cevf_mod_add_consumer_t1(evt_a1_destruct, a1_destruct_handle);
}

cevf_mod_init(mod_hello1_init)
