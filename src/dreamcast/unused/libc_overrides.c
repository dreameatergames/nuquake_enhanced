#include <quakedef.h>
#include <stddef.h>

char *strerror(int a)
{
  (void)a;
  return NULL;
}
char *_strerror_r(struct _reent *a, int b, int c, int *d)
{
  (void)a;
  (void)b;
  (void)c;
  (void)d;
  return NULL;
}

void handle_libc_overrides(void)
{
  strerror(0);
  _strerror_r(NULL, 0, 0, NULL);
}