#include "logging.h"
#include "request.h"
#include <string.h>

void response_create_remove(int fd_pipe){
    Response response;
    read(fd_pipe,&response,sizeof(Response));
    if(response.return_code == 0){
        fprintf(stdout, "OK\n");
    }
    else{
        fprintf(stdout, "ERROR %s\n", response.error_message);
    }
}

void response_list(int fd_pipe){
    Manager_list list;
    read(fd_pipe,&list,sizeof(Manager_list));
    if(list.last ==-1){
        fprintf(stdout, "NO BOXES FOUND\n");
        return;
    }
    while(list.last != 1){    
        fprintf(stdout, "%s %zu %zu %zu\n", list.box_name, list.box_size, list.n_publishers, list.n_subscribers);
        read(fd_pipe,&list,sizeof(Manager_list));
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
    char reg_pipe_name[];
    char mode[10];
    int fd_pipe,reg_pipe;

    strcpy(reg_pipe_name,argv[1]);
    strcpy(regist.pipe_name,argv[2])
    strcpy(mode,argv[3]);
    
    mkfifo(regist.pipe_name,0777);
    fd_pipe = open(regist.pipe_name,O_RDONLY);
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
    
    switch(mode):
    {
        case "create":
        strcpy(regist.box_name,argv[4]);
        regist.code = 3;
        write(reg_pipe,&regist,sizeof(Regist));
        response_create_remove(fd_pipe);
        break;

        case "remove":
        strcpy(regist.box_name,argv[4]);
        regist.code = 5;
        write(reg_pipe,&regist,sizeof(Regist));
        response_create_remove(fd_pipe);
        break;

        case "list":
        regist.code = 7;
        write(reg_pipe,&regist,sizeof(Regist));
        response_list(fd_pipe);
        break;


    }    
    close(reg_pipe);
    close(fd_pipe);


    return 0;
}
