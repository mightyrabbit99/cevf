#ifndef CEVF_MAIN_PRE_H
#define CEVF_MAIN_PRE_H

struct cevf_run_arg_s {
  struct cevf_initialiser_s **ini_arr;
  cevf_asz_t *ini_num;
  struct cevf_producer_s *pd_arr;
  cevf_asz_t pd_num;
  struct cevf_consumer_t1_s *cm_t1_arr;
  cevf_asz_t cm_t1_num;
  struct cevf_consumer_t2_s *cm_t2_arr;
  cevf_asz_t cm_t2_num;
  uint8_t cm_thr_cnt;
  size_t ctrlmqsz;
};

int cevf_init(size_t evqsz);
int cevf_run(int argc, char *argv[], struct cevf_run_arg_s arg);
int cevf_register_signal_terminate(cevf_signal_handler handler, void *user_data);
void cevf_deinit(void);

#endif  // CEVF_MAIN_PRE_H
