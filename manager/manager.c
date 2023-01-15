#include "logging.h"
#include "requests.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define MODE_MAX 10

int compare(const void* a, const void* b) {
    Manager_list a1 = *(Manager_list *) a;
    Manager_list b1 = *(Manager_list *) b;
    return strcmp(a1.box_name,b1.box_name);
}

void response_create_remove(char *pipe_name){
    Response response;
    int fd_pipe;
    fd_pipe = open(pipe_name,O_RDONLY);
    if (fd_pipe == -1) {
        perror("Error opening pipe\n");
        exit(EXIT_FAILURE); 
    }
    
    if(read(fd_pipe,&response,sizeof(Response))==0){
        fprintf(stderr,"Error reading from pipe\n");
        exit(EXIT_FAILURE); 
    }

    if(response.return_code == 0){
        fprintf(stdout, "OK\n");
    }
    else{
        fprintf(stdout, "ERROR %s\n", response.error_message);
    }
    if(close(fd_pipe) == -1){
        fprintf(stderr,"Error closing pipe\n");
        exit(EXIT_FAILURE);
    }
}

void response_list(char *pipe_name){
    Manager_list list[64],m;
    int i=0,j,fd_pipe;

    fd_pipe = open(pipe_name,O_RDONLY);
    if (fd_pipe == -1) {
        perror("Error opening pipe\n");
        exit(EXIT_FAILURE); 
    }
    m.last = 0;
    while(m.last != 1){
        if(read(fd_pipe,&m,sizeof(Manager_list))==0){
            fprintf(stderr,"Error reading from pipe\n");
            exit(EXIT_FAILURE); 
        }
        if(m.last != 1){
            list[i] = m;
            i++;
        }
    }

    if(i == 0){
        fprintf(stdout, "NO BOXES FOUND\n");
        return;
    }
    else{
        size_t size = (size_t) i;
        qsort(list, size, sizeof(Manager_list), compare);
        for(j=0;j<i;j++){
            fprintf(stdout, "%s %zu %zu %zu\n", list[j].box_name, list[j].box_size, list[j].n_publishers, list[j].n_subscribers);
        }
    }

    if(close(fd_pipe) == -1){
        fprintf(stderr,"Error closing pipe\n");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char **argv) {

    if (argc != 4   && argc != 5) {
        fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
        return -1;
    }

    Register regist;
    char reg_pipe_name[PIPE_MAX];
    char mode[MODE_MAX];
    int reg_pipe;
    
    strcpy(reg_pipe_name,argv[1]);
    memset(regist.named_pipe,'\0',sizeof(regist.named_pipe));
    strcpy(regist.named_pipe,argv[2]);
    strcpy(mode,argv[3]);
    
    
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
        fprintf(stderr,"Error opening pipe1\n");
        exit(EXIT_FAILURE);
    }
    ssize_t size;
    switch(mode[0])
    {
        case 'c':
        memset(regist.box_name,'\0',sizeof(regist.box_name));
        strcpy(regist.box_name,argv[4]);
        regist.code = 3;
        if((size = write(reg_pipe,&regist,sizeof(Register))) == -1 || errno == EPIPE){
            fprintf(stderr,"Error writing to pipe\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stdout,"Request to create a box sent\n");
        response_create_remove(regist.named_pipe);
        break;

        case 'r':
        memset(regist.box_name,'\0',sizeof(regist.box_name));
        strcpy(regist.box_name,argv[4]);
        regist.code = 5;
        if(write(reg_pipe,&regist,sizeof(Register)) == -1 || errno == EPIPE){
            fprintf(stderr,"Error writing to pipe\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stdout,"Request to remove a box sent\n");
        response_create_remove(regist.named_pipe);
        break;

        case 'l':
        regist.code = 7;
        if(write(reg_pipe,&regist,sizeof(Register)) == -1 || errno == EPIPE){
            fprintf(stderr,"Error writing to pipe\n");
            exit(EXIT_FAILURE);
        }
        fprintf(stdout,"Request to list all boxes sent\n");
        response_list(regist.named_pipe);
        break;
        default:
        break;


    }    

    if(close(reg_pipe) == -1){
        fprintf(stderr,"Error closing pipe\n");
        exit(EXIT_FAILURE);
    }
    
    unlink(regist.named_pipe);
    fprintf(stdout,"Manager succesfully closed\n");

    return 0;
}
