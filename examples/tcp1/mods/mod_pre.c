#include <errno.h>
#include <stdio.h>

#include "mod.h"
#include "srvctx.h"

#define console_log(...) printf(__VA_ARGS__)

static void hij_server_read_handler(int sd, void *eloop_ctx, void *sock_ctx) {
  struct srvread_s *h = (struct srvread_s *)sock_ctx;
  char readbuf[SRVREAD_READBUF_SIZE];
  int nread = read(h->sd, readbuf, sizeof(readbuf));
  if (nread < 0) {
    fprintf(stderr, "httpread failed: %s", strerror(errno));
    goto bad;
  } else if (nread == 0) {
    goto end;
  } else {
    if (nread >= 5 && memcmp(readbuf, "hello", 5) == 0)
      write(h->sd, "world\n", 6);
    else
      write(h->sd, "fuck you\n", 9);
  }
  return;

bad:
  (*h->cb)(h, h->cookie, SRVREAD_EVENT_ERROR);
  return;
end:
  (*h->cb)(h, h->cookie, SRVREAD_EVENT_CLOSE);
}

static struct srvread_s *hij_server_read_create(int sd, void (*cb)(struct srvread_s *handle, void *cookie, enum srvread_event e), void *cookie) {
  struct srvread_s *ans = zalloc(sizeof(struct srvread_s));
  if (ans == NULL) return NULL;
  ans->sd = sd;
  ans->cb = cb;
  ans->cookie = cookie;
  if (cevf_register_sock(sd, CEVF_SOCKEVENT_TYPE_READ, hij_server_read_handler, NULL, ans)) goto fail;
  return ans;
fail:
  delete_srvread_s(ans);
  return NULL;
}

static void hij_server_read_cb(struct srvread_s *handle, void *cookie, enum srvread_event e) {
  struct srv_conn_ctx_s *srvconn = cookie;
  if (e == SRVREAD_EVENT_CLOSE) delete_srv_conn_ctx_s(srvconn);
}

static struct srv_conn_ctx_s *hij_server_sndrcv_init(struct srv_ctx_s *srv, int fd, struct sockaddr_in *addr) {
  struct srv_conn_ctx_s *srvconn = zalloc(sizeof(struct srv_conn_ctx_s));
  srvconn->srv = srv;
  srvconn->fd = fd;
  srvconn->hread = hij_server_read_create(srvconn->fd, hij_server_read_cb, srvconn);
  if (srvconn->hread == NULL) {
    delete_srv_conn_ctx_s(srvconn);
    return NULL;
  }
  return srvconn;
}

static void hij_server_conn_handler(int sd, void *eloop_ctx, void *sock_ctx) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  struct srv_ctx_s *srv = (struct srv_ctx_s *)eloop_ctx;
  int conn;

  conn = accept(srv->fd, (struct sockaddr *)&addr, &addr_len);
  if (conn < 0) {
    fprintf(stderr, "HTTP: Failed to accept new connection: %s", strerror(errno));
    return;
  }
  printf("HTTP: Connection from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

  struct srv_conn_ctx_s *req = hij_server_sndrcv_init(srv, conn, &addr);
  if (req == NULL) {
    close(conn);
    return;
  }

  req->next = srv->conns;
  srv->conns = req;
  srv->request_count++;
}

static struct srv_ctx_s *register_hij_server(int port) {
  struct srv_ctx_s *srv = (void *)zalloc(sizeof(struct srv_ctx_s));
  if (srv == NULL) return NULL;

  int on = 1;
  srv->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (srv->fd < 0) goto fail;
  if (setsockopt(srv->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) fprintf(stderr, "setsockopt\n");
  if (fcntl(srv->fd, F_SETFL, O_NONBLOCK) < 0) goto fail;
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  if (bind(srv->fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) goto fail;

  if (listen(srv->fd, 10 /* max backlog */) < 0
    || fcntl(srv->fd, F_SETFL, O_NONBLOCK) < 0
    || cevf_register_sock(srv->fd, CEVF_SOCKEVENT_TYPE_READ, hij_server_conn_handler, srv, NULL))
    goto fail;

  return srv;
fail:
  delete_srv_ctx_s(srv);
  return NULL;
}

static struct srv_ctx_s *srv = NULL;

static void root_init_1(void) {
  srv = register_hij_server(8081);
}

static void root_deinit_1(void) {
  delete_srv_ctx_s(srv);
}

static void mod_pre_init(void) {
  cevf_mod_add_initialiser(root_init_1, root_deinit_1);
}

cevf_mod_init(mod_pre_init)
