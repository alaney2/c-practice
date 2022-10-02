#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 80
char buffer[BUFFER_SIZE];

int main(int argc, char **argv) {
  int fd = STDIN_FILENO;
  if (argc == 2) {
    fd = open(*(argv+1), O_RDONLY);
    if (fd == -1) {
      char *part1 = "cat: ";
      char *part2 = ": No such file or directory";
      int msg_len = strlen(part1) + strlen(*(argv+1)) + strlen(part2) + 1;
      char err_msg[msg_len];
      strcpy(err_msg, part1);
      strcat(err_msg, *(argv+1));
      strcat(err_msg, part2);
      err_msg[msg_len-1] = '\n';
      write(STDERR_FILENO, err_msg, msg_len);
      exit(EXIT_FAILURE);
    }
  }

  ssize_t bytes_read;
  while ((bytes_read = read(fd, buffer, BUFFER_SIZE))) {
    write(STDOUT_FILENO, buffer, bytes_read);
  }

  close(fd);
  exit(0);
}