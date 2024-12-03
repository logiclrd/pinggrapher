#include <signal.h>
#include <stdio.h>

int quit;

void restore_cursor(int signal)
{
  printf("\nGot SIGINT\n\x1B[?25h");
  quit = 1;
}

int main()
{
  printf("\x1B[?25l");

  signal(SIGINT, restore_cursor);

  while (!quit)
    getc(stdin);

  return 0;
}

