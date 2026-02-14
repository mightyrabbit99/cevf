#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <cevf_mod.h>
#include "cevf_extras.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#define CEVFM_SOCKSPLICE_SPLICE_SZ 8
#define CEVFM_SOCKSPLICE_SPLICE_FLAVOUR SPLICE_F_MOVE

static cevf_pcdno_t cevfe_socksplicer_splice_pcdno = CEVFE_SOCKSPLICER_SPLICE_PROCD_NO;
static cevf_pcdno_t cevfe_socksplicer_close_pcdno = CEVFE_SOCKSPLICER_CLOSE_PROCD_NO;

static inline int _spleis(int in_fd, int out_fd, int pip[2]) {
  int ret;
  ret = splice(in_fd, NULL, pip[1], NULL, CEVFM_SOCKSPLICE_SPLICE_SZ, CEVFM_SOCKSPLICE_SPLICE_FLAVOUR);
  if (ret == -1) {
    perror("splice(1)");
    return ret;
  }

  ret = splice(pip[0], NULL, out_fd, NULL, CEVFM_SOCKSPLICE_SPLICE_SZ, CEVFM_SOCKSPLICE_SPLICE_FLAVOUR);
  if (ret == -1) {
    perror("splice(1)");
    return ret;
  }

  return 0;
}

struct _sspli_retstru_s {
  int pip_to;
  int pip_fro;
  int fds1[2];
  int fds2[2];
};

static void *sspli_close_sock(int argc, va_list argp) {
  if (argc < 1)
    return NULL;
  struct _sspli_retstru_s *s = (struct _sspli_retstru_s *)va_arg(argp, void *);

  close(s->fds1[0]);
  close(s->fds1[1]);
  close(s->fds2[0]);
  close(s->fds2[1]);

  free(s);
  return (void *)s;
}

#define _close_s(a) \
  do {              \
    if ((a) > 0)    \
      close(a);     \
  } while (0)
static void *sspli_splice_sock(int argc, va_list argp) {
  if (argc < 2)
    return NULL;
  int sd = va_arg(argp, int);
  int dd = va_arg(argp, int);

  struct _sspli_retstru_s *ans = (struct _sspli_retstru_s *)malloc(sizeof(struct _sspli_retstru_s));
  if (ans == NULL) goto fail;
  *ans = (struct _sspli_retstru_s){0};
  if (pipe(ans->fds1) == -1) {
    perror("pipe");
    goto fail;
  }
  if (pipe(ans->fds2) == -1) {
    perror("pipe");
    goto fail;
  }
  if (_spleis(sd, dd, ans->fds1)) {
    goto fail;
  }
  if (_spleis(dd, sd, ans->fds2)) {
    goto fail;
  }

  ans->pip_to = ans->fds1[1];
  ans->pip_fro = ans->fds2[1];

  return (void *)(struct cevf_socksplicer_spliceres_s *)ans;
fail:
  _close_s(ans->fds1[0]);
  _close_s(ans->fds1[1]);
  _close_s(ans->fds2[0]);
  _close_s(ans->fds2[1]);
  free(ans);
  return NULL;
}

static int sspli_init(int argc, char *argv[]) {
  if (!cevf_is_static()) {
    char *env_str;
    env_str = getenv("CEVFE_SOCKSPLICER_SPLICE_PROCD_NO");
    if (env_str)
      cevfe_socksplicer_splice_pcdno = atoi(env_str);
    env_str = getenv("CEVFE_SOCKSPLICER_CLOSE_PROCD_NO");
    if (env_str)
      cevfe_socksplicer_close_pcdno = atoi(env_str);
  }
  cevf_add_procedure_t1((struct cevf_procedure_t1_s){
      .pcdno = cevfe_socksplicer_splice_pcdno,
      .synchronized = 0,
      .vfunc = sspli_splice_sock,
  });
  cevf_add_procedure_t1((struct cevf_procedure_t1_s){
      .pcdno = cevfe_socksplicer_close_pcdno,
      .synchronized = 0,
      .vfunc = sspli_close_sock,
  });
  return 0;
}

static void sspli_deinit(void) {
  return;
}

static void mod_socksplicer_init(void) {
  cevf_mod_add_initialiser(0, sspli_init, sspli_deinit);
}

cevf_mod_init(mod_socksplicer_init)
