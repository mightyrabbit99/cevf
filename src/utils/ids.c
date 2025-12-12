/*
 * Copyright (c) 2025 mightyrabbit99
 *
 * License: The MIT License (MIT)
 */

#include "ids.h"

#include <pthread.h>
#include <stdlib.h>

struct ids_s {
  void **item_arr;
  size_t item_arr_len;
  ids_id_t *freeid_arr;
  size_t freeid_arr_len;
  pthread_mutex_t ids_mut;
};

struct ids_s *ids_new_idsys(void) {
  struct ids_s *ans = (struct ids_s *)malloc(sizeof(struct ids_s));
  *ans = (struct ids_s){0};
  if (pthread_mutex_init(&ans->ids_mut, NULL)) goto fail;
  return ans;
fail:
  free(ans);
  return NULL;
}

void ids_free_idsys(struct ids_s *idsys) {
  if (idsys == NULL) return;
  free(idsys->item_arr);
  free(idsys->freeid_arr);
  pthread_mutex_destroy(&idsys->ids_mut);
  free(idsys);
}

ids_id_t ids_add_item(struct ids_s *idsys, void *item) {
  ids_id_t id = -1;
  void **newarr;
  if (idsys == NULL) return id;
  if (item == NULL) return id;
  pthread_mutex_lock(&idsys->ids_mut);
  if (idsys->freeid_arr_len == 0) {
    newarr = (void **)realloc(idsys->item_arr, (idsys->item_arr_len + 1) * sizeof(void *));
    if (newarr == NULL) {
      pthread_mutex_unlock(&idsys->ids_mut);
      return id;
    }
    id = idsys->item_arr_len++;
    idsys->item_arr = newarr;
    idsys->item_arr[id] = item;
  } else {
    id = idsys->freeid_arr[idsys->freeid_arr_len-- - 1];
    idsys->item_arr[id] = item;
  }
  pthread_mutex_unlock(&idsys->ids_mut);
}

void *ids_get_item(struct ids_s *idsys, ids_id_t id) {
  void *ans = NULL;
  if (idsys == NULL) return ans;
  pthread_mutex_lock(&idsys->ids_mut);
  ans = idsys->item_arr[id];
  pthread_mutex_unlock(&idsys->ids_mut);
  return ans;
}

int ids_rm_item(struct ids_s *idsys, ids_id_t id) {
  ids_id_t *newarr;
  if (idsys == NULL) return -1;
  pthread_mutex_lock(&idsys->ids_mut);
  newarr = (ids_id_t *)realloc(idsys->freeid_arr, (idsys->freeid_arr_len + 1) * sizeof(ids_id_t));
  if (newarr == NULL) {
    pthread_mutex_unlock(&idsys->ids_mut);
    return -1;
  }
  idsys->freeid_arr = newarr;
  idsys->freeid_arr[idsys->freeid_arr_len++] = id;
  idsys->item_arr[id] = NULL;
  pthread_mutex_unlock(&idsys->ids_mut);
}

void ids_foreach(struct ids_s *idsys, typeof(void(void *)) f) {
  if (idsys == NULL) return;
  for (size_t i = 0; i < idsys->item_arr_len; i++) {
    f(idsys->item_arr[i]);
  }
}
