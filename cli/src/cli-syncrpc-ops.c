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

int
gf_cli_sync_stop_volume (call_frame_t *frame, xlator_t *this, void *data)
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
                                   GLUSTER_CLI_STOP_VOLUME, this);

        if (ret) {
                if (args.errstr && strcmp (args.errstr, ""))
                        cli_err ("volume stop: %s: failed: %s", volname,
                                 args.errstr);
                else
                        cli_err ("volume stop: %s: failed", volname);
        } else
                cli_out ("volume stop: %s: success", volname);

out:
        return ret;
}

int
gf_cli_sync_get_volume (call_frame_t *frame, xlator_t *this, void *data)
{
        int                             ret = 0;
        gf_cli_req                      req = {{0,}};
        struct syncargs                 args = {0,};
        cli_cmd_volume_get_ctx_t        *ctx = NULL;
        dict_t                          *dict = NULL;
        int                             vol_count = 0;
        char                            *volname = NULL;
        int                             type = 0;
        int                             vol_type = 0;
        int                             status = 0;
        int                             brick_count = 0;
        int                             dist_count = 0;
        int                             stripe_count = 0;
        int                             replica_count = 0;
        int                             transport = 0;
        char                            *volid = NULL;
        int                             opt_count = 0;
        char                            *brick = NULL;
        char                            key[1024] = {0,};
        int                             i = 1;

        GF_ASSERT (data);

        ctx = data;

        dict = dict_new ();
        if (!dict)
                goto out;

        ret = dict_set_str (dict, "volname", ctx->volname);
        if (ret)
                goto out;

        ret = dict_set_int32 (dict, "flags", GF_CLI_GET_VOLUME);
        if (ret)
                goto out;

        ret = dict_allocate_and_serialize (dict, &req.dict.dict_val,
                                           &req.dict.dict_len);
        if (ret < 0) {
                gf_log (this->name, GF_LOG_ERROR, "failed to serialize dict");
                goto out;
        }

        ret = cli_sync_volume_cmd (&req, &args, frame,
                                   GLUSTER_CLI_GET_VOLUME, this);

        if (ret)
                goto out;

        if (!args.dict) {
                cli_err ("No volumes present");
                ret = 0;
                goto out;
        }

        ret = dict_get_int32 (args.dict, "count", &vol_count);
        if (ret)
                goto out;

        if (!vol_count) {
                cli_err ("Volume %s doesn't exist", ctx->volname);
                ret = -1;
                goto out;
        }

        cli_out (" ");

        ret = dict_get_str (args.dict, "volume0.name", &volname);
        if (ret)
                goto out;

        ret = dict_get_int32 (args.dict, "volume0.type", &type);
        if (ret)
                goto out;

        ret = dict_get_int32 (args.dict, "volume0.status", &status);
        if (ret)
                goto out;

        ret = dict_get_int32 (args.dict, "volume0.brick_count", &brick_count);
        if (ret)
                goto out;

        ret = dict_get_int32 (args.dict, "volume0.dist_count", &dist_count);
        if (ret)
                goto out;

        ret = dict_get_int32 (args.dict, "volume0.stripe_count", &stripe_count);
        if (ret)
                goto out;

        ret = dict_get_int32 (args.dict, "volume0.replica_count", &replica_count);
        if (ret)
                goto out;

        ret = dict_get_int32 (args.dict, "volume0.transport", &transport);
        if (ret)
                goto out;

        ret = dict_get_str (args.dict, "volume0.volume_id", &volid);
        if (ret)
                goto out;

        vol_type = type;

        // Distributed (stripe/replicate/stripe-replica) setups
        if ((type > 0) && ( dist_count < brick_count))
                vol_type = type + 3;

        cli_out ("Volume Name: %s", volname);
        cli_out ("Type: %s", cli_vol_type_str[vol_type]);
        cli_out ("Volume ID: %s", volid);
        cli_out ("Status: %s", cli_vol_status_str[status]);

        if (type == GF_CLUSTER_TYPE_STRIPE_REPLICATE) {
                cli_out ("Number of Bricks: %d x %d x %d = %d",
                         (brick_count / dist_count),
                         stripe_count,
                         replica_count,
                         brick_count);

        } else if (type == GF_CLUSTER_TYPE_NONE) {
                cli_out ("Number of Bricks: %d", brick_count);

        } else {
                /* For both replicate and stripe, dist_count is
                   good enough */
                cli_out ("Number of Bricks: %d x %d = %d",
                         (brick_count / dist_count),
                         dist_count, brick_count);
        }

        cli_out ("Transport-type: %s",
                 ((transport == 0)?"tcp":
                  (transport == 1)?"rdma":
                  "tcp,rdma"));
        i = 1;

        if (brick_count)
                cli_out ("Bricks:");

        while (i <= brick_count) {
                memset (key, 0, sizeof (key));
                snprintf (key, 1024, "volume0.brick%d", i);
                ret = dict_get_str (args.dict, key, &brick);
                if (ret)
                        goto out;

                cli_out ("Brick%d: %s", i, brick);
                i++;
        }

        ret = dict_get_int32 (args.dict, "volume0.opt_count", &opt_count);
        if (ret)
                goto out;

        if (!opt_count)
                goto out;

        cli_out ("Options Reconfigured:");
        //TODO: Output options
        cli_out ("<TODO>");
out:
        if (dict)
                dict_unref (dict);

        if (args.dict)
                dict_unref (args.dict);

        return ret;
}
