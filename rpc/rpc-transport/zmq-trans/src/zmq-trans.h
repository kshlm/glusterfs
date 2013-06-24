#ifndef _ZMQ_H
#define _ZMQ_H

#include <zmq.h>

#include "globals.h"

typedef struct {
        void    *zmq_ctx;
        void    *zmq_sock;
        char    *zmq_endpoint;
        int     sock_fd;
        int     idx;
} zmq_private_t;

#endif
