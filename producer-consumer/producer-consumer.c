#include "producer-consumer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>


// pcq_create: create a queue, with a given (fixed) capacity
//
// Memory: the queue pointer must be previously allocated
// (either on the stack or the heap)
int pcq_create(pc_queue_t *queue, size_t capacity){
    queue->pcq_buffer = malloc(capacity * sizeof(void));
    if (queue->pcq_buffer == NULL) {
    return -1;
    }
    queue->pcq_capacity = capacity;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;

    pthread_mutex_init(&queue->pcq_current_size_lock, NULL);
    pthread_mutex_init(&queue->pcq_head_lock, NULL);
    pthread_mutex_init(&queue->pcq_tail_lock, NULL);
    pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL);
    pthread_cond_init(&queue->pcq_pusher_condvar, NULL);
    pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL);
    pthread_cond_init(&queue->pcq_popper_condvar, NULL);

return 0;
}

// pcq_destroy: releases the internal resources of the queue
//
// Memory: does not free the queue pointer itself
int pcq_destroy(pc_queue_t *queue){
    free(queue->pcq_buffer);
    pthread_mutex_destroy(&queue->pcq_current_size_lock);
    pthread_mutex_destroy(&queue->pcq_head_lock);
    pthread_mutex_destroy(&queue->pcq_tail_lock);
    pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
    pthread_cond_destroy(&queue->pcq_pusher_condvar);
    pthread_mutex_destroy(&queue->pcq_popper_condvar_lock);
    pthread_cond_destroy(&queue->pcq_popper_condvar);
    return 0;
}

// pcq_enqueue: insert a new element at the front of the queue
//
// If the queue is full, sleep until the queue has space
int pcq_enqueue(pc_queue_t *queue, void *elem){
    pthread_mutex_lock(&queue->pcq_current_size_lock);
    // Check if queue is full
    while (queue->pcq_current_size == queue->pcq_capacity) {
        // Unlock current size mutex and wait on pusher condition variable
        pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_current_size_lock);
    }

    // Lock tail mutex
    pthread_mutex_lock(&queue->pcq_tail_lock);

    // Insert element at tail of queue
    queue->pcq_buffer[queue->pcq_tail] = elem;
    queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;

    // Increment current size
    queue->pcq_current_size++;

    // Unlock tail mutex
    pthread_mutex_unlock(&queue->pcq_tail_lock);

    // Signal popper condition variable
    pthread_cond_signal(&queue->pcq_popper_condvar);

    // Unlock current size mutex
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

return 0;

}

// pcq_dequeue: remove an element from the back of the queue
//
// If the queue is empty, sleep until the queue has an element
void *pcq_dequeue(pc_queue_t *queue){
    pthread_mutex_lock(&queue->pcq_current_size_lock);
    // Check if queue is empty
    while (queue->pcq_current_size == 0) {
        // Unlock current size mutex and wait on popper condition variable
        pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_current_size_lock);
    }

    // Lock head mutex
    pthread_mutex_lock(&queue->pcq_head_lock);

    // Remove element from back of the queue
    void *elem = queue->pcq_buffer[queue->pcq_head];
    queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;
    
    // Decrement current size
    queue->pcq_current_size--;

    // Unlock head mutex
    pthread_mutex_unlock(&queue->pcq_head_lock);

    // Signal pusher condition variable
    pthread_cond_signal(&queue->pcq_pusher_condvar);

    // Unlock current size mutex
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

return elem;
}
