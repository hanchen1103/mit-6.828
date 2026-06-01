#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

static int
isalnum(char c)
{
  return ('0' <= c && c <= '9') ||
         ('A' <= c && c <= 'Z') ||
         ('a' <= c && c <= 'z');
}

static int
looks_like_secret(char *s, char *end)
{
  int n = 0;

  while(s + n < end && n < 32 && isalnum(s[n]))
    n++;

  return n > 0 && s + n < end && s[n] == 0;
}

int
main(int argc, char *argv[])
{
  char *marker = "This may help.";
  int len = strlen(marker);
  char *base = sbrk(32 * PGSIZE);
  char *end = base + 32 * PGSIZE;

  if(base == SBRK_ERROR)
    exit(1);

  for(char *p = base; p + len < end; p++){
    if(memcmp(p, marker, len) == 0 && looks_like_secret(p + 16, end)){
      printf("%s\n", p + 16);
      exit(0);
    }
  }

  exit(1);
}
