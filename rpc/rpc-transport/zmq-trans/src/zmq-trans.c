#include "zmq-trans.h"
#include "rpc-transport.h"
#include "mem-types.h"
#include "common-utils.h"
#include "glusterfs.h"
#include "event.h"

#define GF_DEFAULT_ZMQ_LISTEN_PORT  GF_DEFAULT_BASE_PORT

int32_t
zmq_trans_submit (rpc_transport_t *this, rpc_transport_msg_t *msg)
{
        int             ret = -1;
        zmq_private_t   *priv = NULL;
        zmq_msg_t       rpchdr;
        zmq_msg_t       proghdr;
        zmq_msg_t       progpayload;

        GF_VALIDATE_OR_GOTO ("zmq", this, out);
        GF_VALIDATE_OR_GOTO ("zmq", this->private, out);
        GF_VALIDATE_OR_GOTO ("zmq", msg, out);

        priv = this->private;
        GF_VALIDATE_OR_GOTO ("zmq", priv->zmq_sock, out);


        /* An RPC request consists of three parts - rpc header, program header &
         * program payload. ZMQs multipart support makes it easy to send each of
         * these as a seperate message and combine them on reception. If a part
         * is not present in the request, we send empty messages.
         */

        if (msg->rpchdr != NULL) {
                ret = zmq_msg_init_size (&rpchdr, sizeof (struct iovec) *
                                         msg->rpchdrcount);
                if (ret) {
                        gf_log (this->name, GF_LOG_ERROR, "Failed to init "
                                "rpchdr zmq message (%s)", strerror (errno));
                        goto out;
                }
                memcpy (zmq_msg_data (&rpchdr), msg->rpchdr,
                        sizeof (struct iovec) * msg->rpchdrcount);
        } else {
                ret = zmq_msg_init (&rpchdr);
                if (ret) {
                        gf_log (this->name, GF_LOG_ERROR, "Failed to init empty"
                                "rpchdr zmq message (%s)", strerror (errno));
                        goto out;
                }
        }

        if (msg->proghdr != NULL) {
                ret = zmq_msg_init_size (&proghdr, sizeof (struct iovec) *
                                         msg->proghdrcount);
                if (ret) {
                        gf_log (this->name, GF_LOG_ERROR, "Failed to init "
                                "proghdr zmq message (%s)", strerror (errno));
                        goto out;
                }
                memcpy (zmq_msg_data (&proghdr), msg->proghdr,
                        sizeof (struct iovec) * msg->proghdrcount);
        } else {
                ret = zmq_msg_init (&proghdr);
                if (ret) {
                        gf_log (this->name, GF_LOG_ERROR, "Failed to init empty"
                                "proghdr zmq message (%s)", strerror (errno));
                        goto out;
                }
        }

        if (msg->progpayload != NULL) {
                ret = zmq_msg_init_size (&progpayload, sizeof (struct iovec) *
                                         msg->progpayloadcount);
                if (ret) {
                        gf_log (this->name, GF_LOG_ERROR, "Failed to init "
                                "progpayload zmq message (%s)",
                                strerror (errno));
                        goto out;
                }
                memcpy (zmq_msg_data (&progpayload), msg->progpayload,
                        sizeof (struct iovec) * msg->progpayloadcount);
        } else {
                ret = zmq_msg_init (&progpayload);
                if (ret) {
                        gf_log (this->name, GF_LOG_ERROR, "Failed to init empty"
                                "progpayload zmq message (%s)",
                                strerror (errno));
                        goto out;
                }
        }

        ret = zmq_msg_send (&rpchdr, priv->zmq_sock, ZMQ_SNDMORE);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR, "Failed to send rpchdr zmq "
                        "message (%s)", strerror (errno));
                goto out;
        }

        ret = zmq_msg_send (&proghdr, priv->zmq_sock, ZMQ_SNDMORE);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR, "Failed to send proghdr zmq "
                        "message (%s)", strerror (errno));
                goto out;
        }

        ret = zmq_msg_send (&progpayload, priv->zmq_sock, ZMQ_SNDMORE);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR, "Failed to send progpayload "
                        "zmq message (%s)", strerror (errno));
        }

out:
        zmq_msg_close (&rpchdr);
        zmq_msg_close (&proghdr);
        zmq_msg_close (&progpayload);

        return ret;
}

int32_t
zmq_trans_submit_request (rpc_transport_t *this, rpc_transport_req_t *req)
{
        int             ret = -1;

        GF_VALIDATE_OR_GOTO ("zmq", req, out);

        ret = zmq_trans_submit (this, &req->msg);
out:
        return ret;
}

int32_t
zmq_trans_submit_reply (rpc_transport_t *this, rpc_transport_reply_t *reply)
{
        int             ret = -1;

        GF_VALIDATE_OR_GOTO ("zmq", reply, out);

        ret = zmq_trans_submit (this, &reply->msg);
out:
        return ret;
}

static int
zmq_trans_event_handler (int fd, int idx, void *data, int poll_int,
                         int poll_out, int poll_err)
{
        return 0;
}

int32_t
zmq_trans_connect (rpc_transport_t *this, int port)
{
        int             ret = -1;
        zmq_private_t   *priv = NULL;
        uint16_t        remote_port = -1;
        char            *remote_host = NULL;
        char            *zmq_endpoint = NULL;
        void            *zmq_sock = NULL;
        int             sock_fd = -1;
        size_t          size = sizeof (sock_fd);

        GF_VALIDATE_OR_GOTO ("zmq", this, out);
        GF_VALIDATE_OR_GOTO ("zmq", this->private, out);
        GF_VALIDATE_OR_GOTO ("zmq", this->options, out);
        GF_VALIDATE_OR_GOTO ("zmq", this->ctx, out);

        priv = this->private;
        GF_VALIDATE_OR_GOTO ("zmq", priv->zmq_ctx, out);

        if (port > 0) {
                remote_port = port;
        } else {
                ret = dict_get_uint16 (this->options, "remote-port",
                                       &remote_port);
                if (ret)
                        remote_port = GF_DEFAULT_ZMQ_LISTEN_PORT;
                if ((uint16_t)-1 == remote_port) {
                        ret = -1;
                        gf_log (this->name, GF_LOG_ERROR,
                                "remote-port has invalid value in volume %s",
                                this->name);
                        goto out;
                }
        }

        ret = dict_get_str (this->options, "remote-host", &remote_host);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR,
                        "remote-host is not present in volume %s", this->name);
                goto out;
        }

        ret = gf_asprintf (&zmq_endpoint, "tcp://%s:%"PRIu16, remote_host,
                           remote_port);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to build endpoing string");
                goto out;
        }

        zmq_sock = zmq_socket (priv->zmq_ctx, ZMQ_REQ);
        if (!zmq_sock) {
                ret = -1;
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to create zmq socket");
                goto out;
        }

        ret = zmq_connect (zmq_sock, (const char *)zmq_endpoint);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR,
                        "zmq connect attempt on %s failed (%s)", zmq_endpoint,
                        strerror (errno));
                goto out;
        }

        ret = zmq_getsockopt (zmq_sock, ZMQ_FD, &sock_fd, &size);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to get zmq socket fd");
                goto out;
        }

        priv->sock_fd = sock_fd;
        priv->zmq_sock = zmq_sock;

        priv->idx = event_register (this->ctx->event_pool, priv->sock_fd,
                                    zmq_trans_event_handler, this, 1, 1);
        if (priv->idx == -1) {
                ret = -1;
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to register zmq event handler");
                goto out;
        }

out:
        if (ret) {
                priv->sock_fd = -1;
                if (priv->zmq_sock) {
                        zmq_close (priv->zmq_sock);
                        priv->zmq_sock = NULL;
                }
        }
        GF_FREE (zmq_endpoint);
        return ret;
}

int32_t
zmq_trans_disconnect (rpc_transport_t *this)
{
        return 0;
}

static int
zmq_trans_server_event_handler (int fd, int idx, void *data, int poll_int,
                                int poll_out, int poll_err)
{
        int                     ret = -1;
        rpc_transport_t         *this = NULL;
        zmq_private_t           *priv = NULL;
        int                     poll = 0;
        size_t                  size = sizeof (poll);

        this = data;
        GF_VALIDATE_OR_GOTO ("zmq", this, out);
        GF_VALIDATE_OR_GOTO ("zmq", this->private, out);

        priv = this->private;

        /* A poll event on a ZMQ_FD doesn't mean there is something to
         * read from or write to the fd. We need to get the actual state by
         * getting the ZMQ_EVENTS option of the zmq socket
         */
        ret = zmq_getsockopt (priv->zmq_sock, ZMQ_EVENTS, &poll, &size);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to get zmq poll event");
                goto out;
        }

        //TODO: Figure out what socket_server_event_handler is doing on pollin
        //      and implement this section
out:
        return ret;
}

int32_t
zmq_trans_listen (rpc_transport_t *this)
{
        int             ret = -1;
        zmq_private_t   *priv = NULL;
        uint16_t        listen_port = -1;
        char            *listen_host = NULL;
        char            *zmq_endpoint = NULL;
        void            *zmq_sock = NULL;
        int             sock_fd = -1;
        size_t          size = sizeof (sock_fd);

        GF_VALIDATE_OR_GOTO ("zmq", this, out);
        GF_VALIDATE_OR_GOTO ("zmq", this->private, out);
        GF_VALIDATE_OR_GOTO ("zmq", this->options, out);
        GF_VALIDATE_OR_GOTO ("zmq", this->ctx, out);

        priv = this->private;
        GF_VALIDATE_OR_GOTO ("zmq", priv->zmq_ctx, out);

        ret = dict_get_uint16 (this->options, "transport.socket.listen-port",
                               &listen_port);
        if (ret || (uint16_t)-1 == listen_port)
                listen_port = GF_DEFAULT_ZMQ_LISTEN_PORT;

        ret = dict_get_str (this->options, "transport.socket.bind-address",
                            &listen_host);

        ret = gf_asprintf (&zmq_endpoint, "tcp://%s:%"PRIu16,
                           (listen_host ? listen_host : "*"), listen_port);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to build endpoing string");
                goto out;
        }

        zmq_sock = zmq_socket (priv->zmq_ctx, ZMQ_REP);
        if (!zmq_sock) {
                ret = -1;
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to create zmq socket");
                goto out;
        }

        ret = zmq_bind (zmq_sock, (const char *)zmq_endpoint);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to zmq_bind. Error = %s", strerror (errno));
                goto out;
        }

        ret = zmq_getsockopt (zmq_sock, ZMQ_FD, &sock_fd, &size);
        if (ret) {
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to get zmq socket fd");
                goto out;
        }

        priv->sock_fd = sock_fd;
        priv->zmq_sock = zmq_sock;

        priv->idx = event_register (this->ctx->event_pool, priv->sock_fd,
                                    zmq_trans_server_event_handler, this, 1, 0);
        if (priv->idx == -1) {
                ret = -1;
                gf_log (this->name, GF_LOG_ERROR,
                        "Failed to register zmq event handler");
                goto out;
        }

out:
        if (ret) {
                priv->sock_fd = -1;
                if (priv->zmq_sock) {
                        zmq_close (priv->zmq_sock);
                        priv->zmq_sock = NULL;
                }
        }
        GF_FREE (zmq_endpoint);
        return ret;
}

struct rpc_transport_ops tops = {
        .submit_request = zmq_trans_submit_request,
        .submit_reply   = zmq_trans_submit_reply,
        .connect        = zmq_trans_connect,
        .disconnect     = zmq_trans_disconnect,
        .listen         = zmq_trans_listen,
};

int32_t
init (rpc_transport_t *this)
{
        int             ret = -1;
        zmq_private_t   *priv = NULL;

        if (this->private) {
                gf_log_callingfn (this->name, GF_LOG_ERROR,
                                  "Double init attempted");
                return ret;
        }

        priv = GF_CALLOC (1, sizeof (*priv), gf_common_mt_zmq_private_t);
        if (!priv)
                return ret;

        priv->zmq_ctx = zmq_ctx_new ();
        if (!priv) {
                gf_log (this->name, GF_LOG_ERROR,
                        "Could not create new zmq ctx");
                return ret;
        }

        this->private = priv;

        ret = 0;

        return ret;
}

void
fini (rpc_transport_t *this)
{
        zmq_private_t *priv = NULL;

        if (!this)
                return;

        priv = this->private;
        if (priv && priv->zmq_ctx)
                (void)zmq_ctx_destroy (priv->zmq_ctx);
        return;
}
