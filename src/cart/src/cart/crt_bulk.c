/* Copyright (C) 2016-2018 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted for any purpose (including commercial purposes)
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the
 *    documentation and/or materials provided with the distribution.
 *
 * 3. In addition, redistributions of modified forms of the source or binary
 *    code must carry prominent notices stating that the original code was
 *    changed and the date of the change.
 *
 *  4. All publications or advertising materials mentioning features or use of
 *     this software are asked, but not required, to acknowledge that it was
 *     developed by Intel Corporation and credit the contributors.
 *
 * 5. Neither the name of Intel Corporation, nor the name of any Contributor
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * This file is part of CaRT. It implements the CaRT bulk related APIs.
 */
#define D_LOGFAC	DD_FAC(bulk)

#include "crt_internal.h"

/** Check the validation of the d_sg_list_t parameter */
static inline bool
crt_sgl_valid(d_sg_list_t *sgl)
{
	d_iov_t	*iov;
	int		i;

	if (sgl == NULL || sgl->sg_nr == 0) {
		if (sgl == NULL)
			D_ERROR("invalid parameter, NULL sgl.\n");
		else
			D_ERROR("invalid parameter, zero sgl.sg_nr.\n");
		return false;
	}

	/* HG_Bulk_create allows to pass in a NULL but_ptrs in which case HG
	 * will internally allocate memory, temporarily not use this feature.
	 */
	if (sgl->sg_iovs == NULL) {
		D_ERROR("invalid parameter, NULL sgl->sg_iovs.\n");
		return false;
	}
	for (i = 0; i < sgl->sg_nr; i++) {
		iov = &sgl->sg_iovs[i];
		if (iov->iov_buf == NULL || iov->iov_buf_len == 0) {
			if (iov->iov_buf == NULL)
				D_ERROR("invalid parameter, sg_iovs[%d]."
					"iov_buf is NULL.\n", i);
			else
				D_ERROR("invalid parameter, sg_iovs[%d]."
					"iov_buf_len is 0.\n", i);
			return false;
		}
	}
	return true;
}

/** check the validation of bulk descriptor */
static inline bool
crt_bulk_desc_valid(struct crt_bulk_desc *bulk_desc)
{
	if (bulk_desc == NULL || bulk_desc->bd_rpc == NULL ||
	    bulk_desc->bd_rpc->cr_ctx == CRT_CONTEXT_NULL ||
	    bulk_desc->bd_remote_hdl == CRT_BULK_NULL ||
	    bulk_desc->bd_local_hdl == CRT_BULK_NULL ||
	    (bulk_desc->bd_bulk_op != CRT_BULK_PUT &&
	     bulk_desc->bd_bulk_op != CRT_BULK_GET) ||
	    bulk_desc->bd_len == 0) {
		if (bulk_desc == NULL) {
			D_ERROR("invalid parameter of NULL bulk_desc.\n");
			return false;
		}
		if (bulk_desc->bd_rpc == NULL) {
			D_ERROR("invalid parameter(NULL bulk_desc->db_rpc).\n");
			return false;
		}
		if (bulk_desc->bd_rpc->cr_ctx == CRT_CONTEXT_NULL) {
			D_ERROR("invalid parameter(NULL bulk_desc->db_rpc"
				"->dr_ctx).\n");
			return false;
		}
		D_ERROR("invalid parameter of bulk_desc (remote_hdl:%p,"
			"local_hdl:%p, bulk_op:%d, len: "DF_U64".\n",
			bulk_desc->bd_remote_hdl,
			bulk_desc->bd_local_hdl,
			bulk_desc->bd_bulk_op, bulk_desc->bd_len);
		return false;
	} else {
		return true;
	}
}

int
crt_bulk_create(crt_context_t crt_ctx, d_sg_list_t *sgl,
		crt_bulk_perm_t bulk_perm, crt_bulk_t *bulk_hdl)
{
	struct crt_context	*ctx;
	int			rc = 0;

	if (crt_ctx == CRT_CONTEXT_NULL || !crt_sgl_valid(sgl) ||
	    /* Now HG treats WO as invalid parameter */
	    (bulk_perm != CRT_BULK_RW && bulk_perm != CRT_BULK_RO /* &&
	     bulk_perm != CRT_BULK_WO */) || bulk_hdl == NULL) {
		D_ERROR("invalid parameter for crt_bulk_create, crt_ctx: %p, "
			"crt_sgl_valid: %d, bulk_perm: %d, bulk_hdl: %p.\n",
			crt_ctx, crt_sgl_valid(sgl), bulk_perm, bulk_hdl);
		D_GOTO(out, rc = -DER_INVAL);
	}

	ctx = crt_ctx;
	rc = crt_hg_bulk_create(&ctx->cc_hg_ctx, sgl, bulk_perm, bulk_hdl);
	if (rc != 0)
		D_ERROR("crt_hg_bulk_create failed, rc: %d.\n", rc);

out:
	return rc;
}

int
crt_bulk_bind(crt_bulk_t bulk_hdl, crt_context_t crt_ctx)
{
	struct crt_context	*ctx = crt_ctx;
	int			rc = 0;

	if (ctx == CRT_CONTEXT_NULL || bulk_hdl == CRT_BULK_NULL) {
		D_ERROR("invalid parameter, NULL crt_ctx or bulk_hdl.\n");
		D_GOTO(out, rc = -DER_INVAL);
	}

	rc = crt_hg_bulk_bind(bulk_hdl, &ctx->cc_hg_ctx);
	if (rc != 0)
		D_ERROR("crt_hg_bulk_bind failed, rc: %d.\n", rc);

out:
	return rc;
}

int
crt_bulk_addref(crt_bulk_t bulk_hdl)
{
	int	rc;

	if (bulk_hdl == CRT_BULK_NULL) {
		D_ERROR("crt_bulk_addref with NULL bulk_hdl.\n");
		D_GOTO(out, rc = -DER_INVAL);
	}

	rc = crt_hg_bulk_addref(bulk_hdl);
	if (rc != 0)
		D_ERROR("crt_hg_bulk_addref failed, rc: %d.\n", rc);

out:
	return rc;
}

int
crt_bulk_free(crt_bulk_t bulk_hdl)
{
	int	rc;

	if (bulk_hdl == CRT_BULK_NULL) {
		D_ERROR("crt_bulk_free with NULL bulk_hdl.\n");
		D_GOTO(out, rc = -DER_INVAL);
	}

	rc = crt_hg_bulk_free(bulk_hdl);
	if (rc != 0)
		D_ERROR("crt_hg_bulk_free failed, rc: %d.\n", rc);

out:
	return rc;
}

int
crt_bulk_transfer(struct crt_bulk_desc *bulk_desc, crt_bulk_cb_t complete_cb,
		  void *arg, crt_bulk_opid_t *opid)
{
	int			rc = 0;

	if (!crt_bulk_desc_valid(bulk_desc)) {
		D_ERROR("invalid parameter of bulk_desc.\n");
		D_GOTO(out, rc = -DER_INVAL);
	}

	rc = crt_hg_bulk_transfer(bulk_desc, complete_cb, arg, opid, false);
	if (rc != 0)
		D_ERROR("crt_hg_bulk_transfer failed, rc: %d.\n", rc);

out:
	return rc;
}

int
crt_bulk_bind_transfer(struct crt_bulk_desc *bulk_desc,
		       crt_bulk_cb_t complete_cb, void *arg,
		       crt_bulk_opid_t *opid)
{
	int			rc = 0;

	if (!crt_bulk_desc_valid(bulk_desc)) {
		D_ERROR("invalid parameter of bulk_desc.\n");
		D_GOTO(out, rc = -DER_INVAL);
	}

	rc = crt_hg_bulk_transfer(bulk_desc, complete_cb, arg, opid, true);
	if (rc != 0)
		D_ERROR("crt_hg_bulk_transfer failed, rc: %d.\n", rc);

out:
	return rc;
}

int
crt_bulk_get_len(crt_bulk_t bulk_hdl, size_t *bulk_len)
{
	return crt_hg_bulk_get_len(bulk_hdl, bulk_len);
}

int
crt_bulk_get_sgnum(crt_bulk_t bulk_hdl, unsigned int *bulk_sgnum)
{
	return crt_hg_bulk_get_sgnum(bulk_hdl, bulk_sgnum);
}

int
crt_bulk_access(crt_bulk_t bulk_hdl, d_sg_list_t *sgl)
{
	int		rc = 0;

	if (bulk_hdl == CRT_BULK_NULL) {
		D_ERROR("invalid parameter, NULL bulk_hdl.\n");
		D_GOTO(out, rc = -DER_INVAL);
	}
	if (sgl == NULL) {
		D_ERROR("invalid parameter, NULL sgl pointer.\n");
		D_GOTO(out, rc = -DER_INVAL);
	}

	rc = crt_hg_bulk_access(bulk_hdl, sgl);

out:
	return rc;
}

int
crt_bulk_abort(crt_context_t crt_ctx, crt_bulk_opid_t opid)
{
	return crt_hg_bulk_cancel(opid);
}
