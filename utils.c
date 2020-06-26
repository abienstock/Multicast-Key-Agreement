#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

void die_with_error(char *error_msg) {
  perror(error_msg);
  exit(1);
}

void *malloc_check(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL)
    die_with_error("malloc returned NULL");
  return ptr;
}
