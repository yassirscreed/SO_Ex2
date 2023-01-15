#include "logging.h"
#include "requests.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

int compare(const void* a, const void* b) {
    Manager_list a1 = *(Manager_list *) a;
    Manager_list b1 = *(Manager_list *) b;
    return strcmp(a1.box_name,b1.box_name);
}

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
    Manager_list list[64],m;
    int i=0,j;
    if(read(fd_pipe,&m,sizeof(Manager_list))==0)
            return; //add
    while(m.last != 1){
        if(read(fd_pipe,&m,sizeof(Manager_list))==0)
            return; //add
        list[i] = m;  //podem sair todos duplicados~
        i++;
    }

    if(i == 0){
        fprintf(stdout, "NO BOXES FOUND\n"); //fix para dar em ordem alfabética
        return;
    }
    else{
        size_t size = (size_t) i;
        qsort(list, size, sizeof(Manager_list), compare);
        for(j=0;j<=i;j++){ //might segfault
            fprintf(stdout, "%s %llu %llu %llu\n", list[i].box_name, list[i].box_size, list[i].n_pubs, list[i].n_subs);
        }
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
