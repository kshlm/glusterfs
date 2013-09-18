/*
   Copyright (c) 2010-2013 Red Hat, Inc. <http://www.redhat.com>
   This file is part of GlusterFS.

   This file is licensed to you under your choice of the GNU Lesser
   General Public License, version 3 or any later version (LGPLv3 or
   later), or the GNU General Public License, version 2 (GPLv2), in all
   cases as published by the Free Software Foundation.
*/

#ifndef __CLI_SYNC_CMD_H_
#define __CLI_SYNC_CMD_H_

#include "syncop.h"
#include "rpc-clnt.h"
#include "xdr-generic.h"
#include "cli1-xdr.h"
#include "cli.h"
#include "protocol-common.h"
#include "cli-cmd.h"

/* This function performs a synchronus RPC command with glusterd. It is a
 * replacement to cli_cmd_submit().
 *
 * The parameters are,
 *  req     - pointer to the RPC request structure
 *  args    - pointer to the syncargs structure, used to store the state of
 *            command execution
 *  prog    - pointer to the RPC client program
 *  procnum - RPC procnum of the proc to be performed in the RPC program
 *  cbkfn   - callback funtion to process the RPC response from glusterd, and
 *            set the results (op_{ret,errno,errstr}, rsp_dict etc.), in 'args'
 *  xdrproc - xdrproc to be used to serialize 'req'
 *  timeout - timeout for the command in seconds, use 0 for default timeout of
 *            2 minutes (120 seconds) or -1 to disable timeout.
 *
 * The return value will be 'op_ret' returned by glusterd or -1 if failure
 * occurs before performing the actual command.
 *
 * The signature of the cbkfn is,
 *   int (*fop_cbk_fn_t) (struct rpc_req *req, stuct iovec *iov, int count,
 *                        void *frame)
 *
 * For a reference usage of this function, see the implementation of the
 * cli_sync_volume_cmd() defined below.
 */

int
cli_sync_cmd_submit (void *req, struct syncargs *args, rpc_clnt_prog_t *prog,
                     int procnum, fop_cbk_fn_t cbkfn, xdrproc_t xdrproc,
                     int timeout);

/* This function is a wrapper over the cli_sync_cmd_submit() function, to be
 * used only for performing volume commands.
 *
 * The parmeters are,
 *  procnum   - RPC procnum for the volume command
 *  req_dict  - dictionary containing the data needed for the volume command
 *  rsp_dict  - location to store the pointer to the response dictionary given
 *              by glusterd. This needs to be dict_unref()d by the caller, can
 *              be NULL.
 *  op_errstr - location to store the pointer to the error string returned by
 *              glusterd. This needs to be GF_FREE()d by the caller, can be
 *              NULL
 *  timeout   - timeout for the command in seconds, use 0 for default timeout of
 *              2 minutes (120 seconds) or -1 to disable timeout.
 *
 * The return value will be 'op_ret' returned by glusterd or -1 if the command
 * fails before being sent to glusterd. 'errno' will be set to the op_errno
 * returned by glusterd.
 */
int
cli_sync_volume_cmd (int procnum, dict_t *req_dict, dict_t **rsp_dict,
                     char **op_errstr, int timeout);
#endif /* __CLI_SYNC_CMD_H_ */
