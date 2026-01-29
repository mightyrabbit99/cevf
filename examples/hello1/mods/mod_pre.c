#include <stdio.h>
#include "mod.h"

#define console_log(...) printf(__VA_ARGS__)

static int a1_init_1(int argc, char *argv[]) {
  console_log("Hello World!\n");
  return 0;
}

static void a1_deinit_1(void) {
  console_log("End of the World!\n");
}

static void mod_pre_init(void) {
  cevf_mod_add_initialiser(0, a1_init_1, a1_deinit_1);
}

cevf_mod_init(mod_pre_init)
