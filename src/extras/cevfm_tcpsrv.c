#include <cevf_mod.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#include "srvctx.h"
#include "cevf_extras.h"

#define lge(...) fprintf(stderr, __VA_ARGS__)
#define lg(...) (NULL)

#ifndef CEVFE_TCPSRV_RCV_EVENT_NO
#define CEVFE_TCPSRV_RCV_EVENT_NO 0
#endif  // CEVFE_TCPSRV_RCV_EVENT_NO
#ifndef CEVFE_TCPSRV_CLOSE_EVENT_NO
#define CEVFE_TCPSRV_CLOSE_EVENT_NO CEVF_RESERVED_EV_THEND
#endif  // CEVFE_TCPSRV_CLOSE_EVENT_NO
#ifndef CEVFE_TCPSRV_DEFAULT_PORT
#define CEVFE_TCPSRV_DEFAULT_PORT 8081
#endif  // CEVFE_TCPSRV_DEFAULT_PORT
#ifndef CEVFE_TCPSRV_READBUF_SIZE
#define CEVFE_TCPSRV_READBUF_SIZE 4096
#endif  // CEVFE_TCPSRV_READBUF_SIZE

static void fd_close_handle(int fd) {
  if (fd < 0) return;
  cevf_unregister_sock(fd, CEVF_SOCKEVENT_TYPE_READ);
  close(fd);
}

static void *tcpsrv_close_sock(int argc, va_list argp) {
  if (argc < 1) return NULL;
  int sd = va_arg(argp, int);
  fd_close_handle(sd);
  return NULL;
}

static void tcpsrv_close_conn(struct srv_conn_ctx_s *srvconn) {
  fd_close_handle(srvconn->fd);
  erase_srv_conn_ctx_s(srvconn);
}

static void tcpsrv_server_read_handler(int sd, void *eloop_ctx, void *sock_ctx) {
  struct srvread_s *h = (struct srvread_s *)eloop_ctx;
  struct srv_conn_ctx_s *srvconn = (struct srv_conn_ctx_s *)h->cookie;
  char readbuf[CEVFE_TCPSRV_READBUF_SIZE];
  int nread = read(sd, readbuf, sizeof(readbuf));
  if (nread < 0) {
    lge("TCPSRV: read failed: %s\n", strerror(errno));
    goto bad;
  } else if (nread == 0) {
    goto end;
  } else {
    struct cevf_tcpsrv_rcv_s *rpy = new_cevf_tcpsrv_rcv_s(readbuf, nread, sd, (cevf_tcpsrv_on_close_t)tcpsrv_close_conn, srvconn);
    if (rpy == NULL) return;
    cevf_generic_enqueue((void *)rpy, CEVFE_TCPSRV_RCV_EVENT_NO);
  }
  return;

bad:
  (*h->cb)(h, h->cookie, SRVREAD_EVENT_ERROR);
  return;
end:
  (*h->cb)(h, h->cookie, SRVREAD_EVENT_CLOSE);
}

static void tcpsrv_server_read_cb(struct srvread_s *handle, void *cookie, enum srvread_event e) {
  struct srv_conn_ctx_s *srvconn = (struct srv_conn_ctx_s *)cookie;
  if (e == SRVREAD_EVENT_CLOSE) {
    if (
      CEVFE_TCPSRV_CLOSE_EVENT_NO != CEVFE_TCPSRV_RCV_EVENT_NO
      && CEVFE_TCPSRV_CLOSE_EVENT_NO != CEVF_RESERVED_EV_THEND
    ) {
      cevf_generic_enqueue((void *)(uintptr_t)srvconn->fd, CEVFE_TCPSRV_CLOSE_EVENT_NO);
    }
    tcpsrv_close_conn(srvconn);
  }
}

static struct srv_conn_ctx_s *tcpsrv_server_sndrcv_init(struct srv_ctx_s *srv, int fd) {
  struct srv_conn_ctx_s *srvconn = new_src_conn_ctx_s(srv, fd);
  if (srvconn == NULL) goto fail;
  struct srvread_s *hread = new_srvread_s(fd, tcpsrv_server_read_cb, srvconn);
  if (hread == NULL) goto fail;
  if (cevf_register_sock(fd, CEVF_SOCKEVENT_TYPE_READ, tcpsrv_server_read_handler, hread, NULL)) {
    lge("TCPSRV: register read socket failed\n");
    goto fail;
  }

  link_srvconn_rw(srvconn, hread);
  link_srv_srvconn(srv, srvconn);
  return srvconn;
fail:
  close(fd);
  delete_srvread_s(hread);
  delete_srv_conn_ctx_s(srvconn);
  return NULL;
}

static void tcpsrv_server_conn_handler(int sd, void *eloop_ctx, void *sock_ctx) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  struct srv_ctx_s *srv = (struct srv_ctx_s *)eloop_ctx;
  int conn;

  conn = accept(srv->fd, (struct sockaddr *)&addr, &addr_len);
  if (conn < 0) {
    lge("TCPSRV: failed to accept new connection: %s\n", strerror(errno));
    return;
  }
  lg("TCPSRV: connection from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

  if (tcpsrv_server_sndrcv_init(srv, conn) == NULL)
    close(conn);
}

static struct srv_ctx_s *tcpsrv_register_tcp_server(int port) {
  struct srv_ctx_s *srv = new_srv_ctx_s();
  if (srv == NULL) return NULL;

  int on = 1;
  srv->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (srv->fd < 0) goto fail;
  if (setsockopt(srv->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    lge("setsockopt\n");
  if (fcntl(srv->fd, F_SETFL, O_NONBLOCK) < 0) goto fail;
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  if (bind(srv->fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) goto fail;

  if (
    listen(srv->fd, 10 /* max backlog */) < 0
    || fcntl(srv->fd, F_SETFL, O_NONBLOCK) < 0
    || cevf_register_sock(srv->fd, CEVF_SOCKEVENT_TYPE_READ, tcpsrv_server_conn_handler, srv, NULL)
  )
    goto fail;

  return srv;
fail:
  close(srv->fd);
  delete_srv_ctx_s(srv);
  return NULL;
}

static int port = 0;

static int parse_flags(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, "p:")) != -1) {
    switch (c) {
      case 'p':
        port = atoi(optarg);
        break;
    }
  }
  return 0;
}

static struct srv_ctx_s *srv = NULL;

static int tcpsrv_init_1(int argc, char *argv[]) {
  if (cevf_is_static()) {
    parse_flags(argc, argv);
  } else {
    char *port_str = getenv("CEVF_SRV_PORT");
    if (port_str) port = atoi(port_str);
  }
  port = port > 0 && port < 65536 ? port : CEVFE_TCPSRV_DEFAULT_PORT;
  srv = tcpsrv_register_tcp_server(port);
  if (srv == NULL) return -1;
  return 0;
}

static void tcpsrv_deinit_1(void) {
  erase_srv_ctx_s(srv, fd_close_handle);
}

static void mod_tcpsrv_init(void) {
  cevf_mod_add_initialiser(0, tcpsrv_init_1, tcpsrv_deinit_1);
  // cevf_mod_add_procedure_t1(0, tcpsrv_close_sock);
}

cevf_mod_init(mod_tcpsrv_init)
