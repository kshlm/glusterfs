#include "cli-syncrpc-ops.h"

extern struct rpc_clnt *global_rpc;
extern rpc_clnt_prog_t *cli_rpc_prog;

int
_cli_sync_cmd_submit (void *req, struct syncargs *args, call_frame_t *frame,
                     rpc_clnt_prog_t *prog, int procnum, xlator_t *this,
                     fop_cbk_fn_t cbkfn, xdrproc_t xdrproc)
{
        int                     ret = -1;
        int                     count = 0;
        struct iovec            iov = {0,};
        struct iobuf            *iobuf = NULL;
        ssize_t                 xdr_size = 0;

        GF_ASSERT (this);
        GF_ASSERT (req);
        GF_ASSERT (args);

        xdr_size = xdr_sizeof (xdrproc, req);
        iobuf = iobuf_get2 (this->ctx->iobuf_pool, xdr_size);
        if (!iobuf)
                goto out;


        // Is iobref needed ?

        iov.iov_base = iobuf->ptr;
        iov.iov_len = iobuf_size (iobuf);

        ret = xdr_serialize_generic (iov, req, xdrproc);
        if (-1 == ret)
                goto out;

        iov.iov_len = ret;
        count = 1;

        frame->cookie = args;

        ret = rpc_clnt_submit (global_rpc, prog, procnum, cbkfn, &iov, count,
                               NULL, 0, NULL, frame, NULL, 0, NULL, 0, NULL);
        ret = 0;

out:
        return ret;
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

        if (-1 == req->rpc_status) {
                args->op_ret = -1;
                goto out;
        }

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
        args->errstr = rsp.op_errstr;
        args->dict = dict_ref (rsp_dict);

out:
        if (ret)
                args->op_ret = ret;
        __wake (args);

        return ret;
}

int
cli_sync_volume_cmd (void *req, struct syncargs *args, call_frame_t *frame,
                     int procnum, xlator_t *this)
{
        int                     ret = -1;

        args->op_ret = -1;
        args->op_errno = ENOTCONN;

        cli_sync_cmd_submit (req, args, frame, cli_rpc_prog, procnum, this,
                             cli_sync_volume_cmd_cbk, (xdrproc_t)xdr_gf_cli_req);

        ret = args->op_ret;

        return ret;
}

int
gf_cli_sync_start_volume (call_frame_t *frame, xlator_t *this, void *data)
{
        int                     ret = 0;
        gf_cli_req              req = {{0,}};
        struct syncargs         args = {0,};
        dict_t                  *dict = NULL;
        char                    *volname = NULL;

        GF_ASSERT (data);

        dict = data;

        ret = dict_get_str (dict, "volname", &volname);
        if (ret)
                goto out;

        ret = dict_allocate_and_serialize (dict, &req.dict.dict_val,
                                           &req.dict.dict_len);
        if (ret < 0) {
                gf_log (this->name, GF_LOG_ERROR, "failed to serialize dict");
                goto out;
        }

        ret = cli_sync_volume_cmd (&req, &args, frame,
                                   GLUSTER_CLI_START_VOLUME, this);

        if (ret) {
                if (args.errstr && strcmp (args.errstr, ""))
                        cli_err ("volume start: %s: failed: %s", volname,
                                 args.errstr);
                else
                        cli_err ("volume start: %s: failed", volname);
        } else
                cli_out ("volume start: %s: success", volname);

out:
        return ret;
}
