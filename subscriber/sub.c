#include "logging.h"
#include "requests.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

int num_mens = 0;
int session_open = 1;

void end(int sig){
    (void) sig;
    session_open = 0;
}

int main(int argc, char **argv) {

    signal(SIGINT,end);
    if (argc != 4) {
        fprintf(stderr,"usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
        return -1;
    }
    
    Register regist;
    char reg_pipe_name[PIPE_MAX];
    int fd_pipe,reg_pipe;

    memset(regist.named_pipe,'\0',sizeof(regist.named_pipe));
    memset(regist.box_name,'\0',sizeof(regist.box_name));
    strcpy(reg_pipe_name,argv[1]);
    strcpy(regist.named_pipe,argv[2]);
    strcpy(regist.box_name,argv[3]);
    regist.code = 2;

    if (unlink(regist.named_pipe) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", regist.named_pipe,strerror(errno));
        exit(EXIT_FAILURE); 
    }
    
    if (mkfifo(regist.named_pipe, PIPE_CODE) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    reg_pipe = open(reg_pipe_name,O_WRONLY);
    if (reg_pipe == -1) {
        fprintf(stderr,"Error opening pipe\n");
        exit(EXIT_FAILURE);
    }
    if(write(reg_pipe,&regist,sizeof(Register))==-1 || errno == EPIPE){
        fprintf(stderr,"Error writing to pipe\n");
        exit(EXIT_FAILURE);
    }

    if(close(reg_pipe) == -1){
        fprintf(stderr,"Error closing pipe\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout,"Request sent\n");

    Message m;
    fd_pipe = open(regist.named_pipe,O_RDONLY);
    if (fd_pipe == -1) {
        fprintf(stderr,"Error opening pipe\n");
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_read;
    while(session_open){
        if((bytes_read = read(fd_pipe,&m,sizeof(Message))) == 0){
            fprintf(stderr,"Error reading from pipe\n");
            break;
        }
        if(bytes_read > 0){
        fprintf(stdout, "%s\n", m.message);
        num_mens++;
        }
    }
            
    if(close(fd_pipe) == -1){
        fprintf(stderr,"Error closing pipe\n");
        exit(EXIT_FAILURE);
    }

    unlink(regist.named_pipe);
    fprintf(stdout, "%d\n", num_mens);
    

    return 0;
}
