/**
 * (C) Copyright 2017 Intel Corporation.
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
 * rdb: Internal Declarations
 */

#ifndef RDB_INTERNAL_H
#define RDB_INTERNAL_H

#include <abt.h>
#include <raft.h>
#include <gurt/hash.h>
#include <daos/lru.h>
#include <daos/rpc.h>
#include "rdb_layout.h"

/* rdb_raft.c (parts required by struct rdb) **********************************/

enum rdb_raft_event_type {
	RDB_RAFT_STEP_UP,
	RDB_RAFT_STEP_DOWN
};

struct rdb_raft_event {
	enum rdb_raft_event_type	dre_type;
	uint64_t			dre_term;
};

/* rdb.c **********************************************************************/

struct rdb {
	/* General fields */
	d_list_t		d_entry;	/* in rdb_hash */
	uuid_t			d_uuid;		/* of database */
	ABT_mutex		d_mutex;	/* mainly for using CVs */
	int			d_ref;		/* of callers and RPCs */
	ABT_cond		d_ref_cv;	/* for d_ref decrements */
	struct rdb_cbs	       *d_cbs;		/* callers' callbacks */
	void		       *d_arg;		/* for d_cbs callbacks */
	struct daos_lru_cache  *d_kvss;		/* rdb_kvs cache */
	daos_handle_t		d_pool;		/* VOS pool */
	daos_handle_t		d_mc;		/* metadata container */

	/* rdb_raft fields */
	raft_server_t	       *d_raft;
	daos_handle_t		d_lc;		/* log container */
	struct rdb_lc_record	d_lc_record;	/* of d_lc */
	d_rank_list_t	       *d_replicas;
	uint64_t		d_applied;	/* last applied index */
	uint64_t		d_debut;	/* first entry in a term */
	ABT_cond		d_applied_cv;	/* for d_applied updates */
	struct d_hash_table	d_results;	/* rdb_raft_result hash */
	d_list_t		d_requests;	/* RPCs waiting for replies */
	d_list_t		d_replies;	/* RPCs received replies */
	ABT_cond		d_replies_cv;	/* for d_replies enqueues */
	struct rdb_raft_event	d_events[2];	/* rdb_raft_events queue */
	int			d_nevents;	/* d_events queue len from 0 */
	ABT_cond		d_events_cv;	/* for d_events enqueues */
	bool			d_stop;		/* for rdb_stop() */
	ABT_thread		d_timerd;
	ABT_thread		d_callbackd;
	ABT_thread		d_recvd;
};

/* Current rank */
#define DF_RANK "%u"
static inline d_rank_t
DP_RANK(void)
{
	d_rank_t	rank;
	int		rc;

	rc = crt_group_rank(NULL, &rank);
	D_ASSERTF(rc == 0, "%d\n", rc);
	return rank;
}

#define DF_DB		DF_UUID"["DF_RANK"]"
#define DP_DB(db)	DP_UUID(db->d_uuid), DP_RANK()

/* Number of "base" references that the rdb_stop() path expects to remain */
#define RDB_BASE_REFS 1

int rdb_hash_init(void);
void rdb_hash_fini(void);
void rdb_get(struct rdb *db);
void rdb_put(struct rdb *db);
struct rdb *rdb_lookup(const uuid_t uuid);

void rdb_start_handler(crt_rpc_t *rpc);
int rdb_start_aggregator(crt_rpc_t *source, crt_rpc_t *result, void *priv);
void rdb_stop_handler(crt_rpc_t *rpc);
int rdb_stop_aggregator(crt_rpc_t *source, crt_rpc_t *result, void *priv);

/* rdb_raft.c *****************************************************************/

/* Per-raft_node_t data */
struct rdb_raft_node {
	d_rank_t	dn_rank;
};

int rdb_raft_init(daos_handle_t pool, daos_handle_t mc,
		  const d_rank_list_t *replicas);
int rdb_raft_start(struct rdb *db);
void rdb_raft_stop(struct rdb *db);
void rdb_raft_resign(struct rdb *db, uint64_t term);
int rdb_raft_verify_leadership(struct rdb *db);
int rdb_raft_append_apply(struct rdb *db, void *entry, size_t size,
			  void *result);
int rdb_raft_wait_applied(struct rdb *db, uint64_t index, uint64_t term);
void rdb_requestvote_handler(crt_rpc_t *rpc);
void rdb_appendentries_handler(crt_rpc_t *rpc);
void rdb_raft_process_reply(struct rdb *db, raft_node_t *node, crt_rpc_t *rpc);
void rdb_raft_free_request(struct rdb *db, crt_rpc_t *rpc);

/* rdb_rpc.c ******************************************************************/

/*
 * RPC operation codes
 *
 * These are for daos_rpc::dr_opc and DAOS_RPC_OPCODE(opc, ...) rather than
 * crt_req_create(..., opc, ...). See src/include/daos/rpc.h.
 */
enum rdb_operation {
	RDB_REQUESTVOTE		= 1,
	RDB_APPENDENTRIES	= 2,
	RDB_START		= 3,
	RDB_STOP		= 4
};

struct rdb_op_in {
	uuid_t	ri_uuid;
};

struct rdb_op_out {
	int32_t		ro_rc;
	uint32_t	ro_padding;
};

struct rdb_requestvote_in {
	struct rdb_op_in	rvi_op;
	msg_requestvote_t	rvi_msg;
};

struct rdb_requestvote_out {
	struct rdb_op_out		rvo_op;
	msg_requestvote_response_t	rvo_msg;
};

struct rdb_appendentries_in {
	struct rdb_op_in	aei_op;
	msg_appendentries_t	aei_msg;
};

struct rdb_appendentries_out {
	struct rdb_op_out		aeo_op;
	msg_appendentries_response_t	aeo_msg;
};

enum rdb_start_flag {
	RDB_AF_CREATE	= 1
};

struct rdb_start_in {
	uuid_t			dai_uuid;
	uuid_t			dai_pool;
	uint32_t		dai_flags;	/* rdb_start_flag */
	uint32_t		dai_padding;
	uint64_t		dai_size;
	d_rank_list_t	       *dai_ranks;
};

struct rdb_start_out {
	int	dao_rc;
};

enum rdb_stop_flag {
	RDB_OF_DESTROY	= 1
};

struct rdb_stop_in {
	uuid_t		doi_pool;
	uint32_t	doi_flags;	/* rdb_stop_flag */
};

struct rdb_stop_out {
	int	doo_rc;
};

extern struct daos_rpc rdb_srv_rpcs[];

int rdb_create_raft_rpc(crt_opcode_t opc, raft_node_t *node, crt_rpc_t **rpc);
int rdb_send_raft_rpc(crt_rpc_t *rpc, struct rdb *db, raft_node_t *node);
int rdb_abort_raft_rpcs(struct rdb *db);
int rdb_create_bcast(crt_opcode_t opc, crt_group_t *group, crt_rpc_t **rpc);
void rdb_recvd(void *arg);

/* rdb_tx.c *******************************************************************/

int rdb_tx_apply(struct rdb *db, uint64_t index, const void *buf, size_t len,
		 void *result);

/* rdb_kvs.c ******************************************************************/

/* KVS cache entry */
struct rdb_kvs {
	struct daos_llink	de_entry;	/* in LRU */
	rdb_path_t		de_path;
	rdb_oid_t		de_object;
	uint8_t			de_buf[];	/* for de_path */
};

int rdb_kvs_cache_create(struct daos_lru_cache **cache);
void rdb_kvs_cache_destroy(struct daos_lru_cache *cache);
void rdb_kvs_cache_evict(struct daos_lru_cache *cache);
int rdb_kvs_lookup(struct rdb *db, const rdb_path_t *path, uint64_t index,
		   bool alloc, struct rdb_kvs **kvs);
void rdb_kvs_put(struct rdb *db, struct rdb_kvs *kvs);
void rdb_kvs_evict(struct rdb *db, struct rdb_kvs *kvs);

/* rdb_path.c *****************************************************************/

int rdb_path_clone(const rdb_path_t *path, rdb_path_t *new_path);
typedef int (*rdb_path_iterate_cb_t)(daos_iov_t *key, void *arg);
int rdb_path_iterate(const rdb_path_t *path, rdb_path_iterate_cb_t cb,
		     void *arg);
int rdb_path_pop(rdb_path_t *path);

/* rdb_util.c *****************************************************************/

#define DF_IOV		"<%p,"DF_U64">"
#define DP_IOV(iov)	(iov)->iov_buf, (iov)->iov_len

extern const daos_size_t rdb_iov_max;
size_t rdb_encode_iov(const daos_iov_t *iov, void *buf);
ssize_t rdb_decode_iov(const void *buf, size_t len, daos_iov_t *iov);
ssize_t rdb_decode_iov_backward(const void *buf_end, size_t len,
				daos_iov_t *iov);

void rdb_oid_to_uoid(rdb_oid_t oid, daos_unit_oid_t *uoid);

int rdb_vos_fetch(daos_handle_t cont, daos_epoch_t epoch, rdb_oid_t oid,
		  daos_key_t *akey, daos_iov_t *value);
int rdb_vos_fetch_addr(daos_handle_t cont, daos_epoch_t epoch, rdb_oid_t oid,
		       daos_key_t *akey, daos_iov_t *value);
int rdb_vos_iter_fetch(daos_handle_t cont, daos_epoch_t epoch, rdb_oid_t oid,
		       enum rdb_probe_opc opc, daos_key_t *akey_in,
		       daos_key_t *akey_out, daos_iov_t *value);
int rdb_vos_iterate(daos_handle_t cont, daos_epoch_t epoch, rdb_oid_t oid,
		    bool backward, rdb_iterate_cb_t cb, void *arg);
int rdb_vos_update(daos_handle_t cont, daos_epoch_t epoch, rdb_oid_t oid, int n,
		   daos_iov_t akeys[], daos_iov_t values[]);
int rdb_vos_punch(daos_handle_t cont, daos_epoch_t epoch, rdb_oid_t oid, int n,
		  daos_iov_t akeys[]);
int rdb_vos_discard(daos_handle_t cont, daos_epoch_t low, daos_epoch_t high);

/*
 * Maximal number of a-keys (i.e., the n parameter) passed to an
 * rdb_mc_update() call. Bumping this number increases the stack usage of
 * rdb_vos_update().
 */
#define RDB_VOS_BATCH_MAX 2

/* Update n (<= RDB_VOS_BATCH_MAX) a-keys atomically. */
static inline int
rdb_mc_update(daos_handle_t mc, rdb_oid_t oid, int n, daos_iov_t akeys[],
	      daos_iov_t values[])
{
	D_DEBUG(DB_TRACE, "mc="DF_X64" oid="DF_X64" n=%d akeys[0]=<%p, %zd> "
		"values[0]=<%p, %zd>\n", mc.cookie, oid, n, akeys[0].iov_buf,
		akeys[0].iov_len, values[0].iov_buf, values[0].iov_len);
	return rdb_vos_update(mc, RDB_MC_EPOCH, oid, n, akeys, values);
}

static inline int
rdb_mc_lookup(daos_handle_t mc, rdb_oid_t oid, daos_iov_t *akey,
	      daos_iov_t *value)
{
	D_DEBUG(DB_TRACE, "mc="DF_X64" oid="DF_X64" akey=<%p, %zd> "
		"value=<%p, %zd, %zd>\n", mc.cookie, oid, akey->iov_buf,
		akey->iov_len, value->iov_buf, value->iov_buf_len,
		value->iov_len);
	return rdb_vos_fetch(mc, RDB_MC_EPOCH, oid, akey, value);
}

static inline int
rdb_lc_update(daos_handle_t lc, uint64_t index, rdb_oid_t oid, int n,
	      daos_iov_t akeys[], daos_iov_t values[])
{
	D_DEBUG(DB_TRACE, "lc="DF_X64" index="DF_U64" oid="DF_X64
		" n=%d akeys[0]=<%p, %zd> values[0]=<%p, %zd>\n", lc.cookie,
		index, oid, n, akeys[0].iov_buf, akeys[0].iov_len,
		values[0].iov_buf, values[0].iov_len);
	return rdb_vos_update(lc, index, oid, n, akeys, values);
}

static inline int
rdb_lc_punch(daos_handle_t lc, uint64_t index, rdb_oid_t oid, int n,
	     daos_iov_t akeys[])
{
	if (n > 0)
		D_DEBUG(DB_TRACE, "lc="DF_X64" index="DF_U64" oid="DF_X64
			" n=%d akeys[0]=<%p, %zd>\n", lc.cookie, index, oid, n,
			akeys[0].iov_buf, akeys[0].iov_len);
	else
		D_DEBUG(DB_TRACE, "lc="DF_X64" index="DF_U64" oid="DF_X64
			" n=%d\n", lc.cookie, index, oid, n);
	return rdb_vos_punch(lc, index, oid, n, akeys);
}

/* Discard index range [low, high]. */
static inline int
rdb_lc_discard(daos_handle_t lc, uint64_t low, uint64_t high)
{
	D_DEBUG(DB_TRACE, "lc="DF_X64" low="DF_U64" high="DF_U64"\n", lc.cookie,
		low, high);
	return rdb_vos_discard(lc, low, high);
}

static inline int
rdb_lc_lookup(daos_handle_t lc, uint64_t index, rdb_oid_t oid,
	      daos_iov_t *akey, daos_iov_t *value)
{
	D_DEBUG(DB_TRACE, "lc="DF_X64" index="DF_U64" oid="DF_X64
		" akey=<%p, %zd> value=<%p, %zd, %zd>\n", lc.cookie, index, oid,
		akey->iov_buf, akey->iov_len, value->iov_buf,
		value->iov_buf_len, value->iov_len);
	if (value->iov_buf == NULL)
		return rdb_vos_fetch_addr(lc, index, oid, akey, value);
	else
		return rdb_vos_fetch(lc, index, oid, akey, value);
}

static inline int
rdb_lc_iter_fetch(daos_handle_t lc, uint64_t index, rdb_oid_t oid,
		  enum rdb_probe_opc opc, daos_iov_t *akey_in,
		  daos_iov_t *akey_out, daos_iov_t *value)
{
	D_DEBUG(DB_TRACE, "lc="DF_X64" index="DF_U64" oid="DF_X64" opc=%d"
		" akey_in=<%p, %zd> akey_out=<%p, %zd> value=<%p, %zd, %zd>\n",
		lc.cookie, index, oid, opc,
		akey_in == NULL ? NULL : akey_in->iov_buf,
		akey_in == NULL ? 0 : akey_in->iov_len,
		akey_out == NULL ? NULL : akey_out->iov_buf,
		akey_out == NULL ? 0 : akey_out->iov_len,
		value == NULL ? NULL : value->iov_buf,
		value == NULL ? 0 : value->iov_buf_len,
		value == NULL ? 0 : value->iov_len);
	return rdb_vos_iter_fetch(lc, index, oid, opc, akey_in, akey_out,
				  value);
}

static inline int
rdb_lc_iterate(daos_handle_t lc, uint64_t index, rdb_oid_t oid, bool backward,
	       rdb_iterate_cb_t cb, void *arg)
{
	D_DEBUG(DB_TRACE, "lc="DF_X64" index="DF_U64" oid="DF_X64
		" backward=%d\n", lc.cookie, index, oid, backward);
	return rdb_vos_iterate(lc, index, oid, backward, cb, arg);
}

#endif /* RDB_INTERNAL_H */
