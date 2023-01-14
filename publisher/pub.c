#include "logging.h"
#include "requests.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr,"usage: pub <register_pipe_name> <pipe_name> <box_name>");
        return -1;
    }

    Register regist;
    char reg_pipe_name[256];
    int fd_pipe,reg_pipe;
    memset(regist.named_pipe,'\0',sizeof(regist.named_pipe));
    memset(regist.box_name,'\0',sizeof(regist.box_name));
    strcpy(reg_pipe_name,argv[1]);
    strcpy(regist.named_pipe,argv[2]);
    strcpy(regist.box_name,argv[3]);
    regist.code = 1;

    mkfifo(regist.named_pipe,0777);
    mkfifo(reg_pipe_name,0777);
    reg_pipe = open(reg_pipe_name,O_WRONLY);
    if (reg_pipe < 0) {
        perror("Error opening pipe");
        return -1;
    }
   
    if(write(reg_pipe,&regist,sizeof(Register)) == 0)
        return -1; //change
    close(reg_pipe);
    Message m;
    m.code = 9;
    fd_pipe = open(regist.named_pipe,O_WRONLY);
        if (fd_pipe < 0) {
            perror("Error opening pipe");
            return -1;
        }
    while(fgets(m.message,sizeof(m.message),stdin)!= NULL){  
        if(write(fd_pipe,&m,sizeof(Message)) ==0)
            return -1; //change
        
    }    
    close(fd_pipe);
    unlink(regist.named_pipe);
    unlink(reg_pipe_name);
    return 0;
}
