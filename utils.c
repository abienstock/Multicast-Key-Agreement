#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

void die_with_error(char *error_msg) {
  perror(error_msg);
  exit(1);
}

#ifndef assert
#include <execinfo.h>
void assert(bool cond, char *msg) {
  if (!cond) {
    // begin stack backtrace
    // by https://www.gnu.org/software/libc/manual/html_node/Backtraces.html
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace (array, 10);
    strings = backtrace_symbols (array, size);

    // printf ("Obtained %zd stack frames.\n", size);

    for (i = 0; i < size; i++)
      printf ("%s\n", strings[i]);

    free (strings);
    // end stack backtrace

    die_with_error(msg);
  }
}
#endif

void *malloc_check(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL)
    die_with_error("malloc returned NULL");
  return ptr;
}
