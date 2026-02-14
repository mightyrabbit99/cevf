#include <cevf_mod.h>
#include "cevf_extras.h"

#include "assdis.h"

#ifndef CEVFE_ASSDISASS_MAX_SEGMENT_SIZE
#define CEVFE_ASSDISASS_MAX_SEGMENT_SIZE 100
#endif  // CEVFE_ASSDISASS_MAX_SEGMENT_SIZE
#ifndef CEVFE_ASSDISASS_GC_TIMEOUT_SEC
#define CEVFE_ASSDISASS_GC_TIMEOUT_SEC 10
#endif  // CEVFE_ASSDISASS_GC_TIMEOUT_SEC


static size_t cevfe_asmb_max_sgmt_size = CEVFE_ASSDISASS_MAX_SEGMENT_SIZE;

struct assdis_assembler_ctx_s *asmb_assembler_ctx = NULL;
struct assdis_disassembler_ctx_s *asmb_disassembler_ctx = NULL;

static void _asmb_periodic_procedure(void *ctx) {
  assdis_periodic_exec(asmb_assembler_ctx, asmb_disassembler_ctx);
  cevf_register_timeout(CEVFE_ASSDISASS_GC_TIMEOUT_SEC, 0, _asmb_periodic_procedure, NULL);
}

static int asmb_sgmt_in_handle(void *data, cevf_evtyp_t evtyp) {
  int ans;
  ans = assdis_assemble(asmb_assembler_ctx, (struct assdis_frag_s *)data, cevfe_asmb_max_sgmt_size);
  delete_cevf_assdisass_disdata_s((struct cevf_assdisass_disdata_s *)data);
  return ans;
}

static int asmb_chunk_in_handle(void *data, cevf_evtyp_t evtyp) {
  int ret;
  ret = assdis_disassemble(asmb_disassembler_ctx, (struct assdis_data_s *)data, cevfe_asmb_max_sgmt_size);
  if (ret) {
    cevf_generic_enqueue(data, evtyp);
  } else {
    delete_cevf_assdisass_disdata_s((struct cevf_assdisass_disdata_s *)data);
  }
  return 0;
}

static int asmb_init(int argc, char *argv[]) {
  if (!cevf_is_static()) {
    char *env_str;
    env_str = getenv("CEVFE_ASSDISASS_MAX_SEGMENT_SIZE");
    if (env_str) cevfe_asmb_max_sgmt_size = atoi(env_str);
  }
  asmb_assembler_ctx = assdis_new_assembler_ctx();
  if (asmb_assembler_ctx == NULL) return 1;
  asmb_disassembler_ctx = assdis_new_disassembler_ctx();
  if (asmb_disassembler_ctx == NULL) return 1;
  if (cevf_register_timeout(CEVFE_ASSDISASS_GC_TIMEOUT_SEC, 0, _asmb_periodic_procedure, NULL)) return 1;
  return 0;
}

static void asmb_deinit(void) {
  assdis_delete_assembler_ctx(asmb_assembler_ctx);
  assdis_delete_disassembler_ctx(asmb_disassembler_ctx);
}

static void mod_assdisass_init(void) {
  cevf_mod_add_initialiser(0, asmb_init, asmb_deinit);
  cevf_mod_add_consumer_t1(CEVFE_ASSDISASS_SEGMENT_IN_EVENT_NO, asmb_sgmt_in_handle);
  cevf_mod_add_consumer_t1(CEVFE_ASSDISASS_BIGCHUNK_IN_EVENT_NO, asmb_chunk_in_handle);
}

cevf_mod_init(mod_assdisass_init)
