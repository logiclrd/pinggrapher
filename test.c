#include <curses.h>
#include <stdio.h>
#include <term.h>

int main()
{
  char buffer[10000];

  char *ptr = &buffer[0];

  char *poop = tgetstr("cl", NULL);

  while (*poop != 0)
    printf("%02X ", *(poop++));
  printf("\n");

  ptr = &buffer[0];

  poop = tgetstr("vi", NULL);

  while (*poop != 0)
    printf("%02X ", *(poop++));
  printf("\n");

  return 0;
}

