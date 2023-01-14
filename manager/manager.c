#include "logging.h"
#include "../requests.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

void response_create_remove(int fd_pipe){
    Response response;
    if(read(fd_pipe,&response,sizeof(Response))==0)
        return; //add
    if(response.return_code == 0){
        fprintf(stdout, "OK\n");
    }
    else{
        fprintf(stdout, "ERROR %s\n", response.error_message);
    }
}

void response_list(int fd_pipe){
    Manager_list list;
    if(read(fd_pipe,&list,sizeof(Manager_list))==0)
        return; //add
    if(list.last == 1){
        fprintf(stdout, "NO BOXES FOUND\n"); //fix para dar em ordem alfabética
        return;
    }
    while(list.last != 1){    
        fprintf(stdout, "%d %zu %zu %zu\n", list.box_name, list.box_size, list.n_pubs, list.n_subs);
        if(read(fd_pipe,&list,sizeof(Manager_list)) ==0)
            return; //add
    }

}


int main(int argc, char **argv) {
    if (argc != 4   && argc != 5) {
        fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
        return -1;
    }

    Register regist;
    char reg_pipe_name[256];
    char mode[10];
    int fd_pipe,reg_pipe;
    
    strcpy(reg_pipe_name,argv[1]);
    memset(regist.named_pipe,'\0',sizeof(regist.named_pipe));
    strcpy(regist.named_pipe,argv[2]);
    strcpy(mode,argv[3]);
    
    mkfifo(regist.named_pipe,0777);
    fd_pipe = open(regist.named_pipe,O_RDONLY);
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
    
    switch(mode[0])
    {
        case 'c':
        memset(regist.box_name,'\0',sizeof(regist.box_name));
        strcpy(regist.box_name,argv[4]);
        regist.code = 3;
        if(write(reg_pipe,&regist,sizeof(Register)) == 0)
            return -1; //change
        response_create_remove(fd_pipe);
        break;

        case 'r':
        memset(regist.box_name,'\0',sizeof(regist.box_name));
        strcpy(regist.box_name,argv[4]);
        regist.code = 5;
        if(write(reg_pipe,&regist,sizeof(Register)) == 0)
            return -1; //change
        response_create_remove(fd_pipe);
        break;

        case 'l':
        regist.code = 7;
        if(write(reg_pipe,&regist,sizeof(Register)) == 0)
            return -1; //change
        response_list(fd_pipe);
        break;
        default:
        break;


    }    
    close(reg_pipe);
    close(fd_pipe);
    unlink(regist.named_pipe);
    unlink(reg_pipe_name);

    return 0;
}
