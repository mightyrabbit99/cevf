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

static int a1_init_1(int argc, char *argv[]) {
  console_log("Hello World!\n");
  if (parse_flags(argc, argv)) return 1;
  return 0;
}

static void a1_deinit_1(void) {
  console_log("End of the World!\n");
  free(config_file_path);
}

static int b1_init_1(int argc, char *argv[]) {
  return cevf_register_timeout(1, 0, _produce_ev_1, NULL);
}

static void b1_deinit_1(void) {

}

static void mod_hello1_init(void) {
  cevf_mod_add_initialiser(0, a1_init_1, a1_deinit_1);
  cevf_mod_add_initialiser(1, b1_init_1, b1_deinit_1);
  cevf_mod_add_consumer(evt_a1_pcap, a1_pcap_handle);
  cevf_mod_add_consumer(evt_a1_process1, a1_process1_handle);
  cevf_mod_add_consumer(evt_a1_destruct, a1_destruct_handle);
}

cevf_mod_init(mod_hello1_init)
