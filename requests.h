#ifndef REQUEST_H
#define REQUEST_H
#include <stdint.h>
#define BOX_MAX 32
#define PIPE_MAX 256
#define MESSAGE_MAX 1024
#define MAX_BOX_NUMB 64
#define PIPE_CODE 0777

typedef struct 
{
    uint8_t code;
    char named_pipe[PIPE_MAX];
    char box_name[BOX_MAX];
}Register;

typedef struct 
{
    uint8_t code;
    int32_t return_code;
    char error_message[MESSAGE_MAX];
}Response;

typedef struct 
{
    uint8_t code;
    uint8_t last;
    char box_name[BOX_MAX];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;
}Manager_list;

typedef struct
{
    uint8_t code;
    char message[MESSAGE_MAX];
}Message;

#endif