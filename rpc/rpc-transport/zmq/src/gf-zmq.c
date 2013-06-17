#include <zmq.h>

#include "gf-zmq.h"
#include "rpc-transport.h"

int32_t
zmq_submit_request (rpc_transport_t *this, rpc_transport_req_t *req)
{
        return 0;
}

int32_t
zmq_submit_reply (rpc_transport_t *this, rpc_transport_reply_t *reply)
{
        return 0;
}

int32_t
zmq_connect (rpc_transport_t *this, int port)
{
        return 0;
}

int32_t
zmq_disconnect (rpc_transport_t *this)
{
        return 0;
}

int32_t
zmq_listen (rpc_transport_t *this)
{
        return 0;
}

struct rpc_transport_ops tops = {
        .submit_request = zmq_submit_request,
        .submit_reply   = zmq_submit_reply,
        .connect        = zmq_connect,
        .disconnect     = zmq_disconnect,
        .listen         = zmq_listen,
}

int32_t
init (rpc_transport_t *this)
{
        return 0;
}

void (rpc_transport_t *this)
{
        return;
}
