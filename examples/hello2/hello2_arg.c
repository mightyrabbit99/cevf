#include "hello2_arg.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

uint8_t log_level = 0;
char *config_file_path = NULL;

static void usage(const char *argv0, int exitval, const char *errmsg) {
  FILE *fp = stdout;
  if (exitval != 0) fp = stderr;
  if (errmsg != NULL) fprintf(fp, "ERROR: %s\n\n", errmsg);
  fprintf(fp, "Usage: %s -s <config_file> \n", argv0);
  fprintf(fp, "  -s - Fuck you\n");
  exit(exitval);
}

static void parse_flags(int argc, char **argv) {
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
  }
}

__attribute__((section(".init_array"))) static void *_parse_flags = &parse_flags;
static void __attribute__((destructor)) _destruct_1(void) {
  free(config_file_path);
}

