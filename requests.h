#ifndef REQUEST_H
#define REQUEST_H
#include <stdint.h>

typedef struct 
{
    uint8_t code;
    char named_pipe[256];
    char box_name[32];
}Register;

typedef struct 
{
    uint8_t code;
    int32_t return_code;
    char error_message[1024];
}Response;

typedef struct 
{
    uint8_t code;
    uint8_t last;
    char box_name;
    uint64_t box_size;
    uint64_t n_pubs;
    uint64_t n_subs;
}Manager_list;

typedef struct
{
    uint8_t code;
    char message[1024];
}Message;

#endif