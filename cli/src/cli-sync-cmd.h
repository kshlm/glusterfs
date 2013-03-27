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

int
cli_sync_cmd_submit (void *req, struct syncargs *args, rpc_clnt_prog_t *prog,
                     int procnum, fop_cbk_fn_t cbkfn, xdrproc_t xdrproc);

int
cli_sync_volume_cmd (int procnum, dict_t *req_dict, dict_t **rsp_dict,
                     char **op_errstr);
#endif /* __CLI_SYNC_CMD_H_ */
