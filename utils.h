void die_with_error(char *error_msg);

#include <stdbool.h>
void assert(bool cond, char *msg);

void *malloc_check(size_t size);
