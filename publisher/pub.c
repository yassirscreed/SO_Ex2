#include "logging.h"
#include "requests.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>");
    return -1;
  }

  Register regist;
  char reg_pipe_name[PIPE_MAX];
  int fd_pipe, reg_pipe;

  memset(regist.named_pipe, '\0', sizeof(regist.named_pipe));
  memset(regist.box_name, '\0', sizeof(regist.box_name));
  strcpy(reg_pipe_name, argv[1]);
  strcpy(regist.named_pipe, argv[2]);
  strcpy(regist.box_name, argv[3]);
  regist.code = 1;

  if (unlink(regist.named_pipe) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", regist.named_pipe,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (mkfifo(regist.named_pipe, PIPE_CODE) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  reg_pipe = open(reg_pipe_name, O_WRONLY);
  if (reg_pipe < 0) {
    fprintf(stderr, "Error opening pipe\n");
    exit(EXIT_FAILURE);
  }

  if (write(reg_pipe, &regist, sizeof(Register)) == -1 || errno == EPIPE) {
    fprintf(stderr, "Error writing to pipe\n");
    exit(EXIT_FAILURE);
  }

  if (close(reg_pipe) == -1) {
    fprintf(stderr, "Error closing pipe\n");
    exit(EXIT_FAILURE);
  }

  fprintf(stdout, "Request sent\n");

  Message m;
  m.code = 9;
  fd_pipe = open(regist.named_pipe, O_WRONLY);
  if (fd_pipe < 0) {
    fprintf(stderr, "Error opening pipe\n");
    exit(EXIT_FAILURE);
  }

  while (fgets(m.message, sizeof(m.message), stdin) != NULL) {
    m.message[strlen(m.message) - 1] = '\0';
    if (write(fd_pipe, &m, sizeof(Message)) == -1 || errno == EPIPE) {
      fprintf(stderr, "Error writing to pipe\n");
      exit(EXIT_FAILURE);
    }
  }

  if (close(fd_pipe) == -1) {
    fprintf(stderr, "Error closing pipe\n");
    exit(EXIT_FAILURE);
  }
  unlink(regist.named_pipe);
  return 0;
}
