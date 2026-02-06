#include <cevf_mod.h>

#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#include "cevf_extras.h"
#include "srvctx.h"
#include "cvector.h"
#include "cvector_utils.h"

#define lge(...) fprintf(stderr, __VA_ARGS__)
#define lg(...) (NULL)

#ifndef CEVFE_USOCK_RCV_EVENT_NO
#define CEVFE_USOCK_RCV_EVENT_NO 0
#endif  // CEVFE_USOCK_RCV_EVENT_NO
#ifndef CEVFE_USOCK_CLOSE_EVENT_NO
#define CEVFE_USOCK_CLOSE_EVENT_NO CEVF_RESERVED_EV_THEND
#endif  // CEVFE_USOCK_CLOSE_EVENT_NO
#ifndef CEVFE_USOCK_CLOSE_PROCD_NO
#define CEVFE_USOCK_CLOSE_PROCD_NO 0
#endif  // CEVFE_USOCK_CLOSE_PROCD_NO
#ifndef CEVFE_USOCK_CLOSESRV_PROCD_NO
#define CEVFE_USOCK_CLOSESRV_PROCD_NO 1
#endif  // CEVFE_USOCK_CLOSESRV_PROCD_NO
#ifndef CEVFE_USOCK_OPENSRV_PROCD_NO
#define CEVFE_USOCK_OPENSRV_PROCD_NO 2
#endif  // CEVFE_USOCK_OPENSRV_PROCD_NO
#ifndef CEVFE_USOCK_READBUF_SIZE
#define CEVFE_USOCK_READBUF_SIZE 4096
#endif  // CEVFE_USOCK_READBUF_SIZE

static char *cevfe_usock_sock_list_str = NULL;
static cevf_evtyp_t cevfe_usock_rcv_evno = CEVFE_USOCK_RCV_EVENT_NO;
static cevf_evtyp_t cevfe_usock_close_evno = CEVFE_USOCK_CLOSE_EVENT_NO;
static cevf_pcdno_t cevfe_usock_close_pcdno = CEVFE_USOCK_CLOSE_PROCD_NO;
static cevf_pcdno_t cevfe_usock_closesrv_pcdno = CEVFE_USOCK_CLOSESRV_PROCD_NO;
static cevf_pcdno_t cevfe_usock_opensrv_pcdno = CEVFE_USOCK_OPENSRV_PROCD_NO;

static void fd_close_handle(int fd) {
  if (fd < 0) return;
  cevf_unregister_sock(fd, CEVF_SOCKEVENT_TYPE_READ);
  close(fd);
}

static void *usock_close_sock(int argc, va_list argp) {
  if (argc < 1) return NULL;
  int sd = va_arg(argp, int);
  fd_close_handle(sd);
  return NULL;
}

static inline int _for_each_item_in_strarr(const char *strarr, const char *sep, int (*f)(const char *, void *), void *ctx) {
  int ret = 0;
  if (strarr == NULL) return 0;
  char *p = strdup(strarr);
  if (p == NULL) return -1;
  char *p2 = p, *token;
  while ((token = strsep(&p2, sep)) != NULL) {
    ret = f(token, ctx);
    if (ret) break;
  }
  free(p);
  return ret;
}

static void usock_close_conn(struct srv_conn_ctx_s *srvconn) {
  fd_close_handle(srvconn->fd);
  erase_srv_conn_ctx_s(srvconn);
}

static void usock_server_read_handler(int sd, void *eloop_ctx, void *sock_ctx) {
  struct srvread_s *h = (struct srvread_s *)eloop_ctx;
  char *sock = (char *)sock_ctx;
  struct srv_conn_ctx_s *srvconn = (struct srv_conn_ctx_s *)h->cookie;
  char readbuf[CEVFE_USOCK_READBUF_SIZE];
  int nread = read(sd, readbuf, sizeof(readbuf));
  if (nread < 0) {
    lge("USOCK: read failed: %s\n", strerror(errno));
    goto bad;
  } else if (nread == 0) {
    goto end;
  } else {
    struct cevf_usock_rcv_s *rpy = new_cevf_usock_rcv_s(readbuf, nread, sd, sock);
    if (rpy == NULL) return;
    cevf_generic_enqueue((void *)rpy, cevfe_usock_rcv_evno);
  }
  return;

bad:
  (*h->cb)(h, h->cookie, SRVREAD_EVENT_ERROR);
  return;
end:
  (*h->cb)(h, h->cookie, SRVREAD_EVENT_CLOSE);
}

static void usock_server_read_cb(struct srvread_s *handle, void *cookie, enum srvread_event e) {
  struct srv_conn_ctx_s *srvconn = (struct srv_conn_ctx_s *)cookie;
  if (e == SRVREAD_EVENT_CLOSE) {
    if (
      cevfe_usock_close_evno != cevfe_usock_rcv_evno
      && cevfe_usock_close_evno != CEVF_RESERVED_EV_THEND
    ) {
      cevf_generic_enqueue((void *)(uintptr_t)srvconn->fd, cevfe_usock_close_evno);
    }
    usock_close_conn(srvconn);
  }
}

static struct srv_conn_ctx_s *usock_server_sndrcv_init(struct srv_ctx_s *srv, int fd) {
  struct srv_conn_ctx_s *srvconn = new_src_conn_ctx_s(srv, fd);
  if (srvconn == NULL) goto fail;
  struct srvread_s *hread = new_srvread_s(fd, usock_server_read_cb, srvconn);
  if (hread == NULL) goto fail;
  if (cevf_register_sock(fd, CEVF_SOCKEVENT_TYPE_READ, usock_server_read_handler, hread, srv->d1)) {
    lge("USOCK: register read socket failed\n");
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

static void usock_server_conn_handler(int sd, void *eloop_ctx, void *sock_ctx) {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  struct srv_ctx_s *srv = (struct srv_ctx_s *)eloop_ctx;
  int conn;

  conn = accept(srv->fd, (struct sockaddr *)&addr, &addr_len);
  if (conn < 0) {
    lge("USOCK: failed to accept new connection: %s\n", strerror(errno));
    return;
  }
  lg("USOCK: connection from %s\n", (char *)srv->d1);

  if (usock_server_sndrcv_init(srv, conn) == NULL)
    close(conn);
}

static struct srv_ctx_s *usock_register_server(const char *sock) {
  char *sock2 = strdup(sock);
  if (sock2 == NULL) return NULL;
  struct srv_ctx_s *srv = new_srv_ctx_s(sock2);
  if (srv == NULL) goto fail;

  if (access(sock, F_OK) == 0 && unlink(sock)) {
    lge("unlink errno=%d (%s), sock=%s\n", errno, strerror(errno), sock);
    goto fail;
  }

  if ((srv->fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    lge("socket errno=%d (%s), sock=%s\n", errno, strerror(errno), sock);
    goto fail;
  }

  int option = 1;
  setsockopt(srv->fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, sock);

  if (bind(srv->fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    lge("bind errno=%d (%s), sock=%s\n", errno, strerror(errno), sock);
    goto fail2;
  }

  if (listen(srv->fd, 10) < 0) {
    lge("listen errno=%d (%s), sock=%s\n", errno, strerror(errno), sock);
    goto fail2;
  }

  if (fcntl(srv->fd, F_SETFL, O_NONBLOCK) < 0) {
    lge("fnctl errno=%d (%s), sock=%s\n", errno, strerror(errno), sock);
    goto fail2;
  }

  if (cevf_register_sock(srv->fd, CEVF_SOCKEVENT_TYPE_READ, usock_server_conn_handler, srv, NULL)) {
    lge("cevf_register_sock error for %s\n", sock);
    goto fail2;
  }

  return srv;
fail2:
  close(srv->fd);
fail:
  delete_srv_ctx_s(srv);
  free(sock2);
  return NULL;
}

static int _setup_add_usock_to_sockarr(const char *sock, void *ctx) {
  cvector_vector_type(struct srv_ctx_s *) *sockarr = (cvector_vector_type(struct srv_ctx_s *) *)ctx;
  struct srv_ctx_s *srv = usock_register_server(sock);
  if (srv == NULL) return -1;
  cvector_push_back(*sockarr, srv);
  return 0;
}

static int parse_flags(int argc, char **argv) {
  int c;
  while ((c = getopt(argc, argv, "u:")) != -1) {
    switch (c) {
      case 'u':
        cevfe_usock_sock_list_str = strdup(optarg);
        break;
    }
  }
  return 0;
}

static cvector_vector_type(struct srv_ctx_s *) sockarr = NULL;
static pthread_mutex_t sockarr_mutex;

static void *usock_close_srvsock(int argc, va_list argp) {
  if (argc < 1) return NULL;
  char *sockname = va_arg(argp, char *);
  struct srv_ctx_s **it;
  cvector_vector_type(struct srv_ctx_s *) sockarr2 = NULL;
  cvector_vector_type(struct srv_ctx_s *) sockarr3 = NULL;
  pthread_mutex_lock(&sockarr_mutex);
  cvector_for_each_in(it, sockarr) {
    if (*it == NULL) continue;
    if (strcmp((char *)(*it)->d1, sockname)) {
      cvector_push_back(sockarr2, *it);
    } else {
      cvector_push_back(sockarr3, *it);
    }
  }
  cvector_free(sockarr);
  sockarr = sockarr2;
  pthread_mutex_unlock(&sockarr_mutex);
  cvector_for_each_in(it, sockarr3) {
    free((*it)->d1);
    erase_srv_ctx_s(*it, fd_close_handle);
  }
  cvector_free(sockarr3);
  return NULL;
}

static void *usock_open_srvsock(int argc, va_list argp) {
  if (argc < 1) return NULL;
  char *sock = va_arg(argp, char *);
  struct srv_ctx_s *srv = usock_register_server(sock);
  if (srv == NULL) return  NULL;
  pthread_mutex_lock(&sockarr_mutex);
  cvector_push_back(sockarr, srv);
  pthread_mutex_unlock(&sockarr_mutex);
  return NULL;
}

static int usock_init_1(int argc, char *argv[]) {
  pthread_mutex_init(&sockarr_mutex, NULL);
  if (cevf_is_static()) {
    parse_flags(argc, argv);
  } else {
    char *env_str;
    env_str = getenv("CEVFE_USOCK_SOCKS");
    if (env_str) cevfe_usock_sock_list_str = strdup(env_str);
    env_str = getenv("CEVFE_USOCK_RCV_EVENT_NO");
    if (env_str) cevfe_usock_rcv_evno = atoi(env_str);
    env_str = getenv("CEVFE_USOCK_CLOSE_EVENT_NO");
    if (env_str) cevfe_usock_close_evno = atoi(env_str);
    env_str = getenv("CEVFE_USOCK_CLOSE_PROCD_NO");
    if (env_str) cevfe_usock_close_pcdno = atoi(env_str);
    env_str = getenv("CEVFE_USOCK_CLOSESRV_PROCD_NO");
    if (env_str) cevfe_usock_closesrv_pcdno = atoi(env_str);
    env_str = getenv("CEVFE_USOCK_OPENSRV_PROCD_NO");
    if (env_str) cevfe_usock_opensrv_pcdno = atoi(env_str);
  }
  cevf_add_procedure_t1((struct cevf_procedure_t1_s){
      .pcdno = cevfe_usock_close_pcdno,
      .synchronized = 0,
      .vfunc = usock_close_sock,
  });
  cevf_add_procedure_t1((struct cevf_procedure_t1_s){
      .pcdno = cevfe_usock_closesrv_pcdno,
      .synchronized = 0,
      .vfunc = usock_close_srvsock,
  });
  cevf_add_procedure_t1((struct cevf_procedure_t1_s){
      .pcdno = cevfe_usock_opensrv_pcdno,
      .synchronized = 0,
      .vfunc = usock_open_srvsock,
  });
  return _for_each_item_in_strarr(cevfe_usock_sock_list_str, ":", _setup_add_usock_to_sockarr, &sockarr);
}

static void usock_deinit_1(void) {
  free(cevfe_usock_sock_list_str);
  struct srv_ctx_s **it;
  cvector_for_each_in(it, sockarr) {
    free((*it)->d1);
    erase_srv_ctx_s(*it, fd_close_handle);
  }
  cvector_free(sockarr);
  pthread_mutex_destroy(&sockarr_mutex);
}

static void mod_usock_init(void) {
  cevf_mod_add_initialiser(0, usock_init_1, usock_deinit_1);
}

cevf_mod_init(mod_usock_init)
