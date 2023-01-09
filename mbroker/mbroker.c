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
    char* pipe_name;
    int pipe_fd;
    pc_queue_t* queue;
    pthread_t thread_id;
} session_t;


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

int main(int argc, char** argv) {
if (argc != 3) {
    fprintf(stderr, "usage: mbroker <pipename> <max sessions>\n");
    return -1;
    }

char* pipe_name = argv[1];
MAX_THREADS = atoi(argv[2]);


session_t sessions[MAX_THREADS];
tfs_init(NULL);

mkfifo(pipe_name, 0666);
int pipe_fd = open(pipe_name, O_RDONLY | O_NONBLOCK);

if (pipe_fd < 0) {
    perror("Error opening pipe");
    return -1;
}

char msg[MAX_MSG_LEN];
ssize_t n;

int i = 0;
while ((n = read(pipe_fd, msg, MAX_MSG_LEN)) > 0) {
    char* pipe_n = strtok(msg, " ");
    int session_fd = open(pipe_n, O_WRONLY);

    pc_queue_t* queue = (pc_queue_t*)malloc(sizeof(pc_queue_t));
    pcq_create(queue, (size_t)MAX_THREADS);

    pthread_t thread_id;
    session_t* new_session = &sessions[i];
    new_session->pipe_name = strdup(pipe_n);
    new_session->pipe_fd = session_fd;
    new_session->queue = queue;

    pthread_create(&thread_id, NULL, session_thread_func, new_session);
    pthread_detach(thread_id);
    i++;
}

close(pipe_fd);

return 0;
}