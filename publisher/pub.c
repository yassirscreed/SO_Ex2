#include "logging.h"
#include "request.h"
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr,"usage: pub <register_pipe_name> <pipe_name> <box_name>");
        return -1;
    }

    Register regist;
    char reg_pipe_name[256];
    int fd_pipe,reg_pipe;
    strcpy(reg_pipe_name,argv[1]);
    strcpy(regist.pipe_name,argv[2]);
    strcpy(regist.box_name,argv[3])
    regist.code = 1

    mkfifo(regist.pipe_name,0777);
    fd_pipe = open(regist.pipe_name,O_WRONLY);
    if (fd_pipe < 0) {
        perror("Error opening pipe");
        return -1;
    }
    mkfifo(reg_pipe_name,0777);
    reg_pipe = open(reg_pipe_name,O_WRONLY);
    if (reg_pipe < 0) {
        perror("Error opening pipe");
        return -1;
    }
    
   
    write(reg_pipe,&regist,sizeof(Regist));
    Response response;
    read(fd_pipe,&response,sizeof(Response));
    if(response.return_code == -1)
        {
            fprintf(stdout, "ERROR %s\n", error_message);
            return -1;
        }
    Message m;
    m.code = 9;
    while(fgets(m.message,sizeof(m.message),stdin)!= NULL){
        write(reg_pipe,&m,sizeof(Message));
    }    

    close(reg_pipe);
    close(fd_pipe);


    return 0;
}

#include "logging.h"