/*-
 * Copyright (c) 2014-2017 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

/*
 * WT_DATA_HANDLE_CACHE --
 *	Per-session cache of handles to avoid synchronization when opening
 *	cursors.
 */
//__session_add_dhandle�����ռ�͸�ֵ
/*session��cahce*/
struct __wt_data_handle_cache {
	WT_DATA_HANDLE *dhandle;

	TAILQ_ENTRY(__wt_data_handle_cache) q;
	TAILQ_ENTRY(__wt_data_handle_cache) hashq;
};

					/* Generations manager */
#define	WT_GEN_CHECKPOINT	0	/* Checkpoint generation */
#define	WT_GEN_EVICT		1	/* Eviction generation */
#define	WT_GEN_HAZARD		2	/* Hazard pointer */
#define	WT_GEN_SPLIT		3	/* Page splits */
#define	WT_GENERATIONS		4	/* Total generation manager entries */


/*
 * WT_HAZARD --
 *	A hazard pointer.
 */
//__wt_hazard_set�и�ֵ
struct __wt_hazard {
	WT_REF *ref;			/* Page reference */
#ifdef HAVE_DIAGNOSTIC
	const char *file;		/* File/line where hazard acquired */
	int	    line;
#endif
};

/* Get the connection implementation for a session */
//����WT_CONNECTION_IMPL�ṹ     session����ΪWT_SESSION_IMPL, ifaceΪWT_SESSION iface���ͣ�connectionΪWT_CONNECTION *connection;
//WT_CONNECTION_IMPL�ĵ�һ����ԱifaceΪWT_CONNECTION�ṹ�����Կ��Խ�(session)->iface.connectionֱ��ת��ΪWT_CONNECTION_IMPL
#define	S2C(session)	  ((WT_CONNECTION_IMPL *)(session)->iface.connection)

/* Get the btree for a session */ 
//��session���������ǿ�bree����һ��tree����Ӧ�����ϵ�һ���ļ�
#define	S2BT(session)	   ((WT_BTREE *)(session)->dhandle->handle)
#define	S2BT_SAFE(session) ((session)->dhandle == NULL ? NULL : S2BT(session))

/*
 * WT_SESSION_IMPL --
 *	Implementation of WT_SESSION.
 */ 
 //�����ռ�͸�ֵ��__wt_open_internal_session      
 //__wt_connection_impl.sessions����λ������  
 //S2C���session(WT_SESSION_IMPL)��connection(__wt_connection_impl)ת��
 //��ֵ��__open_session  
struct __wt_session_impl {
	WT_SESSION iface; //__open_session�и�ֵ

	void	*lang_private;		/* Language specific private storage */

    //�Ƿ��Ѿ���ʹ��
	u_int active;			/* Non-zero if the session is in-use */

    //session->name���ӡ��������__wt_verbose  ��ֵ��__wt_open_internal_session
	const char *name;		/* Name */ //���Բο�__wt_open_internal_session
	const char *lastop;		/* Last operation */
	//��__wt_connection_impl.sessions�����е��α� 
	uint32_t id;			/* UID, offset in session array */
    //��ֵ��__wt_event_handler_set
	WT_EVENT_HANDLER *event_handler;/* Application's event handlers */
	
	//˵��session->dhandleʵ���ϴ洢���ǵ�ǰ��Ҫ�õ�dhandle�������table,��Ϊ__wt_table.iface�������file���Ϊ__wt_data_handle 
    //__session_get_dhandle(��ȡ��Ĭ���ȴ�hash�����л�ȡ��û�вŴ���)  __wt_conn_dhandle_alloc�� __wt_conn_dhandle_find��ֵ 
	WT_DATA_HANDLE *dhandle;	/* Current data handle */

	/*
	 * Each session keeps a cache of data handles. The set of handles can
	 * grow quite large so we maintain both a simple list and a hash table
	 * of lists. The hash table key is based on a hash of the data handle's
	 * URI.  The hash table list is kept in allocated memory that lives
	 * across session close - so it is declared further down.
	 */
					/* Session handle reference list */		
    //dhandles�������dhhash��ϣ�ɾ��dhandle��__session_discard_dhandle��ɾ���������__session_add_dhandle
	TAILQ_HEAD(__dhandles, __wt_data_handle_cache) dhandles;
	//��Ч��__session_dhandle_sweep
	time_t last_sweep;		/* Last sweep for dead handles */
	struct timespec last_epoch;	/* Last epoch time returned */

					/* Cursors closed with the session */
	TAILQ_HEAD(__cursors, __wt_cursor) cursors;

	WT_CURSOR_BACKUP *bkp_cursor;	/* Hot backup cursor */

	WT_COMPACT_STATE *compact;	/* Compaction information */
	enum { WT_COMPACT_NONE=0,
	    WT_COMPACT_RUNNING, WT_COMPACT_SUCCESS } compact_state;

	WT_CURSOR	*las_cursor;	/* Lookaside table cursor */

    //��ֵ��__wt_metadata_cursor
	WT_CURSOR *meta_cursor;		/* Metadata file */
	void	  *meta_track;		/* Metadata operation tracking */
	void	  *meta_track_next;	/* Current position */
	void	  *meta_track_sub;	/* Child transaction / save point */
	size_t	   meta_track_alloc;	/* Currently allocated */
	int	   meta_track_nest;	/* Nesting level of meta transaction */
#define	WT_META_TRACKING(session)	((session)->meta_track_next != NULL)

	/* Current rwlock for callback. */
	WT_RWLOCK *current_rwlock;
	uint8_t current_rwticket;

    //ָ��WT_ITEM*ָ�����飬�����СΪscratch_alloc
	WT_ITEM	**scratch;		/* Temporary memory for any function */
	u_int	  scratch_alloc;	/* Currently allocated */
	//��ʾ�����scratch�ռ��У�ʣ�µĿ����ÿռ��С������scratch������1K�ֽڿռ��С��item,���ͷŸ�item�󣬱�ʾ������scratch�ռ�Ϊ1K
	size_t	  scratch_cached;	/* Scratch bytes cached */
#ifdef HAVE_DIAGNOSTIC
	/*
	 * Variables used to look for violations of the contract that a
	 * session is only used by a single session at once.
	 */
	volatile uintmax_t api_tid;
	volatile uint32_t api_enter_refcnt;
	/*
	 * It's hard to figure out from where a buffer was allocated after it's
	 * leaked, so in diagnostic mode we track them; DIAGNOSTIC can't simply
	 * add additional fields to WT_ITEM structures because they are visible
	 * to applications, create a parallel structure instead.
	 */
	struct __wt_scratch_track {
		const char *file;	/* Allocating file, line */
		int line;
	} *scratch_track;
#endif

	WT_ITEM err;			/* Error buffer */

    //Ĭ��WT_ISO_READ_COMMITTED����__open_session
	WT_TXN_ISOLATION isolation; 
	//��ֵ��__wt_txn_init   //session->txn  conn->txn_global�Ĺ�ϵ���Բο�__wt_txn_am_oldest 
	//��ʾ��session��ǰ���е�����id
	WT_TXN	txn; 			/* Transaction state */
#define	WT_SESSION_BG_SYNC_MSEC		1200000
	WT_LSN	bg_sync_lsn;		/* Background sync operation LSN. */
	//��session��Ӧ����Чcursor��
	u_int	ncursors;		/* Count of active file cursors. */

    //��ֵ��__wt_block_ext_prealloc
	void	*block_manager;		/* Block-manager support */
	//��ֵ��__wt_block_ext_prealloc
	int	(*block_manager_cleanup)(WT_SESSION_IMPL *);

					/* Checkpoint handles */
	//��ֵ��__wt_checkpoint_get_handles
	//������������Ա����ckpt_handle_next�����Բο�__checkpoint_apply
	WT_DATA_HANDLE **ckpt_handle;	/* Handle list */ 
	u_int   ckpt_handle_next;	/* Next empty slot */
	size_t  ckpt_handle_allocated;	/* Bytes allocated */

	/*
	 * Operations acting on handles.
	 *
	 * The preferred pattern is to gather all of the required handles at
	 * the beginning of an operation, then drop any other locks, perform
	 * the operation, then release the handles.  This cannot be easily
	 * merged with the list of checkpoint handles because some operations
	 * (such as compact) do checkpoints internally.
	 */
	WT_DATA_HANDLE **op_handle;	/* Handle list */
	u_int   op_handle_next;		/* Next empty slot */
	size_t  op_handle_allocated;	/* Bytes allocated */

    //��ֵ��__wt_reconcile
	void	*reconcile;		/* Reconciliation support */
	int	(*reconcile_cleanup)(WT_SESSION_IMPL *);

	/* Sessions have an associated statistics bucket based on its ID. */
	//__open_session�и�ֵ
	u_int	stat_bucket;		/* Statistics bucket offset */

	uint32_t flags;

	/*
	 * All of the following fields live at the end of the structure so it's
	 * easier to clear everything but the fields that persist.
	 */
#define	WT_SESSION_CLEAR_SIZE	(offsetof(WT_SESSION_IMPL, rnd))

	/*
	 * The random number state persists past session close because we don't
	 * want to repeatedly use the same values for skiplist depth when the
	 * application isn't caching sessions.
	 */
	//__wt_random_init��ȡ�������ֵ��rnd
	WT_RAND_STATE rnd;		/* Random number generation state */

    //�����±�ȡֵ�ο�WT_GEN_CHECKPOINT��
    //conn->generations[]��session->s->generations[]�Ĺ�ϵ���Բο�__wt_gen_oldest��conn�������session,��˴����ܵ�
	volatile uint64_t generations[WT_GENERATIONS];

	/*
	 * Session memory persists past session close because it's accessed by
	 * threads of control other than the thread owning the session. For
	 * example, btree splits and hazard pointers can "free" memory that's
	 * still in use. In order to eventually free it, it's stashed here with
	 * with its generation number; when no thread is reading in generation,
	 * the memory can be freed for real.
	 */
	/*
    �Ự�ڴ��ڻỰ�رպ���Ȼ���ڣ����������ͷţ���Ϊ�����ɿ����̷߳��ʵģ���������ӵ�лỰ���̷߳��ʵġ����磬btree�ָ��
    Σ��ָ����ԡ��ͷš�����ʹ�õ��ڴ档��������û���߳����ڶ�ȡʱ���ڴ���������ͷš��ýṹ���ǿ��ƺ����ͷ���Щ�ڴ�
	*/
	struct __wt_session_stash {
		struct __wt_stash {
			void	*p;	/* Memory, length */
			size_t	 len;
			uint64_t gen;	/* Generation */
		} *list;
		size_t  cnt;		/* Array entries */
		size_t  alloc;		/* Allocated bytes */
	} stash[WT_GENERATIONS];

	/*
	 * Hazard pointers.
	 *
	 * Hazard information persists past session close because it's accessed
	 * by threads of control other than the thread owning the session.
	 *
	 * Use the non-NULL state of the hazard field to know if the session has
	 * previously been initialized.
	 */
#define	WT_SESSION_FIRST_USE(s)						\
	((s)->hazard == NULL)

					/* Hashed handle reference list array */
	//dhandles�������dhhash��ϣ�ɾ��dhandle��__session_discard_dhandle��ɾ���������__session_add_dhandle
	TAILQ_HEAD(__dhandles_hash, __wt_data_handle_cache) *dhhash;


    /*
    Hazard Pointer������ָ�룩
    Hazard Pointer��lock-free������һ��ʵ�ַ�ʽ�� �������ǳ��õ�����������ת��Ϊһ���ڴ�������⣬ 
    ͨ����Ҳ�ܼ��ٳ������ȴ���ʱ���Լ������ķ��գ� �����ܹ�������ܣ� �ڶ��̻߳������棬���ܺõĽ������д�ٵ����⡣ 
    ����ԭ�� 
    ����һ����Դ�� ����һ��Hazard Pointer List�� ÿ�����߳���Ҫ������Դ��ʱ�� �����������һ���ڵ㣬 
    ����������ʱ�� ɾ���ýڵ㣻 Ҫɾ������Դ��ʱ�� �жϸ������ǲ��ǿգ� �粻�� �������߳��ڶ�ȡ����Դ�� �Ͳ���ɾ���� 
    
    
    HazardPointer��WiredTiger�е�ʹ�� 
    ��WiredTiger� ����ÿһ�������ҳ�� ʹ��һ��Hazard Pointer ���������� ֮������Ҫ�����Ĺ���ʽ�� ����Ϊ�� 
    ÿ������һ������ҳ���ڴ棬 WiredTiger����������ܵķ��뻺�棬 �Ա��������ڴ���ʣ� ����ͬʱ��һЩevict �߳�
    �����У����ڴ�Խ���ʱ�� evict�߳̾ͻᰴ��LRU�㷨�� ��һЩ���������ʵ����ڴ�ҳд�ش��̡� 
    ����ÿһ���ڴ�ҳ��һ��Hazard Point�� ��evict��ʱ�� �Ϳ��Ը���Hazard Pointer List�ĳ��ȣ� �������Ƿ���Խ���
    �ڴ�ҳ�ӻ�����д�ش��̡�
    */
	/*
	 * The hazard pointer array grows as necessary, initialize with 250
	 * slots.
	 */
	//�����⼸��������__open_session�г�ʼ��
#define	WT_SESSION_INITIAL_HAZARD_SLOTS	250
	uint32_t   hazard_size;		/* Hazard pointer array slots */
	uint32_t   hazard_inuse;	/* Hazard pointer array slots in-use */
	uint32_t   nhazard;		/* Count of active hazard pointers */
	WT_HAZARD *hazard;		/* Hazard pointer array */
};


