void die_with_error(char *error_msg);

#ifndef assert
#include <stdbool.h>
void assert(bool cond, char *msg);
#endif

void *malloc_check(size_t size);
