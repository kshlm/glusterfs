#ifndef _ZMQ_H
#define _ZMQ_H

#include <zmq.h>

#include "globals.h"

typedef struct {
        void    *zmq_ctx;
        void    *zmq_sock;
        int     sock_fd;
        int     idx;
} zmq_private_t;

#endif
