#ifndef mod_h
#define mod_h
#include "cevf_mod.h"

enum _evtyp {
  _reserved_start,
  evt_a1_pcap,
  evt_a1_process1,
  evt_a1_destruct,
  _reserved_end = 255,
};
#endif // mod_h
