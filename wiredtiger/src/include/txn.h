/*-
 * Copyright (c) 2014-2017 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */
//����ID��ΧĬ����WT_TXN_NONE��WT_TXN_ABORTED֮�䣬����������������⼸��ֵ
//��ʾ�����������ύ�ģ��ѽ�����ע��:Ϊ��״̬��session,�п���������checkpoint�������ο�__checkpoint_prepare
#define	WT_TXN_NONE	0		/* No txn running in a session. */
//��ʼ״̬
#define	WT_TXN_FIRST	1		/* First transaction to run. */
//��ʾ�������ǻع��˵�
#define	WT_TXN_ABORTED	UINT64_MAX	/* Update rolled back, ignore. */

/*
 * Transaction ID comparison dealing with edge cases.
 *
 * WT_TXN_ABORTED is the largest possible ID (never visible to a running
 * transaction), WT_TXN_NONE is smaller than any possible ID (visible to all
 * running transactions).
 */
#define	WT_TXNID_LE(t1, t2)						\
	((t1) <= (t2))

#define	WT_TXNID_LT(t1, t2)						\
	((t1) < (t2))

//��ǰsession���ڴ���������״̬��Ϣ
#define	WT_SESSION_TXN_STATE(s) (&S2C(s)->txn_global.states[(s)->id])

#define	WT_SESSION_IS_CHECKPOINT(s)					\
	((s)->id != 0 && (s)->id == S2C(s)->txn_global.checkpoint_id)

/*
 * Perform an operation at the specified isolation level.
 *
 * This is fiddly: we can't cope with operations that begin transactions
 * (leaving an ID allocated), and operations must not move our published
 * snap_min forwards (or updates we need could be freed while this operation is
 * in progress).  Check for those cases: the bugs they cause are hard to debug.
 */
#define	WT_WITH_TXN_ISOLATION(s, iso, op) do {				\
	WT_TXN_ISOLATION saved_iso = (s)->isolation;		        \
	WT_TXN_ISOLATION saved_txn_iso = (s)->txn.isolation;		\
	WT_TXN_STATE *txn_state = WT_SESSION_TXN_STATE(s);		\
	WT_TXN_STATE saved_state = *txn_state;				\
	(s)->txn.forced_iso++;						\
	(s)->isolation = (s)->txn.isolation = (iso);			\
	op;								\
	(s)->isolation = saved_iso;					\
	(s)->txn.isolation = saved_txn_iso;				\
	WT_ASSERT((s), (s)->txn.forced_iso > 0);                        \
	(s)->txn.forced_iso--;						\
	WT_ASSERT((s), txn_state->id == saved_state.id &&		\
	    (txn_state->metadata_pinned == saved_state.metadata_pinned ||\
	    saved_state.metadata_pinned == WT_TXN_NONE) &&		\
	    (txn_state->pinned_id == saved_state.pinned_id ||		\
	    saved_state.pinned_id == WT_TXN_NONE));			\
	txn_state->metadata_pinned = saved_state.metadata_pinned;	\
	txn_state->pinned_id = saved_state.pinned_id;			\
} while (0)

//�������գ��ο�__wt_txn_named_snapshot_begin
struct __wt_named_snapshot {//�ýṹ��Ӧ���������մ��뵽__wt_txn_global.snapshot����
	const char *name;

	TAILQ_ENTRY(__wt_named_snapshot) q;

	uint64_t id, pinned_id, snap_min, snap_max;
	uint64_t *snapshot;
	uint32_t snapshot_count;
};

//__wt_txn_global.states  ��¼����session������id��Ϣ
//ͨ��WT_SESSION_TXN_STATEȡֵ
struct __wt_txn_state {
	WT_CACHE_LINE_PAD_BEGIN
	volatile uint64_t id; /* ִ�����������ID����ֵ��__wt_txn_id_alloc */
	//��txn_global->currentС����Сid��Ҳ������oldest id��ӽ���δ�ύ����id
	volatile uint64_t pinned_id; //��ֵ��__wt_txn_get_snapshot
	//��ʾ��ǰsession������checkpoint������Ҳ���ǵ�ǰsession��checkpoint��id��
	//ע��:Ϊ��״̬��session,�п���������checkpoint�������ο�__checkpoint_prepare
	volatile uint64_t metadata_pinned; //��ֵ��__wt_txn_get_snapshot 

	WT_CACHE_LINE_PAD_END
};

//һ��conn�а������session,ÿ��session��һ����Ӧ������txn��Ϣ
//�ýṹ����ȫ���������__wt_connection_impl.txn_global  ��ȫ�����������conn
struct __wt_txn_global {
    // ȫ��д����ID��������,һֱ����  __wt_txn_id_alloc������ 
	volatile uint64_t current;	/* Current transaction ID. */

	/* The oldest running transaction ID (may race). */
	
	volatile uint64_t last_running;
	/*
	 * The oldest transaction ID that is not yet visible to some
	 * transaction in the system.
	 */ //ϵͳ����������һ���ִ��(Ҳ���ǻ�δ�ύ)��д����ID����ֵ��__wt_txn_update_oldest
	 //δ�ύ��������С��һ������id��ֻ��С�ڸ�ֵ��id������ǿɼ��ģ���__txn_visible_all_id
	volatile uint64_t oldest_id; //��ֵ��__wt_txn_update_oldest

    /* timestamp��صĸ�ֵ��__wt_txn_global_set_timestamp __wt_txn_commit*/
	//WT_DECL_TIMESTAMP(commit_timestamp)
	//ʵ����������session��Ӧ������commit_timestamp���ģ���__wt_txn_commit
	//��Ч�ж���__wt_txn_update_pinned_timestamp->__txn_global_query_timestamp��ʵ������ͨ��Ӱ��pinned_timestamp(__wt_txn_visible_all)��Ӱ��ɼ��Ե�
	wt_timestamp_t commit_timestamp;
	/*
    WiredTiger �ṩ���� oldest timestamp �Ĺ��ܣ������� MongoDB �����ø�ʱ�����������Read as of a timestamp 
    �����ṩ��С��ʱ���������һ���Զ���Ҳ����˵��WiredTiger ����ά�� oldest timestamp ֮ǰ��������ʷ�汾��
    MongoDB ����ҪƵ������ʱ������ oldest timestamp�������� WT cache ѹ��̫��
	*/
	//WT_DECL_TIMESTAMP(oldest_timestamp)
	//WT_DECL_TIMESTAMP(pinned_timestamp)
	/*
	�����ж���̣߳�ÿ���̵߳�session�ڵ��øú�������oldest_timestamp���ã���txn_global->oldest_timestamp
	����Щ�����е����ֵ���ο�__wt_txn_global_set_timestamp
	*/ //���ü���__wt_timestamp_validate  
	//��Ч�ж���__wt_txn_update_pinned_timestamp->__txn_global_query_timestamp��ʵ������ͨ��Ӱ��pinned_timestamp(__wt_txn_visible_all)��Ӱ��ɼ��Ե�
	wt_timestamp_t oldest_timestamp; //����ʹ�ÿ��Բο�thread_ts_run
	//��Ч��__wt_txn_visible_all
	wt_timestamp_t pinned_timestamp; //��ֵ��__wt_txn_update_pinned_timestamp
	/*
    4.0 �汾ʵ���˴洢�����Ļع����ƣ������Ƽ��ڵ���Ҫ�ع�ʱ��ֱ�ӵ��� WiredTiger �ӿڣ������ݻع���
    ĳ���ȶ��汾��ʵ���Ͼ���һ�� Checkpoint��������ȶ��汾�������� stable timestamp��WiredTiger ��ȷ�� 
    stable timestamp ֮������ݲ���д�� Checkpoint�MongoDB ���ݸ��Ƽ���ͬ��״̬���������Ѿ�ͬ�������
    ���ڵ�ʱ��Majority commited��������� stable timestamp����Ϊ��Щ�����Ѿ��ύ��������ڵ��ˣ�һ����
    �ᷢ�� ROLLBACK�����ʱ���֮ǰ�����ݾͶ�����д�� Checkpoint ���ˡ�
	*/
	//WT_DECL_TIMESTAMP(stable_timestamp) //����ʹ�ÿ��Բο�thread_ts_run
	/*
	�����ж���̣߳�ÿ���̵߳�session�ڵ��øú�������stable_timestamp���ã���txn_global->stable_timestamp
	����Щ�����е����ֵ���ο�__wt_txn_global_set_timestamp
	*/ 
	//��ֵ����__wt_timestamp_validate
	//��Ч�ж���__wt_txn_update_pinned_timestamp->__txn_global_query_timestamp��ʵ������ͨ��Ӱ��pinned_timestamp(__wt_txn_visible_all)��Ӱ��ɼ��Ե�
	wt_timestamp_t stable_timestamp; //��ֵͨ��mongodb����__conn_set_timestamp->__wt_txn_global_set_timestampʵ��
	bool has_commit_timestamp;
	bool has_oldest_timestamp;
	bool has_pinned_timestamp; //Ϊtrue��ʾpinned_timestamp����oldest_timestamp����__wt_txn_update_pinned_timestamp
	bool has_stable_timestamp;
	bool oldest_is_pinned; //pinned_timestamp����oldest_timestamp
	bool stable_is_pinned;

	WT_SPINLOCK id_lock;

	/* Protects the active transaction states. */
	WT_RWLOCK rwlock;

	/* Protects logging, checkpoints and transaction visibility. */
	WT_RWLOCK visibility_rwlock;

	/* List of transactions sorted by commit timestamp. */
	WT_RWLOCK commit_timestamp_rwlock;
	//ʱ�����ص����񶼻���ӵ��������У���__wt_txn_set_commit_timestamp
	TAILQ_HEAD(__wt_txn_cts_qh, __wt_txn) commit_timestamph;
	uint32_t commit_timestampq_len; //commit_timestamph���еĳ�Ա����

	/* List of transactions sorted by read timestamp. */
	WT_RWLOCK read_timestamp_rwlock;
	TAILQ_HEAD(__wt_txn_rts_qh, __wt_txn) read_timestamph;
	uint32_t read_timestampq_len;

	/*
	 * Track information about the running checkpoint. The transaction
	 * snapshot used when checkpointing are special. Checkpoints can run
	 * for a long time so we keep them out of regular visibility checks.
	 * Eviction and checkpoint operations know when they need to be aware
	 * of checkpoint transactions.
	 *
	 * We rely on the fact that (a) the only table a checkpoint updates is
	 * the metadata; and (b) once checkpoint has finished reading a table,
	 * it won't revisit it.
	 */
	//˵������checkpoint�����У���__txn_checkpoint_wrapper
	volatile bool	  checkpoint_running;	/* Checkpoint running */
	//��checkpointʱ���session��Ӧ��id����ֵ��__checkpoint_prepare
	volatile uint32_t checkpoint_id;	/* Checkpoint's session ID */
	//��checkpoint����session��state����ֵ��__checkpoint_prepare   ��Ч��__wt_txn_oldest_id
	WT_TXN_STATE	  checkpoint_state;	/* Checkpoint's txn state */
	//��checkpoint����session��txn����ֵ��__checkpoint_prepare
	WT_TXN           *checkpoint_txn;	/* Checkpoint's txn structure */

	volatile uint64_t metadata_pinned;	/* Oldest ID for metadata */

	/* Named snapshot state. */
	WT_RWLOCK nsnap_rwlock;
	volatile uint64_t nsnap_oldest_id;
	TAILQ_HEAD(__wt_nsnap_qh, __wt_named_snapshot) nsnaph;

    //���飬��ͬsession��WT_TXN_STATE��¼���������Ӧλ�ã���WT_SESSION_TXN_STATE
    //�洢������session������id��Ϣ���ο�__wt_txn_am_oldest
    //����һ�����ͻ����ռ䣬��__wt_txn_global_init��ÿ��session��Ӧ��WT_TXN_STATE�ĸ�ֵ��__wt_txn_get_snapshot
	WT_TXN_STATE *states;		/* Per-session transaction states */ 
};

//���Բο�https://blog.csdn.net/yuanrxdu/article/details/78339295
/*
���Խ�� MongoDB�´洢����WiredTigerʵ��(����ƪ) https://www.jianshu.com/p/f053e70f9b18�ο�������뼶��
*/
/* wiredtiger ����������ͣ���Ч��__wt_txn_visible->__txn_visible_id */
typedef enum __wt_txn_isolation { //��ֵ��__wt_txn_config
	WT_ISO_READ_COMMITTED,
	WT_ISO_READ_UNCOMMITTED,
	WT_ISO_SNAPSHOT
} WT_TXN_ISOLATION;

/*
 * WT_TXN_OP --
 *	A transactional operation.  Each transaction builds an in-memory array
 *	of these operations as it runs, then uses the array to either write log
 *	records during commit or undo the operations during rollback.
 */ //��ֵ��__wt_txn_modify  ���ڼ�¼���ֲ���
 //WT_TXN��__wt_txn_op��__txn_next_op�й�������    __wt_txn.mod�����Ա
struct __wt_txn_op {
	uint32_t fileid; //��ֵ��__txn_next_op  ��Ӧbtree id
	enum {
		WT_TXN_OP_BASIC,
		WT_TXN_OP_BASIC_TS,
		WT_TXN_OP_INMEM,
		WT_TXN_OP_REF,
		WT_TXN_OP_TRUNCATE_COL,
		WT_TXN_OP_TRUNCATE_ROW
	} type; //��ֵ��__wt_txn_modify
	union { 
		/* WT_TXN_OP_BASIC, WT_TXN_OP_INMEM */
		WT_UPDATE *upd;
		/* WT_TXN_OP_REF */
		WT_REF *ref;
		/* WT_TXN_OP_TRUNCATE_COL */
		struct {
			uint64_t start, stop;
		} truncate_col;
		/* WT_TXN_OP_TRUNCATE_ROW */
		struct {
			WT_ITEM start, stop;
			enum {
				WT_TXN_TRUNC_ALL,
				WT_TXN_TRUNC_BOTH,
				WT_TXN_TRUNC_START,
				WT_TXN_TRUNC_STOP
			} mode;
		} truncate_row;
	} u; //��ÿ��key��update��������������__wt_txn_modify
};

/*
 * WT_TXN --
 *	Per-session transaction context.
 */
//WT_SESSION_IMPL.txn��ԱΪ������
//WT_TXN��__wt_txn_op��__txn_next_op�й�������
struct __wt_txn {//WT_SESSION_IMPL.txn��Ա��ÿ��session���ж�Ӧ��txn
    //���������ȫ��Ψһ��ID�����ڱ�ʾ�����޸����ݵİ汾�ţ�Ҳ���Ƕ��session�����в�ͬ��id��ÿ��������һ�����ظ�id����__wt_txn_id_alloc
	uint64_t id; /*����ID*/ //��ֵ��__wt_txn_id_alloc

    //��Ч��__txn_visible_id */
	WT_TXN_ISOLATION isolation; /*���뼶��*/ //��ֵ��__wt_txn_config

	uint32_t forced_iso;	/* Isolation is currently forced. */

	/*
	 * Snapshot data:
	 *	ids < snap_min are visible,
	 *	ids > snap_max are invisible,
	 *	everything else is visible unless it is in the snapshot.
	 */ //�����Χ�ڵ������ʾ��ǰϵͳ�����ڲ��������񣬲ο�https://blog.csdn.net/yuanrxdu/article/details/78339295
	uint64_t snap_min, snap_max;
	//ϵͳ����������飬����ϵͳ�����е��������,�����������ִ�����������������������
	//��ǰ����ʼ���߲���ʱ����������ִ���Ҳ�δ�ύ�����񼯺�,�����������
	uint64_t *snapshot; //snapshot���飬��Ӧ__wt_txn_init   ��������Ĭ����__txn_sort_snapshot�а���id����
	uint32_t snapshot_count; //txn->snapshot�������ж��ٸ���Ա
	//��Ч��__wt_txn_log_commit  //��Դ��__logmgr_sync_cfg�����ý���  ��ֵ��__wt_txn_begin
	uint32_t txn_logsync;	/* Log sync configuration */

	/*
	 * Timestamp copied into updates created by this transaction.
	 *
	 * In some use cases, this can be updated while the transaction is
	 * running.
	 */
	//WT_DECL_TIMESTAMP(commit_timestamp)
/*
������Ч����__wt_txn_commit��Ӱ��ȫ��txn_global->commit_timestamp��
�Լ���__wt_txn_set_commit_timestamp��Ӱ��ȫ�ֶ���txn_global->commit_timestamph
������ΪӰ��ȫ��commit_timestamp��commit_timestamph�Ӷ�Ӱ��__wt_txn_update_pinned_timestamp->
__txn_global_query_timestamp��ʵ������ͨ��Ӱ��pinned_timestamp(__wt_txn_visible_all)��Ӱ��ɼ��Ե�
*/
	wt_timestamp_t commit_timestamp;  

	/*
	 * Set to the first commit timestamp used in the transaction and fixed
	 * while the transaction is on the public list of committed timestamps.
	 */
	//WT_DECL_TIMESTAMP(first_commit_timestamp)
	// __wt_txn_set_commit_timestamp
	//�ѱ���session����һ�β�����commit_timestamp���浽first_commit_timestamp
	wt_timestamp_t first_commit_timestamp; //��Ч�Լ���__wt_timestamp_validate

	/* Read updates committed as of this timestamp. */
	//��Ч�ο�__wt_txn_visible����ֵ��__wt_txn_config
	//WT_DECL_TIMESTAMP(read_timestamp)
	//��Ч�ж���__wt_txn_visible
	wt_timestamp_t read_timestamp;  
	TAILQ_ENTRY(__wt_txn) commit_timestampq;
	TAILQ_ENTRY(__wt_txn) read_timestampq;

	/* Array of modifications by this transaction. */
	//һ�����������������ľ�������ͨ��mod����洢
	//��__wt_txn_log_op   ��ֵ��__txn_next_op�з���WT_TXN_OP
	 //WT_TXN��__wt_txn_op��__txn_next_op�й�������
	 //������������ִ�еĲ����б�,��������ع���
	 //���ڴ��е�update�ṹ��Ϣ�����Ǵ���������Ӧ��Ա�е�
	WT_TXN_OP      *mod;  /* ��mod�����¼�˱�session��Ӧ�����������д������Ϣ*/  
	
	size_t		mod_alloc; //__txn_next_op
	u_int		mod_count; //��__txn_next_op

	/* Scratch buffer for in-memory log records. */
	//��ֵ��__txn_logrec_init    https://blog.csdn.net/yuanrxdu/article/details/78339295
	//KV�ĸ��ֲ���  ���� ɾ�����������ȡһ��op��Ȼ����__txn_op_log�а�op��ʽ��Ϊָ����ʽ���ݺ�
	//����logrec�У�Ȼ��ͨ��__wt_txn_log_commit....__wt_log_fill��op����������ݿ�����slot buffer,
	//Ȼ����__wt_txn_commit->__wt_txn_release->__wt_logrec_free���ͷŵ��ÿռ�(��������)���´��µ�OP����
	//��Ϊlogrec��ȡһ���µ�item���ظ��ù��̡�
	WT_ITEM	       *logrec;

	/* Requested notification when transactions are resolved. */
	WT_TXN_NOTIFY *notify;

	/* Checkpoint status. */
	WT_LSN		ckpt_lsn;
	uint32_t	ckpt_nsnapshot;
	WT_ITEM		*ckpt_snapshot;
	bool		full_ckpt;

    //�����WT_TXN_AUTOCOMMIT��
	uint32_t flags;
};

//�����__wt_txn.flags���õ�
/* txn flags��ֵ���� */
#define	WT_TXN_AUTOCOMMIT	0x00001
#define	WT_TXN_ERROR		0x00002
#define	WT_TXN_HAS_ID		0x00004
//��ȡ��ǰϵͳ������պ���__wt_txn_get_snapshot->__txn_sort_snapshot����λ����__wt_txn_release_snapshot��������
//__txn_sort_snapshot  __wt_txn_named_snapshot_get����λ��__wt_txn_release_snapshot�����
#define	WT_TXN_HAS_SNAPSHOT	0x00008
//__wt_txn_set_commit_timestamp����λ
#define	WT_TXN_HAS_TS_COMMIT	0x00010
/* Are we using a read timestamp for this checkpoint transaction? */
//__wt_txn_config->__wt_txn_set_read_timestamp�и�ֵ
#define	WT_TXN_HAS_TS_READ	0x00020
#define	WT_TXN_NAMED_SNAPSHOT	0x00040
//__wt_txn_set_commit_timestamp����λ
#define	WT_TXN_PUBLIC_TS_COMMIT	0x00080
#define	WT_TXN_PUBLIC_TS_READ	0x00100
#define	WT_TXN_READONLY		0x00200
#define	WT_TXN_RUNNING		0x00400
#define	WT_TXN_SYNC_SET		0x00800
#define	WT_TXN_TS_COMMIT_ALWAYS	0x01000
#define	WT_TXN_TS_COMMIT_NEVER	0x02000

