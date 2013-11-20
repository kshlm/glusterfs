/*
   Copyright (c) 2010-2013 Red Hat, Inc. <http://www.redhat.com>
   This file is part of GlusterFS.

   This file is licensed to you under your choice of the GNU Lesser
   General Public License, version 3 or any later version (LGPLv3 or
   later), or the GNU General Public License, version 2 (GPLv2), in all
   cases as published by the Free Software Foundation.
*/

#include "cli-sync-cmd.h"
#include "timer.h"

extern struct rpc_clnt *global_rpc;
extern rpc_clnt_prog_t *cli_rpc_prog;

int
cli_sync_cmd_cbk (struct rpc_req *req, struct iovec *iov, int count, void
                  *frame)
{
        int             ret = -1;
        call_frame_t    *saved_frame = NULL;
        fop_cbk_fn_t    cbkfn = NULL;
        struct syncargs *args = NULL;

        saved_frame = frame;
        cbkfn = saved_frame->local;
        args = saved_frame->cookie;

        GF_ASSERT (cbkfn);
        GF_ASSERT (args);

        ret = cbkfn (req, iov, count, frame);

        __wake (args);

        return ret;
}

void
cli_sync_cmd_timedout (void *data)
{
        struct syncargs *args = data;

        args->op_ret = -1;
        args->op_errno = ETIMEDOUT;

        __wake (args);

        return;
}

int
_cli_sync_cmd_submit (void *req, struct syncargs *args, rpc_clnt_prog_t *prog,
                      int procnum, fop_cbk_fn_t cbkfn, xdrproc_t xdrproc)
{
        int                     ret = -1;
        int                     count = 0;
        struct iovec            iov = {0,};
        struct iobuf            *iobuf = NULL;
        struct iobref           *iobref = NULL;
        call_frame_t            *frame = NULL;
        ssize_t                 xdr_size = 0;

        GF_ASSERT (req);
        GF_ASSERT (args);

        xdr_size = xdr_sizeof (xdrproc, req);
        iobuf = iobuf_get2 (global_rpc->ctx->iobuf_pool, xdr_size);
        if (!iobuf)
                goto out;

        iobref = iobref_new ();
        if (!iobref)
                goto out;

        frame = create_frame (THIS, THIS->ctx->pool);
        if (!frame)
                goto out;

        iobref_add (iobref, iobuf);

        iov.iov_base = iobuf->ptr;
        iov.iov_len = iobuf_size (iobuf);

        ret = xdr_serialize_generic (iov, req, xdrproc);
        if (-1 == ret)
                goto out;

        iov.iov_len = ret;
        count = 1;

        frame->cookie = args;
        frame->local = cbkfn;

        ret = rpc_clnt_submit (global_rpc, prog, procnum, cli_sync_cmd_cbk,
                               &iov, count, NULL, 0, NULL, frame, NULL, 0, NULL,
                               0, NULL);
        ret = 0;

out:
        iobref_unref (iobref);
        iobuf_unref (iobuf);
        return ret;
}

int
cli_sync_cmd_submit (void *req, struct syncargs *args, rpc_clnt_prog_t *prog,
                     int procnum, fop_cbk_fn_t cbkfn, xdrproc_t xdrproc,
                     int timeout)
{
        int        ret = -1;
        gf_timer_t *timer = NULL;
        struct timespec time = {0,};

        __yawn (args);
        ret = _cli_sync_cmd_submit (req, args, prog, procnum, cbkfn, xdrproc);

        if (ret ) {
                args->op_ret = -1;
                goto out;
        }

        if (-1 != timeout) {
                time.tv_sec = timeout ? timeout : CLI_DEFAULT_CMD_TIMEOUT;
                timer = gf_timer_call_after (THIS->ctx, time,
                                             cli_sync_cmd_timedout, args);
        }

        __yield (args);
        if (timer)
                gf_timer_call_cancel (THIS->ctx, timer);
out:
        return args->op_ret;
}

int
cli_sync_volume_cmd_cbk (struct rpc_req *req, struct iovec *iov, int count,
                         void *frame)
{
        int             ret = -1;
        gf_cli_rsp      rsp = {0,};
        call_frame_t    *saved_frame = NULL;
        struct syncargs *args = NULL;
        dict_t          *rsp_dict = NULL;

        saved_frame = frame;
        args = saved_frame->cookie;

        if (-1 == req->rpc_status)
                goto out;

        ret = xdr_to_generic (*iov, &rsp, (xdrproc_t)xdr_gf_cli_rsp);
        if (ret < 0) {
                gf_log ("", GF_LOG_ERROR, "error");
                goto out;
        }

        if (rsp.dict.dict_len > 0) {
                rsp_dict = dict_new ();
                ret = dict_unserialize (rsp.dict.dict_val, rsp.dict.dict_len,
                                        &rsp_dict);
                if (ret)
                        goto out;
        }

        ret = 0;

        args->op_ret = rsp.op_ret;
        args->op_errno = rsp.op_errno;
        if (strlen(rsp.op_errstr))
                args->errstr = gf_strdup (rsp.op_errstr);
        if (rsp_dict)
                args->dict = dict_ref (rsp_dict);

out:
        if (rsp_dict)
                dict_unref (rsp_dict);
        free (rsp.op_errstr);

        if (ret)
                args->op_ret = ret;

        return args->op_ret;
}

int
cli_sync_volume_cmd (int procnum, dict_t *req_dict, dict_t **rsp_dict,
                     char **op_errstr, int timeout)
{
        int                     ret = -1;
        struct syncargs         args = {0,};
        gf_cli_req              req = {{0,},};

        args.op_ret = -1;
        args.op_errno = ENOTCONN;

        ret = dict_allocate_and_serialize (req_dict, &req.dict.dict_val,
                                           &req.dict.dict_len);
        if (ret)
                goto out;

        cli_sync_cmd_submit (&req, &args, cli_rpc_prog, procnum,
                             cli_sync_volume_cmd_cbk, (xdrproc_t)xdr_gf_cli_req,
                             timeout);

        if (op_errstr)
                *op_errstr = args.errstr;
        else
                GF_FREE (args.errstr);

        if (rsp_dict)
                *rsp_dict = args.dict;
        else
                dict_unref (args.dict);
out:
        errno = args.op_errno;
        return args.op_ret;
}
