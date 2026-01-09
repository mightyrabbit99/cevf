/*
Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "heap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define DEFAULT_CAPACITY 13

struct heap_s {
  /* size of array */
  unsigned int size;
  /* items within heap */
  unsigned int count;
  /**  user data */
  const void *udata;
  int (*cmp)(const void *, const void *, const void *);
  void *array[];
};

size_t heap_sizeof(unsigned int size) { return sizeof(heap_t) + size * sizeof(void *); }

static int __child_left(const int idx) { return idx * 2 + 1; }

static int __child_right(const int idx) { return idx * 2 + 2; }

static int __parent(const int idx) { return (idx - 1) / 2; }

void heap_init(heap_t *h, int (*cmp)(const void *, const void *, const void *udata), const void *udata, unsigned int size) {
  h->cmp = cmp;
  h->udata = udata;
  h->size = size;
  h->count = 0;
}

heap_t *heap_new(int (*cmp)(const void *, const void *, const void *udata), const void *udata) {
  heap_t *h = malloc(heap_sizeof(DEFAULT_CAPACITY));

  if (!h) return NULL;

  heap_init(h, cmp, udata, DEFAULT_CAPACITY);

  return h;
}

void heap_free(heap_t *h) { free(h); }

/**
 * @return a new heap on success; NULL otherwise */
static heap_t *__ensurecapacity(heap_t *h) {
  if (h->count < h->size) return h;

  h->size *= 2;

  return realloc(h, heap_sizeof(h->size));
}

static void __swap(heap_t *h, const int i1, const int i2) {
  void *tmp = h->array[i1];

  h->array[i1] = h->array[i2];
  h->array[i2] = tmp;
}

static int __pushup(heap_t *h, unsigned int idx) {
  /* 0 is the root node */
  while (0 != idx) {
    int parent = __parent(idx);

    /* we are smaller than the parent */
    if (h->cmp(h->array[idx], h->array[parent], h->udata) < 0)
      return -1;
    else
      __swap(h, idx, parent);

    idx = parent;
  }

  return idx;
}

static void __pushdown(heap_t *h, unsigned int idx) {
  while (1) {
    unsigned int childl, childr, child;

    childl = __child_left(idx);
    childr = __child_right(idx);

    if (childr >= h->count) {
      /* can't pushdown any further */
      if (childl >= h->count) return;

      child = childl;
    }
    /* find biggest child */
    else if (h->cmp(h->array[childl], h->array[childr], h->udata) < 0)
      child = childr;
    else
      child = childl;

    /* idx is smaller than child */
    if (h->cmp(h->array[idx], h->array[child], h->udata) < 0) {
      __swap(h, idx, child);
      idx = child;
      /* bigger than the biggest child, we stop, we win */
    } else
      return;
  }
}

static void __heap_offerx(heap_t *h, void *item) {
  h->array[h->count] = item;

  /* ensure heap properties */
  __pushup(h, h->count++);
}

int heap_offerx(heap_t *h, void *item) {
  if (h->count == h->size) return -1;
  __heap_offerx(h, item);
  return 0;
}

int heap_offer(heap_t **h, void *item) {
  if (NULL == (*h = __ensurecapacity(*h))) return -1;

  __heap_offerx(*h, item);
  return 0;
}

void *heap_poll(heap_t *h) {
  if (0 == heap_count(h)) return NULL;

  void *item = h->array[0];

  h->array[0] = h->array[h->count - 1];
  h->count--;

  if (h->count > 1) __pushdown(h, 0);

  return item;
}

void *heap_peek(const heap_t *h) {
  if (0 == heap_count(h)) return NULL;

  return h->array[0];
}

void heap_clear(heap_t *h) { h->count = 0; }

/**
 * @return item's index on the heap's array; otherwise -1 */
static int __item_get_idx(const heap_t *h, const void *item) {
  unsigned int idx;

  for (idx = 0; idx < h->count; idx++)
    if (0 == h->cmp(h->array[idx], item, h->udata)) return idx;

  return -1;
}

void *heap_remove_item(heap_t *h, const void *item) {
  int idx = __item_get_idx(h, item);

  if (idx == -1) return NULL;

  /* swap the item we found with the last item on the heap */
  void *ret_item = h->array[idx];
  h->array[idx] = h->array[h->count - 1];
  h->array[h->count - 1] = NULL;

  h->count -= 1;

  /* ensure heap property */
  __pushup(h, idx);

  return ret_item;
}

int heap_contains_item(const heap_t *h, const void *item) { return __item_get_idx(h, item) != -1; }

int heap_count(const heap_t *h) { return h->count; }

int heap_size(const heap_t *h) { return h->size; }
