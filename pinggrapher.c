#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <termios.h>

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
  char *target = "4.2.2.4";

  if (argc > 1)
    target = argv[1];

  char *cmdline = malloc(strlen(target) + 50);

  strcpy(cmdline, "ping -A -i 2 -W 3000 ");
  strcat(cmdline, target);

  FILE *ping = popen(cmdline, "r");

  int buffer_length = 100;
  char *buffer = malloc(buffer_length);

  int num_columns = 80;
  int num_rows = 24;

  struct winsize ws;

  if (tcgetwinsize(0, &ws) >= 0)
  {
    num_columns = ws.ws_col;
    num_rows = ws.ws_row - 1;
  }

  int *columns = (int *)malloc(num_columns * sizeof(int));
  char *screen_buffer = (char *)malloc(num_columns * num_rows + 1);

  memset(columns, 0, num_columns * sizeof(columns[0]));

  int graph_scale = 2000;
  int auto_graph_scale = 0;

  if (argc > 2)
  {
    auto_graph_scale = atoi(argv[2]);

    if (auto_graph_scale > 0)
    {
      graph_scale = auto_graph_scale;
      auto_graph_scale = 0;
    }
    else
      auto_graph_scale = 1;
  }

  printf("\x1B[?25l\x1B[=1S\x1B[2J");
  fflush(stdout);

  signal(SIGINT, restore_cursor);

  while (!time_to_quit)
  {
    if (readline(ping, &buffer, &buffer_length))
    {
#define LOST_PACKET 5000

      char *time = strstr(buffer, "time=");
      char *alarm = strchr(buffer, '\a');

      if (time || alarm)
      {
        if (tcgetwinsize(0, &ws) >= 0)
        {
          if (num_columns != ws.ws_col)
          {
            int *new_columns = (int *)malloc(ws.ws_col * sizeof(int));

            int min(int a, int b) { return (a < b) ? a : b; }

            int copy_columns = min(num_columns, ws.ws_col);

            memset(new_columns, 0, ws.ws_col * sizeof(new_columns[0]));
            memcpy(&new_columns[ws.ws_col - copy_columns], &columns[num_columns - copy_columns], copy_columns * sizeof(columns[0]));

            free(columns);
            columns = new_columns;
          }

          if (num_rows != ws.ws_row - 1)
          {
            num_rows = ws.ws_row - 1;

            free(screen_buffer);
            screen_buffer = malloc(num_columns * num_rows + 1);
          }
        }

        int i, j;

        memmove(&columns[0], &columns[1], (num_columns - 1) * sizeof(columns[0]));
        columns[num_columns - 1] = alarm ? LOST_PACKET : atoi(time + 5);

        if (auto_graph_scale)
        {
          graph_scale = 0;

          for (int i=0; i < num_columns; i++)
            if ((columns[i] > graph_scale) && (columns[i] != LOST_PACKET))
              graph_scale = columns[i];
        }

        char screen_buffer[num_columns * num_rows + 1];

        for (i=0; i < num_rows; i++)
        {
          char *line_buffer = &screen_buffer[i * num_columns];

          int cutoff = graph_scale * (num_rows - i) / num_rows;

          for (j=0; j < num_columns; j++)
            line_buffer[j] = (columns[j] == LOST_PACKET) ? 'O' : (columns[j] && (columns[j] >= cutoff)) ? '#' : ' ';

          if (i == 0)
          {
            char last = line_buffer[num_columns - 1];

            char time_string[15];

            int time_string_length = sprintf(time_string, columns[num_columns - 1] == LOST_PACKET ? " LOST " : " %d ", columns[num_columns - 1]);

            memcpy(&line_buffer[num_columns - 1 - time_string_length], time_string, time_string_length);
          }
        }

        screen_buffer[num_columns * num_rows] = 0;

        printf("\x1B[H%s", screen_buffer);
        fflush(stdout);
      }
    }
  }

  return 0;
}

