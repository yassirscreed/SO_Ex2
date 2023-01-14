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

#define MAX_MSG_LEN 100



typedef struct { 
    int fd_pipe;
    pc_queue_t* queue;
    pthread_mutex_t mutex;
} session_t;

typedef struct {
    char box_name[32];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;   
    int last;
    int tfs_file;
    pthread_cond_t cond;
    int active;
}Box;

int _max_threads;
Box _boxes[64];

void end(int sig){
    (void) sig;
    exit(EXIT_SUCCESS);
}

//void* session_thread_func(void* arg) {
//    session_t* session = (session_t*)arg;
//    char msg[MAX_MSG_LEN];
//    ssize_t n;
//
//    while ((n = read(session->fd_pipe, msg, MAX_MSG_LEN)) > 0) {
//        pcq_enqueue(session->queue, strdup(msg));
//    }
//
//    close(session->fd_pipe);
//    pcq_destroy(session->queue);
//    pthread_exit(NULL);
//}

void register_publisher(Register regist){
    int fd_pipe;
    int i,fhandle;
    if (unlink(regist.named_pipe) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", regist.named_pipe,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(regist.named_pipe, 0777) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    for(i=0; i< 64;i++){
        if(strcmp(regist.box_name,_boxes[i].box_name)==0)
            break;    
    }
    
    fd_pipe = open(regist.named_pipe,O_RDONLY);

    if(i != 64 && _boxes[i].n_publishers == 0){
        session_t session;
        Message m;
        ssize_t bytes_read;
        char box_name_path[65] = "/";

        pthread_mutex_init(&session.mutex,NULL);
        
        _boxes[i].n_publishers= 1;
        strcat(box_name_path,_boxes[i].box_name);

        pthread_mutex_lock(&session.mutex);
        fhandle = tfs_open(box_name_path,TFS_O_APPEND);//meter o / antes do box name stringcat

        while((bytes_read = read(fd_pipe,&m,sizeof(Message))) != 0){
            pthread_cond_broadcast(&_boxes[i].cond);
            tfs_write(fhandle,m.message,sizeof(m.message));    
        }
        tfs_close(fhandle);
        pthread_mutex_unlock(&session.mutex);
        pthread_mutex_destroy(&session.mutex);
        _boxes[i].n_publishers= 0;
        
    
    }

    close(fd_pipe);
    unlink(regist.named_pipe);
    
}

void register_subscriber(Register regist){
    int fd_pipe;
    int i,fhandle;
    if (unlink(regist.named_pipe) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", regist.named_pipe,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(regist.named_pipe, 0777) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    for(i=0; i< 64;i++){
        if(strcmp(regist.box_name,_boxes[i].box_name)==0)
            break;    
    }

    fd_pipe = open(regist.named_pipe,O_WRONLY);

    if(i != 64 && _boxes[i].n_publishers == 0){
        session_t session;
        Message m;
        ssize_t bytes_read;
        char box_name_path[65] = "/";

        pthread_mutex_init(&session.mutex,NULL);
        _boxes[i].n_subscribers++;
        strcat(box_name_path,_boxes[i].box_name);

        pthread_mutex_lock(&session.mutex);
        fhandle = tfs_open(box_name_path,TFS_O_APPEND);//meter o / antes do box name stringcat

        while((bytes_read = write (fd_pipe,&m,sizeof(Message))) != -1 ){ //mudar condição -1
            pthread_cond_wait(&_boxes[i].cond,&session.mutex); // mudar para l  
        }
        tfs_close(fhandle);
        pthread_mutex_unlock(&session.mutex);
        pthread_mutex_destroy(&session.mutex);
        _boxes[i].n_subscribers--;
    }

    close(fd_pipe);
    unlink(regist.named_pipe);
    
}

void register_box(Register regist){
    int fd_pipe;
    int i,space = -1;
    if (unlink(regist.named_pipe) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", regist.named_pipe,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(regist.named_pipe, 0777) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    for(i=0; i< 64;i++){
        if(strcmp(regist.box_name,_boxes[i].box_name)==0)
            break;
        if(space == -1 && _boxes[i].active == 0)
            space = i;
    }

    Response r;
    r.code = 4;
    if( space == -1){
        r.return_code = -1;
        strcpy(r.error_message,"Tfs full");
    }
    else if(i >= 64){
        r.return_code = -1;
        strcpy(r.error_message,"Box name already in use");
    }
    else
    {
        char box_name_path[65] = "/";
        r.return_code = 0;
        strcpy(_boxes[i].box_name,regist.box_name);
        _boxes[i].n_publishers = 0;
        _boxes[i].n_subscribers = 0;
        _boxes[i].last = 0;
        _boxes[i].active = 1;
        strcat(box_name_path,_boxes[i].box_name);
        _boxes[i].tfs_file = tfs_open(box_name_path,TFS_O_CREAT);
        pthread_cond_init(&_boxes[i].cond,NULL);
    }
    fd_pipe = open(regist.named_pipe,O_WRONLY);
    if(write(fd_pipe,&r,sizeof(Response)) == -1){
        return; //change
    }
    close(fd_pipe);
    unlink(regist.named_pipe);

}

void remove_box(Register regist){
    int i,fd_pipe;
     if (unlink(regist.named_pipe) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", regist.named_pipe,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(regist.named_pipe, 0777) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    for(i=0;i<64;i++){
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
    }
    fd_pipe = open(regist.named_pipe,O_WRONLY);
    if(write(fd_pipe,&r,sizeof(Response)) == -1){
        return; //change
    }
    close(fd_pipe);
    unlink(regist.named_pipe);    

}



int main(int argc, char** argv) {
if (argc != 3) {
    fprintf(stderr, "usage: mbroker <pipename> <max sessions>\n");
    return -1;
    }

signal(SIGINT,end);
char pipe_name[256];
strcpy(pipe_name,argv[1]);
_max_threads = atoi(argv[2]);


//session_t sessions[MAX_THREADS];
tfs_init(NULL);

mkfifo(pipe_name, 0666);
int reg_pipe = open(pipe_name, O_RDONLY | O_NONBLOCK);

if (reg_pipe < 0) {
    perror("Error opening pipe");
    return -1;
}
Register regist;
ssize_t bytes_read;
while (1) {
    int dummy_pipe = open(pipe_name, O_RDONLY);
    if (dummy_pipe < 0) {
        if (errno == ENOENT) {
            exit(EXIT_FAILURE);
        }
        perror("Failed to open server pipe");
        exit(EXIT_FAILURE);
    }
    if (close(dummy_pipe) < 0) {
        perror("Failed to close pipe");
        exit(EXIT_FAILURE);
    }

    bytes_read = read(reg_pipe,&regist,sizeof(Register));
    while(bytes_read > 0)
        switch(regist.code){
            case 1:
            register_publisher(regist);
            break;
            case 2:
            register_subscriber(regist);
            break;
            case 3:
            register_box(regist);
            break;
            case 5:
            remove_box(regist);
            break;
            case 7:
            break;
            default:
            break;

        }


    //session_t sessions[MAX_THREADS] //alocar dinamicamente se usar
    

    //pc_queue_t* queue = (pc_queue_t*)malloc(sizeof(pc_queue_t));
    //pcq_create(queue, (size_t)MAX_THREADS);
    //
    //pthread_t thread_id;
    //session_t* new_session = &sessions[i];
    //new_session->pipe_name = strdup(pipe_n);
    //new_session->fd_pipe = session_fd;
    //new_session->queue = queue;
    //
    //pthread_create(&thread_id, NULL, session_thread_func, new_session);
    //pthread_detach(thread_id);
    //i++;
}

close(reg_pipe);

return 0;
}