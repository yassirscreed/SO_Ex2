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

#define MAX_MSG_LEN 100

int MAX_THREADS;

typedef struct {
    char pipe_name[256];
    char box_name[32];
    int pipe_fd;
    pc_queue_t* queue;
    pthread_t thread_id;
    pthread_cond_t cond;
} session_t;

typedef struct {
    char box_name[box_name];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;   
    int tfs_file;
    int last;
    pthread_cond_t cond;
}Box;


void* session_thread_func(void* arg) {
    session_t* session = (session_t*)arg;
    char msg[MAX_MSG_LEN];
    ssize_t n;

    while ((n = read(session->pipe_fd, msg, MAX_MSG_LEN)) > 0) {
        pcq_enqueue(session->queue, strdup(msg));
    }

    close(session->pipe_fd);
    pcq_destroy(session->queue);
    pthread_exit(NULL);
}

void register_publisher(Regist regist){
    if (unlink(regist.named_pipe) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", regist.named_pipe,strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (mkfifo(regist.named_pipe, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    for()
    //adicionar á box;
}




int main(int argc, char** argv) {
if (argc != 3) {
    fprintf(stderr, "usage: mbroker <pipename> <max sessions>\n");
    return -1;
    }

char pipe_name[256];
strcpy(pipe_name,argv[1])
MAX_THREADS = atoi(argv[2]);


session_t sessions[MAX_THREADS];
tfs_init(NULL);

mkfifo(pipe_name, 0666);
int reg_pipe = open(pipe_name, O_RDONLY | O_NONBLOCK);

if (pipe_fd < 0) {
    perror("Error opening pipe");
    return -1;
}
Box boxes[64];
Register regist;
int bytes_read;
mem
while (true) {
    int tmp_pipe = open(pipename, O_RDONLY);
    if (tmp_pipe < 0) {
        if (errno == ENOENT) {
            exit(EXIT_FAILURE);
        }
        perror("Failed to open server pipe");
        exit(EXIT_FAILURE);
    }
    if (close(tmp_pipe) < 0) {
        perror("Failed to close pipe");
        exit(EXIT_FAILURE);
    }

    bytes_read = read(reg_pipe,&regist,sizeof(Regist));
    while(bytes_read > 0)
        switch(regist.code){
            case 1:
            register_publisher(regist);
            break;
            case 2:
            break;
            case 3:
            break;
            case 5:
            break;
            case 7:
            break;
            case 9:
            break;


        }


    //session_t sessions[MAX_THREADS] //alocar dinamicamente se usar
    

    //pc_queue_t* queue = (pc_queue_t*)malloc(sizeof(pc_queue_t));
    //pcq_create(queue, (size_t)MAX_THREADS);
    //
    //pthread_t thread_id;
    //session_t* new_session = &sessions[i];
    //new_session->pipe_name = strdup(pipe_n);
    //new_session->pipe_fd = session_fd;
    //new_session->queue = queue;
    //
    //pthread_create(&thread_id, NULL, session_thread_func, new_session);
    //pthread_detach(thread_id);
    //i++;
}

close(pipe_fd);

return 0;
}