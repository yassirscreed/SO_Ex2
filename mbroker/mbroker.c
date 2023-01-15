#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include "logging.h"
#include "../producer-consumer/producer-consumer.h"
#include "../fs/operations.h"
#include "requests.h"
#include <errno.h>
#include <signal.h>


typedef struct {
    char box_name[BOX_MAX];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;   
    int last;
    int active;
    int tfs_file;
    pthread_cond_t cond;  
    pthread_mutex_t mutex;
}Box;


pc_queue_t* _queue;
Box _boxes[MAX_BOX_NUMB];


void register_publisher(Register regist){
    int fd_pipe;
    int i,fhandle;

    for(i=0; i< MAX_BOX_NUMB;i++){
        if(strcmp(regist.box_name,_boxes[i].box_name)==0 && _boxes[i].active == 1)
            break;    
    }

    fd_pipe = open(regist.named_pipe,O_RDONLY);
        if (fd_pipe == -1) {
            fprintf(stdout,"Error opening pipe\n");
            exit(EXIT_FAILURE); 
        }

    if(i != 64 && _boxes[i].n_publishers == 0){
        Message m;
        char box_name_path[MAX_BOX_NUMB+1] = "/";
        ssize_t size,size1;

        _boxes[i].n_publishers= 1;
        strcat(box_name_path,_boxes[i].box_name);

        fhandle = tfs_open(box_name_path,TFS_O_APPEND);
        

        while((size1 = read(fd_pipe,&m,sizeof(Message))) > 0){
            pthread_mutex_lock(&_boxes[i].mutex);
            int dummy_pipe = open(regist.named_pipe, O_RDONLY);
            if (dummy_pipe < 0) {
                if (errno == ENOENT) {
                exit(EXIT_FAILURE);
            }
                fprintf(stderr,"Failed to open server pipe\n");
                exit(EXIT_FAILURE);
            }
            if (close(dummy_pipe) < 0) {
                fprintf(stderr,"Failed to close pipe\n");
                exit(EXIT_FAILURE);
            }
            size = tfs_write(fhandle,m.message,strlen(m.message)+1);
            _boxes[i].box_size+= (uint64_t) size;            
            pthread_cond_broadcast(&_boxes[i].cond);
            pthread_mutex_unlock(&_boxes[i].mutex);
            
            
        }
        fprintf(stdout,"%ld\n",size1);
        tfs_close(fhandle);
        _boxes[i].n_publishers= 0;
    
    }

    if(close(fd_pipe) == -1){
            fprintf(stderr,"Error closing pipe\n");
            exit(EXIT_FAILURE);
        }
    unlink(regist.named_pipe);     
    fprintf(stdout,"Publisher finished successfully\n");
    
}

void register_subscriber(Register regist){
    int fd_pipe;
    int i,fhandle;

    signal(SIGPIPE,SIG_IGN);

    for(i=0; i< 64;i++){
        if(strcmp(regist.box_name,_boxes[i].box_name)==0 && _boxes[i].active == 1)
            break;    
    }

    fd_pipe = open(regist.named_pipe,O_WRONLY);
    if (fd_pipe == -1) {
        fprintf(stdout,"Error opening pipe\n");
        exit(EXIT_FAILURE); 
    }
    
    if(i != 64){
        Message m;
        char box_name_path[BOX_MAX+1] = "/";
        char buffer[MESSAGE_MAX];
        ssize_t size;
        _boxes[i].n_subscribers++;
        strcat(box_name_path,_boxes[i].box_name);

        
        fhandle = tfs_open(box_name_path,TFS_O_CREAT);
        memset(buffer,'\0',sizeof(buffer));
        size = tfs_read(fhandle,buffer,sizeof(buffer));
        int i1 = (int)strlen(buffer);
        int j = 0;
        while(i1!=0) {
            fprintf(stdout,"1\n"); 
            memset(m.message,'\0',MESSAGE_MAX*sizeof(char));
            strcpy(m.message,(buffer+j));
            if(write(fd_pipe,&m,sizeof(Message)) == -1 || errno == EPIPE){
                fprintf(stderr,"Error writing to pipe\n");
                exit(EXIT_FAILURE);
            }           
            fprintf(stdout,"%s\n",m.message);
            j+= i1 + 1;
            i1 = (int)strlen((buffer+j));
            
        }
        
        
        size =1;
        while(size > 0){ 
            pthread_mutex_lock(&_boxes[i].mutex);
            pthread_cond_wait(&_boxes[i].cond,&_boxes[i].mutex);
            size = tfs_read(fhandle,buffer,sizeof(buffer));
            strcpy(m.message,(buffer));
            if(write(fd_pipe,&m,sizeof(Message)) == -1 || errno == EPIPE){
                break;
            }
            memset(buffer,'\0',sizeof(buffer));
            pthread_mutex_unlock(&_boxes[i].mutex);

        }

        tfs_close(fhandle);
        _boxes[i].n_subscribers--;
    }
    
    if(close(fd_pipe) == -1){
        fprintf(stderr,"Error closing pipe\n");
        exit(EXIT_FAILURE);
    }
    unlink(regist.named_pipe);
    fprintf(stdout,"Subscriber finished successfully\n");
    
}

void register_box(Register regist){
    int fd_pipe;
    int i,space = -1;
    
    for(i=0; i< MAX_BOX_NUMB;i++){
        if(strcmp(regist.box_name,_boxes[i].box_name)==0 && _boxes[i].active == 1)
            break;
        if(space == -1 && _boxes[i].active == 0)
            space = i;
    }

    Response r;
    r.code = 4;
    
    if(i < 64){
        r.return_code = -1;
        strcpy(r.error_message,"Box name already in use");
        fprintf(stdout,"Manager failed to create \n");
    }
    else if( space == -1){
        r.return_code = -1;
        strcpy(r.error_message,"Tfs full");
        fprintf(stdout,"Manager failed to create\n");
    }
    else{
        char box_name_path[BOX_MAX+1] = "/";
        r.return_code = 0;
        strcpy(_boxes[space].box_name,regist.box_name);
        _boxes[space].n_publishers = 0;
        _boxes[space].n_subscribers = 0;
        _boxes[space].last = 0;
        _boxes[space].active = 1;
        pthread_mutex_init(&_boxes[space].mutex,NULL);
        strcat(box_name_path,_boxes[space].box_name);
        _boxes[space].tfs_file = tfs_open(box_name_path,TFS_O_CREAT);
        pthread_cond_init(&_boxes[space].cond,NULL);
        fprintf(stdout,"Box created successfully \n");
    }

    fd_pipe = open(regist.named_pipe,O_WRONLY);
    if (fd_pipe == -1) {
        fprintf(stdout,"Error opening pipe\n");
        exit(EXIT_FAILURE); 
    }

    if(write(fd_pipe,&r,sizeof(Response)) == -1 || errno == EPIPE){
        fprintf(stderr,"Error writing to pipe\n");
        exit(EXIT_FAILURE);
    }

    if(close(fd_pipe) == -1){
        fprintf(stderr,"Error closing pipe\n");
        exit(EXIT_FAILURE);
    }

    unlink(regist.named_pipe);


}

void remove_box(Register regist){
    int i,fd_pipe;
    for(i=0;i<MAX_BOX_NUMB;i++){
        if(strcmp(regist.box_name,_boxes[i].box_name) == 0 && _boxes[i].active == 1)
            break;
    }
    Response r;
    r.code = 6;
    if(i == 64){
        r.return_code =-1;
        strcpy(r.error_message,"The box doesn't exist");
    }
    else{
        r.return_code=0;
        _boxes[i].active = 0;
        tfs_close(_boxes[i].tfs_file);
        pthread_cond_destroy(&_boxes[i].cond);
        pthread_mutex_destroy(&_boxes[i].mutex);
    }

    fd_pipe = open(regist.named_pipe,O_WRONLY);
    if (fd_pipe == -1) {
        fprintf(stdout,"Error opening pipe\n");
        exit(EXIT_FAILURE); 
    }

    if(write(fd_pipe,&r,sizeof(Response)) == -1 || errno == EPIPE){
        fprintf(stderr,"Error writing to pipe\n");
        exit(EXIT_FAILURE);
    }
    if(close(fd_pipe) == -1){
        fprintf(stderr,"Error closing pipe\n");
        exit(EXIT_FAILURE);
    }
    unlink(regist.named_pipe);    
    fprintf(stdout,"Box removed successfully \n");
}

void list_box(Register regist){
    int i,fd_pipe;
    
    Manager_list ml;
    ml.code = 10;
    fd_pipe = open(regist.named_pipe,O_WRONLY);

    if (fd_pipe == -1) {
        fprintf(stdout,"Error opening pipe\n");
        exit(EXIT_FAILURE); 
    }

    for(i=0;i<64;i++){ 
        if(_boxes[i].active == 1){
            strcpy(ml.box_name,_boxes[i].box_name);
            ml.box_size = _boxes[i].box_size;
            ml.n_publishers=_boxes[i].n_publishers;
            ml.n_subscribers=_boxes[i].n_subscribers;
            if(write(fd_pipe,&ml,sizeof(Manager_list)) == -1 || errno == EPIPE){
                fprintf(stderr,"Error writing to pipe\n");
                exit(EXIT_FAILURE);
            }
        }        
    }

    
    ml.last = 1;
    if(write(fd_pipe,&ml,sizeof(Manager_list)) == -1 || errno == EPIPE){
        fprintf(stderr,"Error writing to pipe\n");
        exit(EXIT_FAILURE);
    }
    

    if(close(fd_pipe) == -1){
        fprintf(stderr,"Error closing pipe\n");
        exit(EXIT_FAILURE);
    }
    unlink(regist.named_pipe);
    fprintf(stdout,"List Boxes finished successfully\n");      
}

void *worker_thread_function(void *arg) {
    while (1) {
        Register* regist = pcq_dequeue(_queue);
        (void) arg;
        switch (regist->code) {
            case 1:
            fprintf(stdout,"Request to regist a publisher received\n");
            register_publisher(*regist);
            break;
            case 2:
            fprintf(stdout,"Request to regist a subscriber received\n");
            register_subscriber(*regist);
            break;
            case 3:
            fprintf(stdout,"Request to create a box received\n");
            register_box(*regist);
            break;
            case 5:
            fprintf(stdout,"Request to remove a box received\n");
            remove_box(*regist);
            break;
            case 7:
            fprintf(stdout,"Request to list boxes received\n");
            list_box(*regist);
            break;
            default:
            break;
        }
    }
}


int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: mbroker <pipename> <max sessions>\n");
        exit(EXIT_FAILURE);
    }

    if(tfs_init(NULL) == -1){
        fprintf(stderr,"Tfs failed to initialize\n");
        exit(EXIT_FAILURE);
    }

    char pipe_name[PIPE_MAX];
    pthread_t* workers;
    size_t max_threads;
    
    strcpy(pipe_name,argv[1]);
    max_threads = (size_t) atoi(argv[2]);
    
    _queue = malloc(sizeof(pc_queue_t));
    pcq_create(_queue, max_threads);
    
    workers = malloc(sizeof(pthread_t) * max_threads);
    for(int i = 0; i < max_threads; i++) {
        pthread_create(&workers[i], NULL, worker_thread_function, NULL);
    }
    
    if (unlink(pipe_name) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipe_name,strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (mkfifo(pipe_name, PIPE_CODE) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    int reg_pipe = open(pipe_name, O_RDONLY);
    if (reg_pipe < 0) {
        fprintf(stderr,"Error opening pipe\n");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout,"Mbroker created\n");
    
    Register regist;
    ssize_t bytes_read;
    while (1) {
        int dummy_pipe = open(pipe_name, O_RDONLY);
        if (dummy_pipe < 0) {
            if (errno == ENOENT) {
                exit(EXIT_FAILURE);
            }
            fprintf(stderr,"Failed to open server pipe\n");
            exit(EXIT_FAILURE);
        }
        if (close(dummy_pipe) < 0) {
            fprintf(stderr,"Failed to close pipe\n");
            exit(EXIT_FAILURE);
        }
    
        if((bytes_read = read(reg_pipe,&regist,sizeof(Register))) < 0){
            fprintf(stderr,"Error reading from pipe\n");
            exit(EXIT_FAILURE); 
        }

        if(bytes_read>0)
            pcq_enqueue(_queue,&regist);
        
    
    }
    for(int i = 0; i < max_threads; i++) {
        pthread_join(workers[i], NULL);
    }
    
    close(reg_pipe);
    unlink(pipe_name);
    return 0;
}