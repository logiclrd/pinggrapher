#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int readline(FILE *stream, char **buffer, int *buffer_length)
{
  int buffer_offset = 0;

  while (1)
  {
    int ch = fgetc(stream);

    if (ch == '\n')
    {
      (*buffer)[buffer_offset] = 0;
      return 1;
    }
    else if (ch < 0)
      return 0;
    else
    {
      (*buffer)[buffer_offset] = ch;

      buffer_offset++;

      if (buffer_offset >= *buffer_length)
      {
        int new_buffer_length = *buffer_length * 2;
        char *new_buffer = malloc(new_buffer_length);

        memcpy(new_buffer, *buffer, *buffer_length);
        free(*buffer);

        *buffer = new_buffer;
        *buffer_length = new_buffer_length;
      }

      if (ch == '\a')
      {
        (*buffer)[buffer_offset] = 0;
        return 1;
      }
    }
  }
}

int time_to_quit;

void restore_cursor(int signal)
{
  printf("\x1B[?25h\x1B[=S");
  time_to_quit = 1;
}

int main(int argc, char *argv[])
{
  FILE *ping = popen("ping -A -i 2 -W 3000 24.78.128.1", "r");

  int buffer_length = 100;
  char *buffer = malloc(buffer_length);

  int columns[80];

  memset(columns, 0, sizeof(columns));

  printf("\x1B[?25l\x1B[=1S\x1B[2J");
  fflush(stdout);

  signal(SIGINT, restore_cursor);

  while (!time_to_quit)
  {
    if (readline(ping, &buffer, &buffer_length))
    {
      char *time = strstr(buffer, "time=");
      char *alarm = strchr(buffer, '\a');

      if (time || alarm)
      {
        int i, j;

        memmove(&columns[0], &columns[1], sizeof(columns) - sizeof(columns[0]));
        columns[79] = alarm ? 2000 : atoi(time + 5);

        char screen_buffer[80 * 24 + 1];

        for (i=0; i < 24; i++)
        {
          char *line_buffer = &screen_buffer[i * 80];

          int cutoff = 2000 * (24 - i) / 24;

          for (j=0; j < 80; j++)
            line_buffer[j] = (columns[j] > cutoff) ? '#' : ' ';
        }

        screen_buffer[80 * 24] = 0;

        printf("\x1B[H%s", screen_buffer);
        fflush(stdout);
      }
    }
  }

  return 0;
}

