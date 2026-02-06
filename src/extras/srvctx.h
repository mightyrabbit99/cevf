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

inline static struct srvread_s *new_srvread_s(int sd, void (*cb)(struct srvread_s *handle, void *cookie, enum srvread_event e), void *cookie) {
  struct srvread_s *ans = (struct srvread_s *)zalloc(sizeof(struct srvread_s));
  if (ans == NULL) return NULL;
  ans->sd = sd;
  ans->cb = cb;
  ans->cookie = cookie;
  return ans;
}

inline static void destroy_srvread_s(struct srvread_s *h) {
  if (h == NULL) return;
}

inline static void delete_srvread_s(struct srvread_s *h) {
  if (h == NULL) return;
  destroy_srvread_s(h);
  free(h);
}

struct srv_ctx_s {
  int fd;
  int request_count;
  struct srv_conn_ctx_s *conns;
  void *d1;
};

struct srv_conn_ctx_s {
  struct srv_ctx_s *srv;
  int fd;
  struct srvread_s *hread;
  struct srv_conn_ctx_s *next;
};

inline static struct srv_conn_ctx_s *new_src_conn_ctx_s(struct srv_ctx_s *srv, int fd) {
  struct srv_conn_ctx_s *ans = (struct srv_conn_ctx_s *)zalloc(sizeof(struct srv_conn_ctx_s));
  if (ans == NULL) goto fail;
  ans->srv = srv;
  ans->fd = fd;
  return ans;
fail:
  free(ans);
  return NULL;
}

inline static void destroy_srv_conn_ctx_s(struct srv_conn_ctx_s *srvconn) {
  if (srvconn == NULL) return;
}

inline static void destruct_srv_conn_ctx_s(struct srv_conn_ctx_s *srvconn) {
  if (srvconn == NULL) return;
  close(srvconn->fd);
  delete_srvread_s(srvconn->hread);
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
  destroy_srv_conn_ctx_s(srvconn);
}

inline static void delete_srv_conn_ctx_s(struct srv_conn_ctx_s *srvconn) {
  if (srvconn == NULL) return;
  destroy_srv_conn_ctx_s(srvconn);
  free(srvconn);
}

inline static void erase_srv_conn_ctx_s(struct srv_conn_ctx_s *srvconn) {
  if (srvconn == NULL) return;
  destruct_srv_conn_ctx_s(srvconn);
  free(srvconn);
}

inline static void link_srvconn_rw(struct srv_conn_ctx_s *srvconn, struct srvread_s *hread) {
  srvconn->hread = hread;
}

inline static void link_srv_srvconn(struct srv_ctx_s *srv, struct srv_conn_ctx_s *srvconn) {
  srvconn->next = srv->conns;
  srv->conns = srvconn;
  srv->request_count++;
}

inline static void srv_conn_free_all(struct srv_conn_ctx_s *head) {
  struct srv_conn_ctx_s *prev;
  while (head) {
    prev = head;
    head = head->next;
    erase_srv_conn_ctx_s(prev);
  }
}

inline static void destroy_srv_ctx_s(struct srv_ctx_s *srv) {
  if (srv == NULL) return;
}

inline static void destruct_srv_ctx_s(struct srv_ctx_s *srv, typeof(void(int)) fd_close_handle) {
  if (srv == NULL) return;
  fd_close_handle(srv->fd);
  srv_conn_free_all(srv->conns);
}

inline static void erase_srv_ctx_s(struct srv_ctx_s *srv, typeof(void(int)) fd_close_handle) {
  destruct_srv_ctx_s(srv, fd_close_handle);
  free(srv);
}

inline static void delete_srv_ctx_s(struct srv_ctx_s *srv) {
  destroy_srv_ctx_s(srv);
  free(srv);
}

inline static struct srv_ctx_s *new_srv_ctx_s(void *d1) {
  struct srv_ctx_s *srv = (void *)zalloc(sizeof(struct srv_ctx_s));
  if (srv == NULL) return NULL;
  srv->d1 = d1;
  return srv;
}

#endif // SRVCTX_H
