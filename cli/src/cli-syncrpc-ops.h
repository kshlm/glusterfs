#include "syncop.h"
#include "rpc-clnt.h"
#include "xdr-generic.h"
#include "cli1-xdr.h"
#include "cli.h"
#include "protocol-common.h"
#include "cli-cmd.h"

#define cli_sync_cmd_submit(req, args, frame, prog, procnum, this, cbk, xdrproc) do {   \
                int             ret = -1;                                               \
                                                                                        \
                cli_cmd_sent_status_set (0);                                            \
                __yawn (args);                                                          \
                ret = _cli_sync_cmd_submit (req, args, frame, prog, procnum,            \
                                           this, cbk, xdrproc);                         \
                if (!ret) {                                                             \
                        cli_cmd_sent_status_set (1);                                    \
                        __yield (args);                                                 \
                } else                                                                  \
                        args->op_ret = -1;                                              \
        } while (0);

int
_cli_sync_cmd_submit (void *req, struct syncargs *args, call_frame_t *frame,
                      rpc_clnt_prog_t *prog, int procnum, xlator_t *this,
                      fop_cbk_fn_t cbkfn, xdrproc_t xdrproc);

int
cli_perform_volume_cmd (void *req, call_frame_t *frame, rpc_clnt_prog_t *prog,
                        int procnum, xlator_t *this, xdrproc_t xdrproc);

int
gf_cli_sync_start_volume (call_frame_t *frame, xlator_t *this, void *data);

int
gf_cli_sync_stop_volume (call_frame_t *frame, xlator_t *this, void *data);
