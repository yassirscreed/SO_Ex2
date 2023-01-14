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
    if (argc != 4) {
        fprintf(stderr,"usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
        return -1;
    }
    signal(SIGINT,end);
    Register regist;
    char reg_pipe_name[256];
    int fd_pipe,reg_pipe;
    memset(regist.named_pipe,'\0',sizeof(regist.named_pipe));
    memset(regist.box_name,'\0',sizeof(regist.box_name));
    strcpy(reg_pipe_name,argv[1]);
    strcpy(regist.named_pipe,argv[2]);
    strcpy(regist.box_name,argv[3]);
    regist.code = 2;

    mkfifo(regist.named_pipe,0777);
    
    mkfifo(reg_pipe_name,0777);
    reg_pipe = open(reg_pipe_name,O_WRONLY);
    if (reg_pipe < 0) {
        perror("Error opening pipe");
        return -1;
    }
    
    if(write(reg_pipe,&regist,sizeof(Register))==-1)
        return -1; // change
    close(reg_pipe);
    Message m;
    ssize_t bytes_read;
    fd_pipe = open(regist.named_pipe,O_RDONLY);
        if (fd_pipe < 0) {
            perror("Error opening pipe");
            return -1;
        }
    while(session_open){
       
        bytes_read = read(fd_pipe,&m,sizeof(Message));
        if(bytes_read > 0){
            fprintf(stdout, "%s\n", m.message);
            num_mens++;
            //adaptar leitura para ler todas as mensagens da caixa quando inicia e ler mensagem a mensagem
        }
    }        
    close(fd_pipe);
    unlink(regist.named_pipe);
    unlink(reg_pipe_name);
    fprintf(stdout, "%d\n", num_mens);
    

    return 0;
}
