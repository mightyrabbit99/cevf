/*
 * Copyright (c) 2025 mightyrabbit99
 *
 * License: The MIT License (MIT)
 */

#ifndef qmsg2_h
#define qmsg2_h

#include <time.h>
#include <stdint.h>

struct qmsg2_s;
int qmsg2_init(void);
void qmsg2_deinit(void);
struct qmsg2_s *qmsg2_new_mq(size_t sz);
void qmsg2_del_mq(struct qmsg2_s *mq);
int qmsg2_enq(struct qmsg2_s *mq, void *item);
int qmsg2_enq_soft(struct qmsg2_s *mq, void *item);
int qmsg2_deq(struct qmsg2_s *mq, void **buf);
int qmsg2_poll(struct qmsg2_s *mq, void **buf, time_t tv_sec, long tv_nsec);
int qmsg2_flush(struct qmsg2_s *mq, typeof(void(void *)) destroyer);

#define QMSG2_ERR_MQ NULL
#endif // qmsg2_h
