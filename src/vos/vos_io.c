/**
 * (C) Copyright 2018-2020 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
 * The Government's rights to use, modify, reproduce, release, perform, display,
 * or disclose this software are subject to the terms of the Apache License as
 * provided in Contract No. B609815.
 * Any reproduction of computer software, computer software documentation, or
 * portions thereof marked with this legend must also reproduce the markings.
 */
/**
 * This file is part of daos
 *
 * vos/vos_io.c
 */
#define D_LOGFAC	DD_FAC(vos)

#include <daos/common.h>
#include <daos/checksum.h>
#include <daos/btree.h>
#include <daos_types.h>
#include <daos_srv/vos.h>
#include <daos.h>
#include "vos_internal.h"
#include "evt_priv.h"

/** I/O context */
struct vos_io_context {
	daos_epoch_range_t	 ic_epr;
	daos_unit_oid_t		 ic_oid;
	struct vos_container	*ic_cont;
	daos_iod_t		*ic_iods;
	struct dcs_iod_csums	*iod_csums;
	/** reference on the object */
	struct vos_object	*ic_obj;
	/** BIO descriptor, has ic_iod_nr SGLs */
	struct bio_desc		*ic_biod;
	struct vos_ts_set	*ic_ts_set;
	/** Checksums for bio_iovs in \ic_biod */
	struct dcs_csum_info	*ic_biov_csums;
	uint32_t		 ic_biov_csums_at;
	uint32_t		 ic_biov_csums_nr;
	/** current dkey info */
	struct vos_ilog_info	 ic_dkey_info;
	/** current akey info */
	struct vos_ilog_info	 ic_akey_info;
	/** cursor of SGL & IOV in BIO descriptor */
	unsigned int		 ic_sgl_at;
	unsigned int		 ic_iov_at;
	/** reserved SCM extents */
	struct vos_rsrvd_scm	 ic_rsrvd_scm;
	/** reserved offsets for SCM update */
	umem_off_t		*ic_umoffs;
	unsigned int		 ic_umoffs_cnt;
	unsigned int		 ic_umoffs_at;
	/** reserved NVMe extents */
	d_list_t		 ic_blk_exts;
	daos_size_t		 ic_space_held[DAOS_MEDIA_MAX];
	/** number DAOS IO descriptors */
	unsigned int		 ic_iod_nr;
	/** IO had a read conflict */
	bool			 ic_read_conflict;
	/** flags */
	unsigned int		 ic_update:1,
				 ic_size_fetch:1,
				 ic_save_recx:1;
	/**
	 * Input shadow recx lists, one for each iod. Now only used for degraded
	 * mode EC obj fetch handling.
	 */
	struct daos_recx_ep_list *ic_shadows;
	/**
	 * Output recx/epoch lists, one for each iod. To save the recx list when
	 * vos_fetch_begin() with VOS_FETCH_RECX_LIST flag. User can get it by
	 * vos_ioh2recx_list() and should free it by daos_recx_ep_list_free().
	 */
	struct daos_recx_ep_list *ic_recx_lists;
};

static inline struct umem_instance *
vos_ioc2umm(struct vos_io_context *ioc)
{
	return &ioc->ic_cont->vc_pool->vp_umm;
}

static struct vos_io_context *
vos_ioh2ioc(daos_handle_t ioh)
{
	return (struct vos_io_context *)ioh.cookie;
}

static daos_handle_t
vos_ioc2ioh(struct vos_io_context *ioc)
{
	daos_handle_t ioh;

	ioh.cookie = (uint64_t)ioc;
	return ioh;
}

static struct dcs_csum_info *
vos_ioc2csum(struct vos_io_context *ioc)
{
	if (ioc->iod_csums != NULL)
		return ioc->iod_csums[ioc->ic_sgl_at].ic_data;
	return NULL;
}

static void
iod_empty_sgl(struct vos_io_context *ioc, unsigned int sgl_at)
{
	struct bio_sglist *bsgl;

	D_ASSERT(sgl_at < ioc->ic_iod_nr);
	ioc->ic_iods[sgl_at].iod_size = 0;
	bsgl = bio_iod_sgl(ioc->ic_biod, sgl_at);
	bsgl->bs_nr_out = 0;
}

static void
vos_ioc_reserve_fini(struct vos_io_context *ioc)
{
	struct vos_rsrvd_scm *rsrvd_scm = &ioc->ic_rsrvd_scm;

	D_ASSERT(d_list_empty(&ioc->ic_blk_exts));
	D_ASSERT(rsrvd_scm->rs_actv_at == 0);

	if (rsrvd_scm->rs_actv_cnt != 0) {
		D_FREE(rsrvd_scm->rs_actv);
		rsrvd_scm->rs_actv = NULL;
	}

	if (ioc->ic_umoffs != NULL) {
		D_FREE(ioc->ic_umoffs);
		ioc->ic_umoffs = NULL;
	}
}

static int
vos_ioc_reserve_init(struct vos_io_context *ioc)
{
	struct vos_rsrvd_scm	*rsrvd_scm;
	int			 i, total_acts = 0;

	if (!ioc->ic_update)
		return 0;

	for (i = 0; i < ioc->ic_iod_nr; i++) {
		daos_iod_t *iod = &ioc->ic_iods[i];

		total_acts += iod->iod_nr;
	}

	D_ALLOC_ARRAY(ioc->ic_umoffs, total_acts);
	if (ioc->ic_umoffs == NULL)
		return -DER_NOMEM;

	if (vos_ioc2umm(ioc)->umm_ops->mo_reserve == NULL)
		return 0;

	rsrvd_scm = &ioc->ic_rsrvd_scm;
	D_ALLOC_ARRAY(rsrvd_scm->rs_actv, total_acts);
	if (rsrvd_scm->rs_actv == NULL)
		return -DER_NOMEM;

	rsrvd_scm->rs_actv_cnt = total_acts;
	return 0;
}

static void
vos_ioc_destroy(struct vos_io_context *ioc, bool evict)
{
	if (ioc->ic_biod != NULL)
		bio_iod_free(ioc->ic_biod);

	if (ioc->ic_biov_csums != NULL)
		D_FREE(ioc->ic_biov_csums);

	if (ioc->ic_obj)
		vos_obj_release(vos_obj_cache_current(), ioc->ic_obj, evict);

	vos_ioc_reserve_fini(ioc);
	vos_ilog_fetch_finish(&ioc->ic_dkey_info);
	vos_ilog_fetch_finish(&ioc->ic_akey_info);
	vos_cont_decref(ioc->ic_cont);
	vos_ts_set_free(ioc->ic_ts_set);
	D_FREE(ioc);
}

static int
vos_ioc_create(daos_handle_t coh, daos_unit_oid_t oid, bool read_only,
	       daos_epoch_t epoch, uint64_t cond_flags, unsigned int iod_nr,
	       daos_iod_t *iods, struct dcs_iod_csums *iod_csums,
	       uint32_t fetch_flags, struct daos_recx_ep_list *shadows,
	       struct vos_io_context **ioc_pp)
{
	struct vos_container *cont;
	struct vos_io_context *ioc = NULL;
	struct bio_io_context *bioc;
	int i, rc;

	if (iod_nr == 0) {
		D_ERROR("Invalid iod_nr (0).\n");
		rc = -DER_IO_INVAL;
		goto error;
	}

	D_ALLOC_PTR(ioc);
	if (ioc == NULL)
		return -DER_NOMEM;

	ioc->ic_iod_nr = iod_nr;
	ioc->ic_iods = iods;
	ioc->ic_epr.epr_hi = epoch;
	ioc->ic_epr.epr_lo = 0;
	ioc->ic_oid = oid;
	ioc->ic_cont = vos_hdl2cont(coh);
	vos_cont_addref(ioc->ic_cont);
	ioc->ic_update = !read_only;
	ioc->ic_size_fetch = ((fetch_flags & VOS_FETCH_SIZE_ONLY) != 0);
	ioc->ic_save_recx = ((fetch_flags & VOS_FETCH_RECX_LIST) != 0);
	ioc->ic_read_conflict = false;
	ioc->ic_umoffs_cnt = ioc->ic_umoffs_at = 0;
	ioc->iod_csums = iod_csums;
	vos_ilog_fetch_init(&ioc->ic_dkey_info);
	vos_ilog_fetch_init(&ioc->ic_akey_info);
	D_INIT_LIST_HEAD(&ioc->ic_blk_exts);
	ioc->ic_shadows = shadows;

	rc = vos_ioc_reserve_init(ioc);
	if (rc != 0)
		goto error;

	rc = vos_ts_set_allocate(&ioc->ic_ts_set, cond_flags, iod_nr);
	if (rc != 0)
		goto error;

	cont = vos_hdl2cont(coh);

	bioc = cont->vc_pool->vp_io_ctxt;
	D_ASSERT(bioc != NULL);
	ioc->ic_biod = bio_iod_alloc(bioc, iod_nr, !read_only);
	if (ioc->ic_biod == NULL) {
		rc = -DER_NOMEM;
		goto error;
	}

	ioc->ic_biov_csums_nr = 1;
	ioc->ic_biov_csums_at = 0;
	D_ALLOC_ARRAY(ioc->ic_biov_csums, ioc->ic_biov_csums_nr);
	if (ioc->ic_biov_csums == NULL) {
		rc = -DER_NOMEM;
		goto error;
	}

	for (i = 0; i < iod_nr; i++) {
		int iov_nr = iods[i].iod_nr;
		struct bio_sglist *bsgl;

		if ((iods[i].iod_type == DAOS_IOD_SINGLE && iov_nr != 1) ||
		    (iov_nr == 0 && iods[i].iod_recxs != NULL)) {
			D_ERROR("Invalid iod_nr=%d, iod_type %d.\n",
				iov_nr, iods[i].iod_type);
			rc = -DER_IO_INVAL;
			goto error;
		}

		/* Don't bother to initialize SGLs for size fetch */
		if (ioc->ic_size_fetch)
			continue;

		bsgl = bio_iod_sgl(ioc->ic_biod, i);
		rc = bio_sgl_init(bsgl, iov_nr);
		if (rc != 0)
			goto error;
	}

	*ioc_pp = ioc;
	return 0;
error:
	if (ioc != NULL)
		vos_ioc_destroy(ioc, false);
	return rc;
}

static int
iod_fetch(struct vos_io_context *ioc, struct bio_iov *biov)
{
	struct bio_sglist *bsgl;
	int iov_nr, iov_at;

	if (ioc->ic_size_fetch)
		return 0;

	bsgl = bio_iod_sgl(ioc->ic_biod, ioc->ic_sgl_at);
	D_ASSERT(bsgl != NULL);
	iov_nr = bsgl->bs_nr;
	iov_at = ioc->ic_iov_at;

	D_ASSERT(iov_nr > iov_at);
	D_ASSERT(iov_nr >= bsgl->bs_nr_out);

	if (iov_at == iov_nr - 1) {
		struct bio_iov *biovs;

		D_REALLOC_ARRAY(biovs, bsgl->bs_iovs, (iov_nr * 2));
		if (biovs == NULL)
			return -DER_NOMEM;

		bsgl->bs_iovs = biovs;
		bsgl->bs_nr = iov_nr * 2;
	}

	bsgl->bs_iovs[iov_at] = *biov;
	bsgl->bs_nr_out++;
	ioc->ic_iov_at++;
	return 0;
}

static int
bsgl_csums_resize(struct vos_io_context *ioc)
{
	struct dcs_csum_info *csums = ioc->ic_biov_csums;
	uint32_t	 dcb_nr = ioc->ic_biov_csums_nr;

	if (ioc->ic_size_fetch)
		return 0;

	if (ioc->ic_biov_csums_at == dcb_nr - 1) {
		struct dcs_csum_info *new_infos;
		uint32_t	 new_nr = dcb_nr * 2;

		D_REALLOC_ARRAY(new_infos, csums, new_nr);
		if (new_infos == NULL)
			return -DER_NOMEM;

		ioc->ic_biov_csums = new_infos;
		ioc->ic_biov_csums_nr = new_nr;
	}

	return 0;
}

/** Save the checksum to a list that can be retrieved later */
static int
save_csum(struct vos_io_context *ioc, struct dcs_csum_info *csum_info,
	  struct evt_entry *entry)
{
	struct dcs_csum_info	*saved_csum_info;
	int			 rc;

	if (ioc->ic_size_fetch)
		return 0;

	rc = bsgl_csums_resize(ioc);
	if (rc != 0)
		return rc;

	/**
	 * it's expected that the csum the csum_info points to is in memory
	 * that will persist until fetch is complete ... so memcpy isn't needed
	 */
	saved_csum_info = &ioc->ic_biov_csums[ioc->ic_biov_csums_at];
	*saved_csum_info = *csum_info;
	if (entry != NULL)
		evt_entry_csum_update(&entry->en_ext, &entry->en_sel_ext,
				      saved_csum_info);

	ioc->ic_biov_csums_at++;

	return 0;
}

/** Fetch the single value within the specified epoch range of an key */
static int
akey_fetch_single(daos_handle_t toh, const daos_epoch_range_t *epr,
		  daos_size_t *rsize, struct vos_io_context *ioc)
{
	struct vos_svt_key	 key;
	struct vos_rec_bundle	 rbund;
	d_iov_t			 kiov; /* iov to carry key bundle */
	d_iov_t			 riov; /* iov to carry record bundle */
	struct bio_iov		 biov; /* iov to return data buffer */
	int			 rc;
	struct dcs_csum_info	csum_info = {0};

	d_iov_set(&kiov, &key, sizeof(key));
	key.sk_epoch	= epr->epr_hi;
	key.sk_minor_epc = VOS_MINOR_EPC_MAX;

	tree_rec_bundle2iov(&rbund, &riov);
	memset(&biov, 0, sizeof(biov));
	rbund.rb_biov	= &biov;
	rbund.rb_csum = &csum_info;

	rc = dbtree_fetch(toh, BTR_PROBE_LE, DAOS_INTENT_DEFAULT, &kiov, &kiov,
			  &riov);
	if (rc == -DER_NONEXIST) {
		rbund.rb_rsize = 0;
		bio_addr_set_hole(&biov.bi_addr, 1);
		rc = 0;
	} else if (rc != 0) {
		goto out;
	} else if (key.sk_epoch < epr->epr_lo) {
		/* The single value is before the valid epoch range (after a
		 * punch when incarnation log is available
		 */
		rc = 0;
		rbund.rb_rsize = 0;
		bio_addr_set_hole(&biov.bi_addr, 1);
	}
	if (ci_is_valid(&csum_info))
		save_csum(ioc, &csum_info, NULL);

	rc = iod_fetch(ioc, &biov);
	if (rc != 0)
		goto out;

	*rsize = rbund.rb_gsize;
out:
	return rc;
}

static inline void
biov_set_hole(struct bio_iov *biov, ssize_t len)
{
	memset(biov, 0, sizeof(*biov));
	bio_iov_set_len(biov, len);
	bio_addr_set_hole(&biov->bi_addr, 1);
}

/**
 * Calculate the bio_iov and extent chunk alignment and set appropriate
 * prefix & suffix on the biov so that whole chunks are fetched in case needed
 * for checksum calculation and verification.
 * Should only be called when the entity has a valid checksum.
 */
static void
biov_align_lens(struct bio_iov *biov, struct evt_entry *ent, daos_size_t rsize)
{
	struct evt_extent aligned_extent;

	aligned_extent = evt_entry_align_to_csum_chunk(ent, rsize);
	bio_iov_set_extra(biov,
			  (ent->en_sel_ext.ex_lo - aligned_extent.ex_lo) *
			  rsize,
			  (aligned_extent.ex_hi - ent->en_sel_ext.ex_hi) *
			  rsize);
}

/**
 * Save to recx/ep list, user can get it by vos_ioh2recx_list() and then free
 * the memory.
 */
static int
save_recx(struct vos_io_context *ioc, uint64_t rx_idx, uint64_t rx_nr,
	  daos_epoch_t ep, int type)
{
	struct daos_recx_ep_list	*recx_list;
	struct daos_recx_ep		 recx_ep;

	if (ioc->ic_recx_lists == NULL) {
		D_ALLOC_ARRAY(ioc->ic_recx_lists, ioc->ic_iod_nr);
		if (ioc->ic_recx_lists == NULL)
			return -DER_NOMEM;
	}

	recx_list = &ioc->ic_recx_lists[ioc->ic_sgl_at];
	recx_ep.re_recx.rx_idx = rx_idx;
	recx_ep.re_recx.rx_nr = rx_nr;
	recx_ep.re_ep = ep;
	recx_ep.re_type = type;

	return daos_recx_ep_add(recx_list, &recx_ep);
}

/** Fetch an extent from an akey */
static int
akey_fetch_recx(daos_handle_t toh, const daos_epoch_range_t *epr,
		daos_recx_t *recx, daos_epoch_t shadow_ep, daos_size_t *rsize_p,
		struct vos_io_context *ioc)
{
	struct evt_entry	*ent;
	/* At present, this is not exposed in interface but passing it toggles
	 * sorting and clipping of rectangles
	 */
	struct evt_entry_array	 ent_array = { 0 };
	struct evt_extent	 extent;
	struct bio_iov		 biov = {0};
	daos_size_t		 holes; /* hole width */
	daos_size_t		 rsize;
	daos_off_t		 index;
	daos_off_t		 end;
	bool			 csum_enabled = false;
	bool			 with_shadow = (shadow_ep != DAOS_EPOCH_MAX);
	int			 rc;

	index = recx->rx_idx;
	end   = recx->rx_idx + recx->rx_nr;

	extent.ex_lo = index;
	extent.ex_hi = end - 1;

	evt_ent_array_init(&ent_array);
	rc = evt_find(toh, epr, &extent, &ent_array);
	if (rc != 0)
		goto failed;

	holes = 0;
	rsize = 0;
	evt_ent_array_for_each(ent, &ent_array) {
		daos_off_t	 lo = ent->en_sel_ext.ex_lo;
		daos_off_t	 hi = ent->en_sel_ext.ex_hi;
		daos_size_t	 nr;

		D_ASSERT(hi >= lo);
		nr = hi - lo + 1;

		if (lo != index) {
			D_ASSERTF(lo > index,
				  DF_U64"/"DF_U64", "DF_EXT", "DF_ENT"\n",
				  lo, index, DP_EXT(&extent),
				  DP_ENT(ent));
			holes += lo - index;
		}

		/* Hole extent, with_shadow case only used for EC obj */
		if (bio_addr_is_hole(&ent->en_addr) ||
		    (with_shadow && (ent->en_epoch < shadow_ep))) {
			index = lo + nr;
			holes += nr;
			continue;
		}

		if (holes != 0) {
			if (with_shadow) {
				rc = save_recx(ioc, lo - holes, holes,
					       shadow_ep, DRT_SHADOW);
				if (rc != 0)
					goto failed;
			}
			biov_set_hole(&biov, holes * ent_array.ea_inob);
			/* skip the hole */
			rc = iod_fetch(ioc, &biov);
			if (rc != 0)
				goto failed;
			holes = 0;
		}

		if (rsize == 0)
			rsize = ent_array.ea_inob;
		D_ASSERT(rsize == ent_array.ea_inob);

		if (ioc->ic_save_recx) {
			rc = save_recx(ioc, lo, nr, ent->en_epoch, DRT_NORMAL);
			if (rc != 0)
				goto failed;
		}
		bio_iov_set(&biov, ent->en_addr, nr * ent_array.ea_inob);

		if (ci_is_valid(&ent->en_csum)) {
			rc = save_csum(ioc, &ent->en_csum, ent);
			if (rc != 0)
				return rc;
			biov_align_lens(&biov, ent, rsize);
			csum_enabled = true;
		} else {
			bio_iov_set_extra(&biov, 0, 0);
			if (csum_enabled)
				D_ERROR("Checksum found in some entries, "
					"but not all\n");
		}

		rc = iod_fetch(ioc, &biov);
		if (rc != 0)
			goto failed;

		index = lo + nr;
	}

	D_ASSERT(index <= end);
	if (index < end)
		holes += end - index;

	if (holes != 0) { /* trailing holes */
		if (with_shadow) {
			rc = save_recx(ioc, end - holes, holes, shadow_ep,
				       DRT_SHADOW);
			if (rc != 0)
				goto failed;
		}
		biov_set_hole(&biov, holes * ent_array.ea_inob);
		rc = iod_fetch(ioc, &biov);
		if (rc != 0)
			goto failed;
	}
	if (rsize_p && *rsize_p == 0)
		*rsize_p = rsize;
failed:
	evt_ent_array_fini(&ent_array);
	return rc;
}

/* Trim the tail holes for the current sgl */
static void
ioc_trim_tail_holes(struct vos_io_context *ioc)
{
	struct bio_sglist *bsgl;
	struct bio_iov *biov;
	int i;

	if (ioc->ic_size_fetch)
		return;

	bsgl = bio_iod_sgl(ioc->ic_biod, ioc->ic_sgl_at);
	for (i = ioc->ic_iov_at - 1; i >= 0; i--) {
		biov = &bsgl->bs_iovs[i];
		if (bio_addr_is_hole(&biov->bi_addr))
			bsgl->bs_nr_out--;
		else
			break;
	}

	if (bsgl->bs_nr_out == 0)
		iod_empty_sgl(ioc, ioc->ic_sgl_at);
}

static int
key_ilog_check(struct vos_io_context *ioc, struct vos_krec_df *krec,
	       const struct vos_ilog_info *parent, daos_epoch_range_t *epr_out,
	       struct vos_ilog_info *info)
{
	struct umem_instance	*umm;
	daos_epoch_range_t	 epr = ioc->ic_epr;
	int			 rc;

	umm = vos_obj2umm(ioc->ic_obj);
	rc = vos_ilog_fetch(umm, vos_cont2hdl(ioc->ic_cont),
			    DAOS_INTENT_DEFAULT, &krec->kr_ilog,
			    epr.epr_hi, 0, parent, info);
	if (rc != 0)
		goto out;

	rc = vos_ilog_check(info, &epr, epr_out, true);
out:
	D_DEBUG(DB_TRACE, "ilog check returned "DF_RC" epr_in="DF_X64"-"DF_X64
		" punch="DF_X64" epr_out="DF_X64"-"DF_X64"\n", DP_RC(rc),
		epr.epr_lo, epr.epr_hi, info->ii_prior_punch,
		epr_out ? epr_out->epr_lo : 0,
		epr_out ? epr_out->epr_hi : 0);
	return rc;
}

static void
akey_fetch_recx_get(daos_recx_t *iod_recx, struct daos_recx_ep_list *shadow,
		    daos_recx_t *fetch_recx, daos_epoch_t *shadow_ep)
{
	struct daos_recx_ep	*recx_ep;
	daos_recx_t		*recx;
	uint32_t		 i;

	if (shadow == NULL)
		goto no_shadow;

	for (i = 0; i < shadow->re_nr; i++) {
		recx_ep = &shadow->re_items[i];
		recx = &recx_ep->re_recx;
		if (!DAOS_RECX_PTR_OVERLAP(iod_recx, recx))
			continue;

		fetch_recx->rx_idx = iod_recx->rx_idx;
		fetch_recx->rx_nr = min((iod_recx->rx_idx + iod_recx->rx_nr),
					(recx->rx_idx + recx->rx_nr)) -
				    iod_recx->rx_idx;
		D_ASSERT(fetch_recx->rx_nr > 0 &&
			 fetch_recx->rx_nr <= iod_recx->rx_nr);
		iod_recx->rx_idx += fetch_recx->rx_nr;
		iod_recx->rx_nr -= fetch_recx->rx_nr;
		*shadow_ep = recx_ep->re_ep;
		return;
	}

no_shadow:
	*fetch_recx = *iod_recx;
	iod_recx->rx_idx += fetch_recx->rx_nr;
	iod_recx->rx_nr -= fetch_recx->rx_nr;
	*shadow_ep = DAOS_EPOCH_MAX;
}

static int
akey_fetch(struct vos_io_context *ioc, daos_handle_t ak_toh)
{
	daos_iod_t		*iod = &ioc->ic_iods[ioc->ic_sgl_at];
	struct vos_krec_df	*krec = NULL;
	daos_epoch_range_t	 val_epr = {0};
	daos_handle_t		 toh = DAOS_HDL_INVAL;
	int			 i, rc;
	int			 flags = 0;
	bool			 is_array = (iod->iod_type == DAOS_IOD_ARRAY);
	struct daos_recx_ep_list *shadow;

	D_DEBUG(DB_IO, "akey "DF_KEY" fetch %s epr "DF_X64"-"DF_X64"\n",
		DP_KEY(&iod->iod_name),
		iod->iod_type == DAOS_IOD_ARRAY ? "array" : "single",
		ioc->ic_epr.epr_lo, ioc->ic_epr.epr_hi);

	if (is_array)
		flags |= SUBTR_EVT;

	rc = key_tree_prepare(ioc->ic_obj, ak_toh,
			      VOS_BTR_AKEY, &iod->iod_name, flags,
			      DAOS_INTENT_DEFAULT, &krec, &toh, ioc->ic_ts_set);

	if (rc != 0) {
		if (rc == -DER_NONEXIST) {
			if (ioc->ic_ts_set &&
			    ioc->ic_ts_set->ts_flags & VOS_OF_COND_AKEY_FETCH)
				goto out;
			D_DEBUG(DB_IO, "Nonexistent akey "DF_KEY"\n",
				DP_KEY(&iod->iod_name));
			iod_empty_sgl(ioc, ioc->ic_sgl_at);
			rc = 0;
		} else {
			D_ERROR("Failed to fetch akey: "DF_RC"\n", DP_RC(rc));
		}
		goto out;
	}

	rc = key_ilog_check(ioc, krec, &ioc->ic_dkey_info, &val_epr,
			    &ioc->ic_akey_info);

	if (rc != 0) {
		if (rc == -DER_NONEXIST) {
			if (ioc->ic_ts_set &&
			    ioc->ic_ts_set->ts_flags & VOS_OF_COND_AKEY_FETCH)
				goto out;
			iod_empty_sgl(ioc, ioc->ic_sgl_at);
			D_DEBUG(DB_IO, "Nonexistent akey %.*s\n",
				(int)iod->iod_name.iov_len,
				(char *)iod->iod_name.iov_buf);
			rc = 0;
		} else {
			D_CDEBUG(rc == -DER_INPROGRESS, DB_IO, DLOG_ERR,
				 "Fetch akey failed: rc="DF_RC"\n",
				 DP_RC(rc));
		}
		goto out;
	}

	if (iod->iod_type == DAOS_IOD_SINGLE) {
		rc = akey_fetch_single(toh, &val_epr, &iod->iod_size, ioc);
		goto out;
	}

	iod->iod_size = 0;
	shadow = (ioc->ic_shadows == NULL) ? NULL :
					     &ioc->ic_shadows[ioc->ic_sgl_at];
	for (i = 0; i < iod->iod_nr; i++) {
		daos_recx_t	iod_recx;
		daos_recx_t	fetch_recx;
		daos_epoch_t	shadow_ep;
		daos_size_t	rsize = 0;

		if (iod->iod_recxs[i].rx_nr == 0) {
			D_DEBUG(DB_IO,
				"Skip empty read IOD at %d: idx %lu, nr %lu\n",
				i, (unsigned long)iod->iod_recxs[i].rx_idx,
				(unsigned long)iod->iod_recxs[i].rx_nr);
			continue;
		}

		iod_recx = iod->iod_recxs[i];
		while (iod_recx.rx_nr > 0) {
			akey_fetch_recx_get(&iod_recx, shadow, &fetch_recx,
					    &shadow_ep);
			rc = akey_fetch_recx(toh, &val_epr, &fetch_recx,
					     shadow_ep, &rsize, ioc);
			if (rc != 0) {
				D_DEBUG(DB_IO, "Failed to fetch index %d: "
					DF_RC"\n", i, DP_RC(rc));
				goto out;
			}
		}

		/*
		 * Empty tree or all holes, DAOS array API relies on zero
		 * iod_size to see if an array cell is empty.
		 */
		if (rsize == 0)
			continue;

		if (iod->iod_size == DAOS_REC_ANY)
			iod->iod_size = rsize;

		if (iod->iod_size != rsize) {
			D_ERROR("Cannot support mixed record size "
				DF_U64"/"DF_U64"\n", iod->iod_size, rsize);
			rc = -DER_INVAL;
			goto out;
		}
	}

	ioc_trim_tail_holes(ioc);
out:
	if (!daos_handle_is_inval(toh))
		key_tree_release(toh, is_array);

	return rc;
}

static void
iod_set_cursor(struct vos_io_context *ioc, unsigned int sgl_at)
{
	D_ASSERT(sgl_at < ioc->ic_iod_nr);
	D_ASSERT(ioc->ic_iods != NULL);

	ioc->ic_sgl_at = sgl_at;
	ioc->ic_iov_at = 0;
}

static int
dkey_fetch(struct vos_io_context *ioc, daos_key_t *dkey)
{
	struct vos_object	*obj = ioc->ic_obj;
	struct vos_krec_df	*krec;
	daos_handle_t		 toh = DAOS_HDL_INVAL;
	int			 i, rc;

	rc = obj_tree_init(obj);
	if (rc != 0)
		return rc;

	rc = key_tree_prepare(obj, obj->obj_toh, VOS_BTR_DKEY,
			      dkey, 0, DAOS_INTENT_DEFAULT, &krec,
			      &toh, ioc->ic_ts_set);

	if (rc == -DER_NONEXIST) {
		if (ioc->ic_ts_set &&
		    ioc->ic_ts_set->ts_flags & VOS_COND_FETCH_MASK)
			goto out;
		for (i = 0; i < ioc->ic_iod_nr; i++)
			iod_empty_sgl(ioc, i);
		D_DEBUG(DB_IO, "Nonexistent dkey\n");
		rc = 0;
		goto out;
	}

	if (rc != 0) {
		D_ERROR("Failed to prepare subtree: "DF_RC"\n", DP_RC(rc));
		goto out;
	}

	rc = key_ilog_check(ioc, krec, &obj->obj_ilog_info, &ioc->ic_epr,
			    &ioc->ic_dkey_info);

	if (rc != 0) {
		if (rc == -DER_NONEXIST) {
			if (ioc->ic_ts_set &&
			    ioc->ic_ts_set->ts_flags & VOS_COND_FETCH_MASK)
				goto out;
			for (i = 0; i < ioc->ic_iod_nr; i++)
				iod_empty_sgl(ioc, i);
			D_DEBUG(DB_IO, "Nonexistent dkey\n");
			rc = 0;
		} else {
			D_CDEBUG(rc == -DER_INPROGRESS, DB_IO, DLOG_ERR,
				 "Fetch dkey failed: rc="DF_RC"\n",
				 DP_RC(rc));
		}
		goto out;
	}

	for (i = 0; i < ioc->ic_iod_nr; i++) {
		iod_set_cursor(ioc, i);
		rc = akey_fetch(ioc, toh);
		if (rc != 0)
			break;
	}
out:
	if (!daos_handle_is_inval(toh))
		key_tree_release(toh, false);

	return rc;
}

int
vos_fetch_end(daos_handle_t ioh, int err)
{
	struct vos_io_context *ioc = vos_ioh2ioc(ioh);

	/* NB: it's OK to use the stale ioc->ic_obj for fetch_end */
	D_ASSERT(!ioc->ic_update);
	vos_ioc_destroy(ioc, false);
	return err;
}

static void
update_ts_on_fetch(struct vos_io_context *ioc, int err)
{
	struct vos_ts_set	*ts_set = ioc->ic_ts_set;
	struct vos_ts_entry	*entry;
	struct vos_ts_entry	*prev;
	int			 akey_idx;

	if (ts_set == NULL)
		return;

	/** Aborted for another reason, no timestamp updates */
	if (err != 0 && err != -DER_NONEXIST)
		return;

	/** Fetch is always a read of the value so always update the
	 *  both akey timestamps regardless
	 */
	entry = vos_ts_set_get_entry_type(ts_set, VOS_TS_TYPE_CONT, 0);
	entry->te_ts_rh = MAX(entry->te_ts_rh, ioc->ic_epr.epr_hi);
	entry = vos_ts_set_get_entry_type(ts_set, VOS_TS_TYPE_OBJ, 0);
	entry->te_ts_rh = MAX(entry->te_ts_rh, ioc->ic_epr.epr_hi);
	prev = entry;
	entry = vos_ts_set_get_entry_type(ts_set, VOS_TS_TYPE_DKEY, 0);

	if (entry == NULL)
		goto update_prev;
	if (ts_set->ts_flags & VOS_OF_COND_DKEY_FETCH)
		entry->te_ts_rl = MAX(entry->te_ts_rl, ioc->ic_epr.epr_hi);
	entry->te_ts_rh = MAX(entry->te_ts_rh, ioc->ic_epr.epr_hi);

	if (entry == prev)
		return;

	prev = entry;

	for (akey_idx = 0; akey_idx < ioc->ic_iod_nr; akey_idx++) {
		entry = vos_ts_set_get_entry_type(ts_set, VOS_TS_TYPE_AKEY,
						  akey_idx);
		if (entry == NULL)
			goto update_prev;

		entry->te_ts_rl = MAX(entry->te_ts_rl, ioc->ic_epr.epr_hi);
		entry->te_ts_rh = MAX(entry->te_ts_rh, ioc->ic_epr.epr_hi);
	}

	return;

update_prev:
	prev->te_ts_rl = MAX(prev->te_ts_rl, ioc->ic_epr.epr_hi);
}

int
vos_fetch_begin(daos_handle_t coh, daos_unit_oid_t oid, daos_epoch_t epoch,
		uint64_t cond_flags, daos_key_t *dkey, unsigned int iod_nr,
		daos_iod_t *iods, uint32_t fetch_flags,
		struct daos_recx_ep_list *shadows, daos_handle_t *ioh,
		struct dtx_handle *dth)
{
	struct vos_io_context	*ioc;
	struct vos_ts_entry	*entry;
	int			 i, rc;

	D_DEBUG(DB_TRACE, "Fetch "DF_UOID", desc_nr %d, epoch "DF_X64"\n",
		DP_UOID(oid), iod_nr, epoch);

	rc = vos_ioc_create(coh, oid, true, epoch, cond_flags, iod_nr, iods,
			    NULL, fetch_flags, shadows, &ioc);
	if (rc != 0)
		return rc;

	if (!vos_ts_lookup(ioc->ic_ts_set, ioc->ic_cont->vc_ts_idx, false,
			   &entry)) {
		/** Re-cache the container timestamps */
		entry = vos_ts_alloc(ioc->ic_ts_set, ioc->ic_cont->vc_ts_idx,
				     0);
	}

	rc = vos_obj_hold(vos_obj_cache_current(), ioc->ic_cont, oid,
			  &ioc->ic_epr, true, DAOS_INTENT_DEFAULT, true,
			  &ioc->ic_obj, ioc->ic_ts_set);
	if (rc != -DER_NONEXIST && rc != 0)
		goto out;

	if (rc == -DER_NONEXIST) {
		if (ioc->ic_ts_set &&
		    ioc->ic_ts_set->ts_flags & VOS_COND_FETCH_MASK)
			goto out;
		rc = 0;
		for (i = 0; i < iod_nr; i++)
			iod_empty_sgl(ioc, i);
	} else {
		rc = dkey_fetch(ioc, dkey);
		if (rc != 0)
			goto out;
	}

	*ioh = vos_ioc2ioh(ioc);

out:
	update_ts_on_fetch(ioc, rc);

	if (rc != 0) {
		daos_recx_ep_list_free(ioc->ic_recx_lists, ioc->ic_iod_nr);
		ioc->ic_recx_lists = NULL;
		return vos_fetch_end(vos_ioc2ioh(ioc), rc);
	}
	return 0;
}

static umem_off_t
iod_update_umoff(struct vos_io_context *ioc)
{
	umem_off_t umoff;

	D_ASSERTF(ioc->ic_umoffs_at < ioc->ic_umoffs_cnt,
		  "Invalid ioc_reserve at/cnt: %u/%u\n",
		  ioc->ic_umoffs_at, ioc->ic_umoffs_cnt);

	umoff = ioc->ic_umoffs[ioc->ic_umoffs_at];
	ioc->ic_umoffs_at++;

	return umoff;
}

static struct bio_iov *
iod_update_biov(struct vos_io_context *ioc)
{
	struct bio_sglist *bsgl;
	struct bio_iov *biov;

	bsgl = bio_iod_sgl(ioc->ic_biod, ioc->ic_sgl_at);
	D_ASSERT(bsgl->bs_nr_out != 0);
	D_ASSERT(bsgl->bs_nr_out > ioc->ic_iov_at);

	biov = &bsgl->bs_iovs[ioc->ic_iov_at];
	ioc->ic_iov_at++;

	return biov;
}

static int
akey_update_single(daos_handle_t toh, uint32_t pm_ver, daos_size_t rsize,
		   daos_size_t gsize, struct vos_io_context *ioc,
		   uint16_t minor_epc)
{
	struct vos_svt_key	 key;
	struct vos_rec_bundle	 rbund;
	struct dcs_csum_info	 csum;
	d_iov_t			 kiov, riov;
	struct bio_iov		*biov;
	umem_off_t		 umoff;
	daos_epoch_t		 epoch = ioc->ic_epr.epr_hi;
	int			 rc;

	ci_set_null(&csum);
	d_iov_set(&kiov, &key, sizeof(key));
	key.sk_epoch		= epoch;
	key.sk_minor_epc	= minor_epc;

	umoff = iod_update_umoff(ioc);
	D_ASSERT(!UMOFF_IS_NULL(umoff));

	D_ASSERT(ioc->ic_iov_at == 0);
	biov = iod_update_biov(ioc);

	tree_rec_bundle2iov(&rbund, &riov);

	struct dcs_csum_info *value_csum = vos_ioc2csum(ioc);

	if (value_csum != NULL)
		rbund.rb_csum	= value_csum;
	else
		rbund.rb_csum	= &csum;

	rbund.rb_biov	= biov;
	rbund.rb_rsize	= rsize;
	rbund.rb_gsize	= gsize;
	rbund.rb_off	= umoff;
	rbund.rb_ver	= pm_ver;

	rc = dbtree_update(toh, &kiov, &riov);
	if (rc != 0)
		D_ERROR("Failed to update subtree: "DF_RC"\n", DP_RC(rc));

	return rc;
}

/**
 * Update a record extent.
 * See comment of vos_recx_fetch for explanation of @off_p.
 */
static int
akey_update_recx(daos_handle_t toh, uint32_t pm_ver, daos_recx_t *recx,
		 struct dcs_csum_info *csum, daos_size_t rsize,
		 struct vos_io_context *ioc, uint16_t minor_epc)
{
	struct evt_entry_in	 ent;
	struct bio_iov		*biov;
	daos_epoch_t		 epoch = ioc->ic_epr.epr_hi;
	int rc;

	D_ASSERT(recx->rx_nr > 0);
	memset(&ent, 0, sizeof(ent));
	ent.ei_rect.rc_epc = epoch;
	ent.ei_rect.rc_ex.ex_lo = recx->rx_idx;
	ent.ei_rect.rc_ex.ex_hi = recx->rx_idx + recx->rx_nr - 1;
	ent.ei_rect.rc_minor_epc = minor_epc;
	ent.ei_ver = pm_ver;
	ent.ei_inob = rsize;

	if (ci_is_valid(csum)) {
		ent.ei_csum = *csum;
	}

	biov = iod_update_biov(ioc);
	ent.ei_addr = biov->bi_addr;
	rc = evt_insert(toh, &ent, NULL);

	return rc;
}

static int
akey_update(struct vos_io_context *ioc, uint32_t pm_ver, daos_handle_t ak_toh,
	    uint16_t minor_epc)
{
	struct vos_object	*obj = ioc->ic_obj;
	struct vos_krec_df	*krec = NULL;
	daos_iod_t		*iod = &ioc->ic_iods[ioc->ic_sgl_at];
	struct dcs_csum_info	*iod_csums = vos_ioc2csum(ioc);
	struct dcs_csum_info	*recx_csum = NULL;
	uint32_t		 update_cond = 0;
	bool			 is_array = (iod->iod_type == DAOS_IOD_ARRAY);
	int			 flags = SUBTR_CREATE;
	daos_handle_t		 toh = DAOS_HDL_INVAL;
	int			 i;
	int			 rc = 0;

	D_DEBUG(DB_TRACE, "akey "DF_KEY" update %s value eph "DF_X64"\n",
		DP_KEY(&iod->iod_name), is_array ? "array" : "single",
		ioc->ic_epr.epr_hi);

	if (is_array)
		flags |= SUBTR_EVT;

	rc = key_tree_prepare(obj, ak_toh, VOS_BTR_AKEY,
			      &iod->iod_name, flags, DAOS_INTENT_UPDATE,
			      &krec, &toh, ioc->ic_ts_set);
	if (rc != 0)
		return rc;

	if (vos_ts_check_rh_conflict(ioc->ic_ts_set, ioc->ic_epr.epr_hi))
		ioc->ic_read_conflict = true;

	if (ioc->ic_ts_set) {
		switch (ioc->ic_ts_set->ts_flags & VOS_COND_AKEY_UPDATE_MASK) {
		case VOS_OF_COND_AKEY_UPDATE:
			update_cond = VOS_ILOG_COND_UPDATE;
			break;
		case VOS_OF_COND_AKEY_INSERT:
			update_cond = VOS_ILOG_COND_INSERT;
			break;
		default:
			break;
		}
	}

	rc = vos_ilog_update(ioc->ic_cont, &krec->kr_ilog, &ioc->ic_epr,
			     &ioc->ic_dkey_info, &ioc->ic_akey_info,
			     update_cond, ioc->ic_ts_set);
	if (update_cond == VOS_ILOG_COND_UPDATE && rc == -DER_NONEXIST) {
		D_DEBUG(DB_IO, "Conditional update on non-existent akey\n");
		goto out;
	}
	if (update_cond == VOS_ILOG_COND_INSERT && rc == -DER_EXIST) {
		D_DEBUG(DB_IO, "Conditional insert on existent akey\n");
		goto out;
	}

	if (rc != 0) {
		VOS_TX_LOG_FAIL(rc, "Failed to update akey ilog: "DF_RC"\n",
				DP_RC(rc));
		goto out;
	}

	if (iod->iod_type == DAOS_IOD_SINGLE) {
		uint64_t	gsize;

		gsize = (iod->iod_recxs == NULL) ? iod->iod_size :
						   (uintptr_t)iod->iod_recxs;
		rc = akey_update_single(toh, pm_ver, iod->iod_size, gsize, ioc,
					minor_epc);
		goto out;
	} /* else: array */

	for (i = 0; i < iod->iod_nr; i++) {
		umem_off_t	umoff = iod_update_umoff(ioc);

		if (iod->iod_recxs[i].rx_nr == 0) {
			D_ASSERT(UMOFF_IS_NULL(umoff));
			D_DEBUG(DB_IO,
				"Skip empty write IOD at %d: idx %lu, nr %lu\n",
				i, (unsigned long)iod->iod_recxs[i].rx_idx,
				(unsigned long)iod->iod_recxs[i].rx_nr);
			continue;
		}

		if (iod_csums != NULL)
			recx_csum = &iod_csums[i];
		rc = akey_update_recx(toh, pm_ver, &iod->iod_recxs[i],
				      recx_csum, iod->iod_size, ioc,
				      minor_epc);
		if (rc != 0)
			goto out;
	}
out:
	if (!daos_handle_is_inval(toh))
		key_tree_release(toh, is_array);

	return rc;
}

static int
dkey_update(struct vos_io_context *ioc, uint32_t pm_ver, daos_key_t *dkey,
	    uint16_t minor_epc)
{
	struct vos_object	*obj = ioc->ic_obj;
	daos_handle_t		 ak_toh;
	struct vos_krec_df	*krec;
	uint32_t		 update_cond = 0;
	bool			 subtr_created = false;
	int			 i, rc;

	rc = obj_tree_init(obj);
	if (rc != 0)
		return rc;

	rc = key_tree_prepare(obj, obj->obj_toh, VOS_BTR_DKEY, dkey,
			      SUBTR_CREATE, DAOS_INTENT_UPDATE, &krec, &ak_toh,
			      ioc->ic_ts_set);
	if (rc != 0) {
		D_ERROR("Error preparing dkey tree: rc="DF_RC"\n", DP_RC(rc));
		goto out;
	}
	subtr_created = true;

	if (vos_ts_check_rl_conflict(ioc->ic_ts_set, ioc->ic_epr.epr_hi))
		ioc->ic_read_conflict = true;

	if (ioc->ic_ts_set) {
		if (ioc->ic_ts_set->ts_flags & VOS_COND_UPDATE_OP_MASK)
			update_cond = VOS_ILOG_COND_UPDATE;
		else if (ioc->ic_ts_set->ts_flags & VOS_OF_COND_DKEY_INSERT)
			update_cond = VOS_ILOG_COND_INSERT;
	}

	rc = vos_ilog_update(ioc->ic_cont, &krec->kr_ilog, &ioc->ic_epr,
			     &obj->obj_ilog_info, &ioc->ic_dkey_info,
			     update_cond, ioc->ic_ts_set);
	if (update_cond == VOS_ILOG_COND_UPDATE && rc == -DER_NONEXIST) {
		D_DEBUG(DB_IO, "Conditional update on non-existent akey\n");
		goto out;
	}
	if (update_cond == VOS_ILOG_COND_INSERT && rc == -DER_EXIST) {
		D_DEBUG(DB_IO, "Conditional insert on existent akey\n");
		goto out;
	}
	if (rc != 0) {
		VOS_TX_LOG_FAIL(rc, "Failed to update dkey ilog: "DF_RC"\n",
				DP_RC(rc));
		goto out;
	}

	for (i = 0; i < ioc->ic_iod_nr; i++) {
		iod_set_cursor(ioc, i);

		rc = akey_update(ioc, pm_ver, ak_toh, minor_epc);
		if (rc != 0)
			goto out;
	}
out:
	if (!subtr_created)
		return rc;

	if (rc != 0)
		goto release;

release:
	key_tree_release(ak_toh, false);

	return rc;
}

daos_size_t
vos_recx2irec_size(daos_size_t rsize, struct dcs_csum_info *csum)
{
	struct vos_rec_bundle	rbund;

	rbund.rb_csum	= csum;
	rbund.rb_rsize	= rsize;

	return vos_irec_size(&rbund);
}

umem_off_t
vos_reserve_scm(struct vos_container *cont, struct vos_rsrvd_scm *rsrvd_scm,
		daos_size_t size)
{
	umem_off_t	umoff;

	D_ASSERT(size > 0);

	if (vos_cont2umm(cont)->umm_ops->mo_reserve != NULL) {
		struct pobj_action *act;

		D_ASSERT(rsrvd_scm->rs_actv_cnt > rsrvd_scm->rs_actv_at);
		D_ASSERT(rsrvd_scm->rs_actv != NULL);
		act = &rsrvd_scm->rs_actv[rsrvd_scm->rs_actv_at];

		umoff = umem_reserve(vos_cont2umm(cont), act, size);
		if (!UMOFF_IS_NULL(umoff))
			rsrvd_scm->rs_actv_at++;
	} else {
		umoff = umem_alloc(vos_cont2umm(cont), size);
	}

	return umoff;
}

int
vos_reserve_blocks(struct vos_container *cont, d_list_t *rsrvd_nvme,
		   daos_size_t size, enum vos_io_stream ios, uint64_t *off)
{
	struct vea_space_info	*vsi;
	struct vea_hint_context	*hint_ctxt;
	struct vea_resrvd_ext	*ext;
	uint32_t		 blk_cnt;
	int			 rc;

	vsi = vos_cont2pool(cont)->vp_vea_info;
	D_ASSERT(vsi);

	hint_ctxt = cont->vc_hint_ctxt[ios];
	D_ASSERT(hint_ctxt);

	blk_cnt = vos_byte2blkcnt(size);

	rc = vea_reserve(vsi, blk_cnt, hint_ctxt, rsrvd_nvme);
	if (rc)
		return rc;

	ext = d_list_entry(rsrvd_nvme->prev, struct vea_resrvd_ext, vre_link);
	D_ASSERTF(ext->vre_blk_cnt == blk_cnt, "%u != %u\n",
		  ext->vre_blk_cnt, blk_cnt);
	D_ASSERT(ext->vre_blk_off != 0);

	*off = ext->vre_blk_off << VOS_BLK_SHIFT;
	return 0;
}

static int
reserve_space(struct vos_io_context *ioc, uint16_t media, daos_size_t size,
	      uint64_t *off)
{
	int	rc;

	if (media == DAOS_MEDIA_SCM) {
		umem_off_t	umoff;

		umoff = vos_reserve_scm(ioc->ic_cont, &ioc->ic_rsrvd_scm, size);
		if (!UMOFF_IS_NULL(umoff)) {
			ioc->ic_umoffs[ioc->ic_umoffs_cnt] = umoff;
			ioc->ic_umoffs_cnt++;
			*off = umoff;
			return 0;
		}
		D_ERROR("Reserve "DF_U64" from SCM failed.\n", size);
		return -DER_NOSPACE;
	}

	D_ASSERT(media == DAOS_MEDIA_NVME);
	rc = vos_reserve_blocks(ioc->ic_cont, &ioc->ic_blk_exts, size,
				VOS_IOS_GENERIC, off);
	if (rc)
		D_ERROR("Reserve "DF_U64" from NVMe failed. "DF_RC"\n",
			size, DP_RC(rc));
	return rc;
}

static int
iod_reserve(struct vos_io_context *ioc, struct bio_iov *biov)
{
	struct bio_sglist *bsgl;

	bsgl = bio_iod_sgl(ioc->ic_biod, ioc->ic_sgl_at);
	D_ASSERT(bsgl->bs_nr != 0);
	D_ASSERT(bsgl->bs_nr > bsgl->bs_nr_out);
	D_ASSERT(bsgl->bs_nr > ioc->ic_iov_at);

	bsgl->bs_iovs[ioc->ic_iov_at] = *biov;
	ioc->ic_iov_at++;
	bsgl->bs_nr_out++;

	D_DEBUG(DB_TRACE, "media %hu offset "DF_U64" size %zd\n",
		biov->bi_addr.ba_type, biov->bi_addr.ba_off,
		bio_iov2len(biov));
	return 0;
}

/* Reserve single value record on specified media */
static int
vos_reserve_single(struct vos_io_context *ioc, uint16_t media,
		   daos_size_t size)
{
	struct vos_irec_df	*irec;
	daos_size_t		 scm_size;
	umem_off_t		 umoff;
	struct bio_iov		 biov;
	uint64_t		 off = 0;
	int			 rc;
	struct dcs_csum_info	*value_csum = vos_ioc2csum(ioc);

	/*
	 * TODO:
	 * To eliminate internal fragmentaion, misaligned record (record size
	 * isn't aligned with 4K) on NVMe could be split into two parts, large
	 * aligned part will be stored on NVMe and being referenced by
	 * vos_irec_df->ir_ex_addr, small unaligned part will be stored on SCM
	 * along with vos_irec_df, being referenced by vos_irec_df->ir_body.
	 */
	scm_size = (media == DAOS_MEDIA_SCM) ?
		vos_recx2irec_size(size, value_csum) :
		vos_recx2irec_size(0, value_csum);

	rc = reserve_space(ioc, DAOS_MEDIA_SCM, scm_size, &off);
	if (rc) {
		D_ERROR("Reserve SCM for SV failed. "DF_RC"\n", DP_RC(rc));
		return rc;
	}

	D_ASSERT(ioc->ic_umoffs_cnt > 0);
	umoff = ioc->ic_umoffs[ioc->ic_umoffs_cnt - 1];
	irec = (struct vos_irec_df *) umem_off2ptr(vos_ioc2umm(ioc), umoff);
	vos_irec_init_csum(irec, value_csum);

	memset(&biov, 0, sizeof(biov));
	if (size == 0) { /* punch */
		bio_addr_set_hole(&biov.bi_addr, 1);
		goto done;
	}

	if (media == DAOS_MEDIA_SCM) {
		char *payload_addr;

		/* Get the record payload offset */
		payload_addr = vos_irec2data(irec);
		D_ASSERT(payload_addr >= (char *)irec);
		off = umoff + (payload_addr - (char *)irec);
	} else {
		rc = reserve_space(ioc, DAOS_MEDIA_NVME, size, &off);
		if (rc) {
			D_ERROR("Reserve NVMe for SV failed. "DF_RC"\n",
				DP_RC(rc));
			return rc;
		}
	}
done:
	bio_addr_set(&biov.bi_addr, media, off);
	bio_iov_set_len(&biov, size);
	rc = iod_reserve(ioc, &biov);

	return rc;
}

static int
vos_reserve_recx(struct vos_io_context *ioc, uint16_t media, daos_size_t size)
{
	struct bio_iov	biov;
	uint64_t	off = 0;
	int		rc;

	memset(&biov, 0, sizeof(biov));
	/* recx punch */
	if (size == 0 || media != DAOS_MEDIA_SCM) {
		ioc->ic_umoffs[ioc->ic_umoffs_cnt] = UMOFF_NULL;
		ioc->ic_umoffs_cnt++;
		if (size == 0) {
			bio_addr_set_hole(&biov.bi_addr, 1);
			goto done;
		}
	}

	/*
	 * TODO:
	 * To eliminate internal fragmentaion, misaligned recx (total recx size
	 * isn't aligned with 4K) on NVMe could be split into two evtree rects,
	 * larger rect will be stored on NVMe and small reminder on SCM.
	 */
	rc = reserve_space(ioc, media, size, &off);
	if (rc) {
		D_ERROR("Reserve recx failed. "DF_RC"\n", DP_RC(rc));
		return rc;
	}
done:
	bio_addr_set(&biov.bi_addr, media, off);
	bio_iov_set_len(&biov, size);
	rc = iod_reserve(ioc, &biov);

	return rc;
}

static int
akey_update_begin(struct vos_io_context *ioc)
{
	daos_iod_t *iod = &ioc->ic_iods[ioc->ic_sgl_at];
	int i, rc;

	if (iod->iod_type == DAOS_IOD_SINGLE && iod->iod_nr != 1) {
		D_ERROR("Invalid sv iod_nr=%d\n", iod->iod_nr);
		return -DER_IO_INVAL;
	}

	for (i = 0; i < iod->iod_nr; i++) {
		daos_size_t size;
		uint16_t media;

		size = (iod->iod_type == DAOS_IOD_SINGLE) ? iod->iod_size :
				iod->iod_recxs[i].rx_nr * iod->iod_size;

		media = vos_media_select(vos_cont2pool(ioc->ic_cont),
					 iod->iod_type, size);

		if (iod->iod_type == DAOS_IOD_SINGLE)
			rc = vos_reserve_single(ioc, media, size);
		else
			rc = vos_reserve_recx(ioc, media, size);
		if (rc)
			return rc;
	}
	return 0;
}

static int
dkey_update_begin(struct vos_io_context *ioc)
{
	int i, rc = 0;

	for (i = 0; i < ioc->ic_iod_nr; i++) {
		iod_set_cursor(ioc, i);
		rc = akey_update_begin(ioc);
		if (rc != 0)
			break;
	}

	return rc;
}

int
vos_publish_scm(struct vos_container *cont, struct vos_rsrvd_scm *rsrvd_scm,
		bool publish)
{
	int	rc = 0;

	D_ASSERT(rsrvd_scm->rs_actv_at <= rsrvd_scm->rs_actv_cnt);
	if (rsrvd_scm->rs_actv_at == 0)
		return 0;

	D_ASSERT(rsrvd_scm->rs_actv != NULL);
	if (publish)
		rc = umem_tx_publish(vos_cont2umm(cont), rsrvd_scm->rs_actv,
				     rsrvd_scm->rs_actv_at);
	else
		umem_cancel(vos_cont2umm(cont), rsrvd_scm->rs_actv,
			    rsrvd_scm->rs_actv_at);

	rsrvd_scm->rs_actv_at = 0;
	return rc;
}

/* Publish or cancel the NVMe block reservations */
int
vos_publish_blocks(struct vos_container *cont, d_list_t *blk_list, bool publish,
		   enum vos_io_stream ios)
{
	struct vea_space_info	*vsi;
	struct vea_hint_context	*hint_ctxt;
	int			 rc;

	if (d_list_empty(blk_list))
		return 0;

	vsi = cont->vc_pool->vp_vea_info;
	D_ASSERT(vsi);
	hint_ctxt = cont->vc_hint_ctxt[ios];
	D_ASSERT(hint_ctxt);

	rc = publish ? vea_tx_publish(vsi, hint_ctxt, blk_list) :
		       vea_cancel(vsi, hint_ctxt, blk_list);
	if (rc)
		D_ERROR("Error on %s NVMe reservations. "DF_RC"\n",
			publish ? "publish" : "cancel", DP_RC(rc));

	return rc;
}

static void
update_cancel(struct vos_io_context *ioc)
{

	/* Cancel SCM reservations or free persistent allocations */
	if (vos_cont2umm(ioc->ic_cont)->umm_ops->mo_reserve != NULL) {
		vos_publish_scm(ioc->ic_cont, &ioc->ic_rsrvd_scm, false);
	} else if (ioc->ic_umoffs_cnt != 0) {
		struct umem_instance *umem = vos_ioc2umm(ioc);
		int i;

		D_ASSERT(umem->umm_id == UMEM_CLASS_VMEM);

		for (i = 0; i < ioc->ic_umoffs_cnt; i++) {
			if (!UMOFF_IS_NULL(ioc->ic_umoffs[i]))
				/* Ignore umem_free failure. */
				umem_free(umem, ioc->ic_umoffs[i]);
		}
	}

	/* Cancel NVMe reservations */
	vos_publish_blocks(ioc->ic_cont, &ioc->ic_blk_exts, false,
			   VOS_IOS_GENERIC);
}

static void
update_ts_on_update(struct vos_io_context *ioc, int err)
{
	struct vos_ts_set	*ts_set = ioc->ic_ts_set;
	struct vos_ts_entry	*entry;
	struct vos_ts_entry	*centry;
	int			 akey_idx;

	if (ts_set == NULL)
		return;

	/** No conditional flags, so no timestamp updates */
	if ((ts_set->ts_flags & VOS_COND_UPDATE_MASK) == 0)
		return;

	/** Aborted for another reason, no timestamp updates */
	if (err != 0 && err != -DER_NONEXIST && err != -DER_EXIST)
		return;

	if (err == 0) {
		/** the update succeeded so any negative entries used for
		 *  checks should be changed to positive entries
		 */
		vos_ts_set_upgrade(ts_set);
	}

	entry = vos_ts_set_get_entry_type(ts_set, VOS_TS_TYPE_CONT, 0);
	entry->te_ts_rh = MAX(entry->te_ts_rh, ioc->ic_epr.epr_hi);
	entry = vos_ts_set_get_entry_type(ts_set, VOS_TS_TYPE_OBJ, 0);
	entry->te_ts_rh = MAX(entry->te_ts_rh, ioc->ic_epr.epr_hi);
	centry = vos_ts_set_get_entry_type(ts_set, VOS_TS_TYPE_DKEY, 0);
	if (centry == NULL) {
		entry->te_ts_rl = MAX(entry->te_ts_rl, ioc->ic_epr.epr_hi);
		return;
	}
	entry = centry;
	if (ts_set->ts_flags & VOS_COND_DKEY_UPDATE_MASK)
		entry->te_ts_rl = MAX(entry->te_ts_rl, ioc->ic_epr.epr_hi);
	entry->te_ts_rh = MAX(entry->te_ts_rh, ioc->ic_epr.epr_hi);

	if ((ts_set->ts_flags & VOS_COND_AKEY_UPDATE_MASK) == 0)
		return;

	for (akey_idx = 0; akey_idx < ioc->ic_iod_nr; akey_idx++) {
		centry = vos_ts_set_get_entry_type(ts_set, VOS_TS_TYPE_AKEY,
						  akey_idx);
		if (centry == NULL) {
			entry->te_ts_rl = MAX(entry->te_ts_rl,
					      ioc->ic_epr.epr_hi);
			continue;
		}
		centry->te_ts_rl = MAX(entry->te_ts_rl, ioc->ic_epr.epr_hi);
		centry->te_ts_rh = MAX(entry->te_ts_rh, ioc->ic_epr.epr_hi);
	}
}

int
vos_update_end(daos_handle_t ioh, uint32_t pm_ver, daos_key_t *dkey, int err,
	       struct dtx_handle *dth)
{
	struct vos_dtx_act_ent	**daes = NULL;
	struct vos_io_context	*ioc = vos_ioh2ioc(ioh);
	struct umem_instance	*umem;
	struct vos_ts_entry	*entry;
	uint64_t		time = 0;

	VOS_TIME_START(time, VOS_UPDATE_END);
	D_ASSERT(ioc->ic_update);

	if (err != 0)
		goto out;

	if (!vos_ts_lookup(ioc->ic_ts_set, ioc->ic_cont->vc_ts_idx, false,
			   &entry)) {
		/** Re-cache the container timestamps */
		entry = vos_ts_alloc(ioc->ic_ts_set, ioc->ic_cont->vc_ts_idx,
				     0);
	}

	if (vos_ts_check_rl_conflict(ioc->ic_ts_set, ioc->ic_epr.epr_hi))
		ioc->ic_read_conflict = true;

	umem = vos_ioc2umm(ioc);

	err = umem_tx_begin(umem, vos_txd_get());
	if (err)
		goto out;

	vos_dth_set(dth);

	/* Commit the CoS DTXs via the IO PMDK transaction. */
	if (dth != NULL && dth->dth_dti_cos_count > 0) {
		D_ALLOC_ARRAY(daes, dth->dth_dti_cos_count);
		if (daes == NULL)
			D_GOTO(abort, err = -DER_NOMEM);

		err = vos_dtx_commit_internal(ioc->ic_cont, dth->dth_dti_cos,
					      dth->dth_dti_cos_count,
					      0, NULL, daes);
		if (err <= 0)
			D_FREE(daes);
	}

	err = vos_obj_hold(vos_obj_cache_current(), ioc->ic_cont, ioc->ic_oid,
			  &ioc->ic_epr, false, DAOS_INTENT_UPDATE, true,
			  &ioc->ic_obj, ioc->ic_ts_set);
	if (err != 0)
		goto abort;

	/** Check object timestamp */
	if (vos_ts_check_rl_conflict(ioc->ic_ts_set, ioc->ic_epr.epr_hi))
		ioc->ic_read_conflict = true;

	/* Publish SCM reservations */
	err = vos_publish_scm(ioc->ic_cont, &ioc->ic_rsrvd_scm, true);
	if (err) {
		D_ERROR("Publish SCM failed. "DF_RC"\n", DP_RC(err));
		goto abort;
	}

	/* Update tree index */
	err = dkey_update(ioc, pm_ver, dkey, dth != NULL ? dth->dth_op_seq : 1);
	if (err) {
		VOS_TX_LOG_FAIL(err, "Failed to update tree index: "DF_RC"\n",
				DP_RC(err));
		goto abort;
	}

	/** Now that we are past the existence checks, ensure there isn't a
	 * read conflict
	 */
	if (ioc->ic_read_conflict) {
		err = -DER_TX_RESTART;
		goto abort;
	}

	/* Publish NVMe reservations */
	err = vos_publish_blocks(ioc->ic_cont, &ioc->ic_blk_exts, true,
				 VOS_IOS_GENERIC);

	if (dth != NULL && err == 0)
		err = vos_dtx_prepared(dth);

abort:
	err = err ? umem_tx_abort(umem, err) : umem_tx_commit(umem);
out:
	if (err != 0) {
		vos_dtx_cleanup_dth(dth);
		update_cancel(ioc);
	} else if (daes != NULL) {
		vos_dtx_post_handle(ioc->ic_cont, daes,
				    dth->dth_dti_cos_count, false);
	}

	D_FREE(daes);

	update_ts_on_update(ioc, err);

	VOS_TIME_END(time, VOS_UPDATE_END);
	vos_space_unhold(vos_cont2pool(ioc->ic_cont), &ioc->ic_space_held[0]);

	vos_ioc_destroy(ioc, err != 0);
	vos_dth_set(NULL);

	return err;
}

int
vos_update_begin(daos_handle_t coh, daos_unit_oid_t oid, daos_epoch_t epoch,
		 uint64_t flags, daos_key_t *dkey, unsigned int iod_nr,
		 daos_iod_t *iods, struct dcs_iod_csums *iods_csums,
		 daos_handle_t *ioh, struct dtx_handle *dth)
{
	struct vos_io_context	*ioc;
	int			 rc;

	D_DEBUG(DB_TRACE, "Prepare IOC for "DF_UOID", iod_nr %d, epc "DF_X64
		"\n", DP_UOID(oid), iod_nr, dth ? dth->dth_epoch :  epoch);

	rc = vos_ioc_create(coh, oid, false, dth ? dth->dth_epoch : epoch,
			    flags, iod_nr, iods, iods_csums, 0, NULL, &ioc);
	if (rc != 0)
		return rc;

	/* flags may have VOS_OF_CRIT to skip sys/held checks here */
	rc = vos_space_hold(vos_cont2pool(ioc->ic_cont), flags, dkey, iod_nr,
			    iods, iods_csums, &ioc->ic_space_held[0]);
	if (rc != 0) {
		D_ERROR(DF_UOID": Hold space failed. "DF_RC"\n",
			DP_UOID(oid), DP_RC(rc));
		goto error;
	}

	rc = dkey_update_begin(ioc);
	if (rc != 0) {
		D_ERROR(DF_UOID"dkey update begin failed. %d\n", DP_UOID(oid),
			rc);
		goto error;
	}
	*ioh = vos_ioc2ioh(ioc);
	return 0;
error:
	vos_update_end(vos_ioc2ioh(ioc), 0, dkey, rc, dth);
	return rc;
}

struct daos_recx_ep_list *
vos_ioh2recx_list(daos_handle_t ioh)
{
	return vos_ioh2ioc(ioh)->ic_recx_lists;
}

struct bio_desc *
vos_ioh2desc(daos_handle_t ioh)
{
	struct vos_io_context *ioc = vos_ioh2ioc(ioh);

	D_ASSERT(ioc->ic_biod != NULL);
	return ioc->ic_biod;
}

struct dcs_csum_info *
vos_ioh2ci(daos_handle_t ioh)
{
	struct vos_io_context *ioc = vos_ioh2ioc(ioh);

	return ioc->ic_biov_csums;
}

uint32_t
vos_ioh2ci_nr(daos_handle_t ioh)
{
	struct vos_io_context *ioc = vos_ioh2ioc(ioh);

	return ioc->ic_biov_csums_at;
}

struct bio_sglist *
vos_iod_sgl_at(daos_handle_t ioh, unsigned int idx)
{
	struct vos_io_context *ioc = vos_ioh2ioc(ioh);

	if (idx > ioc->ic_iod_nr) {
		D_ERROR("Invalid SGL index %d >= %d\n",
			idx, ioc->ic_iod_nr);
		return NULL;
	}
	return bio_iod_sgl(ioc->ic_biod, idx);
}

/**
 * @defgroup vos_obj_update() & vos_obj_fetch() functions
 * @{
 */

/**
 * vos_obj_update() & vos_obj_fetch() are two helper functions used
 * for inline update and fetch, so far it's used by rdb, rebuild and
 * some test programs (daos_perf, vos tests, etc).
 *
 * Caveat: These two functions may yield, please use with caution.
 */
static int
vos_obj_copy(struct vos_io_context *ioc, d_sg_list_t *sgls,
	     unsigned int sgl_nr)
{
	int rc, err;

	D_ASSERT(sgl_nr == ioc->ic_iod_nr);
	rc = bio_iod_prep(ioc->ic_biod);
	if (rc)
		return rc;

	err = bio_iod_copy(ioc->ic_biod, sgls, sgl_nr);
	rc = bio_iod_post(ioc->ic_biod);

	return err ? err : rc;
}

int
vos_obj_update(daos_handle_t coh, daos_unit_oid_t oid, daos_epoch_t epoch,
	       uint32_t pm_ver, uint64_t flags, daos_key_t *dkey,
	       unsigned int iod_nr, daos_iod_t *iods,
	       struct dcs_iod_csums *iods_csums, d_sg_list_t *sgls)
{
	daos_handle_t ioh;
	int rc;

	rc = vos_update_begin(coh, oid, epoch, flags, dkey, iod_nr, iods,
			      iods_csums, &ioh, NULL);
	if (rc) {
		D_ERROR("Update "DF_UOID" failed "DF_RC"\n", DP_UOID(oid),
			DP_RC(rc));
		return rc;
	}

	if (sgls) {
		rc = vos_obj_copy(vos_ioh2ioc(ioh), sgls, iod_nr);
		if (rc)
			D_ERROR("Copy "DF_UOID" failed "DF_RC"\n", DP_UOID(oid),
				DP_RC(rc));
	}

	rc = vos_update_end(ioh, pm_ver, dkey, rc, NULL);
	return rc;
}

int
vos_obj_fetch(daos_handle_t coh, daos_unit_oid_t oid, daos_epoch_t epoch,
	      uint64_t flags, daos_key_t *dkey, unsigned int iod_nr,
	      daos_iod_t *iods, d_sg_list_t *sgls)
{
	daos_handle_t	ioh;
	bool		size_fetch = (sgls == NULL);
	uint32_t	fetch_flags = size_fetch ? VOS_FETCH_SIZE_ONLY : 0;
	int		rc;

	rc = vos_fetch_begin(coh, oid, epoch, flags, dkey, iod_nr, iods,
			     fetch_flags, NULL, &ioh, NULL);
	if (rc) {
		if (rc == -DER_INPROGRESS)
			D_DEBUG(DB_TRACE, "Cannot fetch "DF_UOID" because of "
				"conflict modification: "DF_RC"\n",
				DP_UOID(oid), DP_RC(rc));
		else
			D_ERROR("Fetch "DF_UOID" failed "DF_RC"\n",
				DP_UOID(oid), DP_RC(rc));
		return rc;
	}

	if (!size_fetch) {
		struct vos_io_context *ioc = vos_ioh2ioc(ioh);
		int i, j;

		for (i = 0; i < iod_nr; i++) {
			struct bio_sglist *bsgl = bio_iod_sgl(ioc->ic_biod, i);
			d_sg_list_t *sgl = &sgls[i];

			/* Inform caller the nonexistent of object/key */
			if (bsgl->bs_nr_out == 0) {
				for (j = 0; j < sgl->sg_nr; j++)
					sgl->sg_iovs[j].iov_len = 0;
			}
		}

		rc = vos_obj_copy(ioc, sgls, iod_nr);
		if (rc)
			D_ERROR("Copy "DF_UOID" failed "DF_RC"\n",
				DP_UOID(oid), DP_RC(rc));
	}

	rc = vos_fetch_end(ioh, rc);
	return rc;
}

/**
 * @} vos_obj_update() & vos_obj_fetch() functions
 */
