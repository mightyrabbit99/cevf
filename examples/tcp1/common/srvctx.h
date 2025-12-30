#ifndef SRVCTX_H
#define SRVCTX_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "cevf.h"

#define SRVREAD_READBUF_SIZE 100

inline static void *zalloc(size_t sz) {
  return calloc(1, sz);
}

enum srvread_event {
	SRVREAD_EVENT_CLOSE = 1,        /* including reply ready */
	SRVREAD_EVENT_TIMEOUT = 2,
	SRVREAD_EVENT_ERROR = 3      /* misc. error, esp malloc error */
};

struct srvread_s {
  int sd;
  void *cookie;
  void (*cb)(struct srvread_s *handle, void *cookie,
		    enum srvread_event e);
};

inline static void destroy_srvread_s(struct srvread_s *h) {
  cevf_unregister_sock(h->sd, CEVF_SOCKEVENT_TYPE_READ);
}

inline static void delete_srvread_s(struct srvread_s *h) {
  destroy_srvread_s(h);
  free(h);
}

struct srv_ctx_s {
  int fd;
  int request_count;
  struct srv_conn_ctx_s *conns;
};

struct srv_conn_ctx_s {
  struct srv_ctx_s *srv;
  int fd;
  struct srvread_s *hread;
  struct srv_conn_ctx_s *next;
};

inline static void destroy_srv_conn_ctx_s(struct srv_conn_ctx_s *srvconn) {
  delete_srvread_s(srvconn->hread);
  close(srvconn->fd);
}

inline static void delete_srv_conn_ctx_s(struct srv_conn_ctx_s *srvconn) {
  destroy_srv_conn_ctx_s(srvconn);
  struct srv_ctx_s *srv = srvconn->srv;
  struct srv_conn_ctx_s *r = srv->conns, *r2 = NULL;
  while (r) {
    if (r == srvconn) {
      if (r2)
        r2->next = r->next;
      else
        srv->conns = r->next;
      srv->request_count--;
      break;
    }
    r2 = r;
    r = r->next;
  }
  free(srvconn);
}

inline static void srv_conn_free_all(struct srv_conn_ctx_s *head) {
  struct srv_conn_ctx_s *prev;
  while (head) {
    prev = head;
    head = head->next;
    destroy_srv_conn_ctx_s(prev);
  }
}

inline static void destroy_srv_ctx_s(struct srv_ctx_s *srv) {
  if (srv == NULL) return;
	if (srv->fd >= 0) {
    cevf_unregister_sock(srv->fd, CEVF_SOCKEVENT_TYPE_READ);
    close(srv->fd);
  }
  srv_conn_free_all(srv->conns);
}

inline static void delete_srv_ctx_s(struct srv_ctx_s *srv) {
  destroy_srv_ctx_s(srv);
  free(srv);
}

#endif // SRVCTX_H
