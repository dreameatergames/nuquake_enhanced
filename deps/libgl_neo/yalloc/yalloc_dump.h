#pragma once

#include "yalloc_internals.h"

#include <stdio.h>

static void printOffset(void * pool, char * name, uint16_t offset)
{
  if (isNil(offset))
    printf("  %s: nil\n", name);
  else
    printf("  %s: %u\n", name, (uintptr_t)HDR_PTR(offset) - (uintptr_t)pool);
}

void yalloc_dump(void * pool, char * name)
{
  printf("---- %s ----\n", name);
  Header * cur = (Header*)pool;
  for (;;)
  {
    printf(isFree(cur) ? "%u: free @0x%08x\n" : "%u: used @0x%08x\n", (uintptr_t)cur - (uintptr_t)pool, cur);
    printOffset(pool, cur == pool ? "first free" : "prev", cur->prev);
    printOffset(pool, "next", cur->next);
    if (isFree(cur))
    {
      printOffset(pool, "prevFree", cur[1].prev);
      printOffset(pool, "nextFree", cur[1].next);
    }
    else
      printf("  payload includes padding: %i\n", isPadded(cur));

    if (isNil(cur->next))
      break;

    printf("  %u bytes payload\n", (uintptr_t)HDR_PTR(cur->next) - (uintptr_t)cur - sizeof(Header));

    cur = HDR_PTR(cur->next);
  }

  fflush(stdout);
}
