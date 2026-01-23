/*
 * Copyright (c) 2025 Tan Wah Ken
 *
 * License: The MIT License (MIT)
 */

#ifndef data_h
#define data_h

#include <stdlib.h>

inline static int *intdup(int a) {
  int *ans = (int *)malloc(sizeof(int));
  if (ans == NULL) return NULL;
  *ans = a;
  return ans;
}

inline static long *longdup(long a) {
  long *ans = (long *)malloc(sizeof(long));
  if (ans == NULL) return NULL;
  *ans = a;
  return ans;
}


#endif // data_h
