/*-
 * Copyright (c) 2014-2017 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

/*
 * Condition variables:
 *
 * WiredTiger uses condition variables to signal between threads, and for
 * locking operations that are expected to block.
 */
struct __wt_condvar {
	const char *name;		/* Mutex name for debugging */

	wt_mutex_t mtx;			/* Mutex */
	wt_cond_t  cond;		/* Condition variable */

	int waiters;			/* Numbers of waiters, or
					   -1 if signalled with no waiters. */
	/*
	 * The following fields are used for automatically adjusting condition
	 * variable wait times.
	 */
	uint64_t	min_wait;	/* Minimum wait duration */
	uint64_t	max_wait;	/* Maximum wait duration */
	uint64_t	prev_wait;	/* Wait duration used last time */
};

/*
 * Read/write locks:
 *
 * WiredTiger uses read/write locks for shared/exclusive access to resources.
 * !!!
 * Don't modify this structure without understanding the read/write locking
 * functions.
 */
//不同功能模块会有不同的锁，都会包含一个WT_RWLOCK成员，可以搜索WT_RWLOCK
struct __wt_rwlock {			/* Read/write lock */
	volatile union {
		uint64_t v;			/* Full 64-bit value */
		struct {
			uint8_t current;	/* Current ticket */
			uint8_t next;		/* Next available ticket */
			uint8_t reader;		/* Read queue ticket */
			uint8_t __notused;	/* Padding */
			uint16_t readers_active;/* Count of active readers */
			uint16_t readers_queued;/* Count of queued readers */
		} s;
	} u;

    //初始赋值见WT_RWLOCK_INIT_TRACKED
	int16_t stat_read_count_off;	/* read acquisitions offset */
	int16_t stat_write_count_off;	/* write acquisitions offset */
	int16_t stat_app_usecs_off;	/* waiting application threads offset */
	int16_t stat_int_usecs_off;	/* waiting server threads offset */

	WT_CONDVAR *cond_readers;	/* Blocking readers */
	WT_CONDVAR *cond_writers;	/* Blocking writers */
};

/*
 * WT_RWLOCK_INIT_TRACKED --
 *	Read write lock initialization, with tracking.
 *
 * Implemented as a macro so we can pass in a statistics field and convert
 * it into a statistics structure array offset.
 */ //对应的锁信息赋值给了__wt_rwlock相关成员   各种不同__wt_rwlock锁真正的赋值在__wt_rwlock_init  __wt_try_readlock
#define	WT_RWLOCK_INIT_TRACKED(session, l, name) do {                   \
	WT_RET(__wt_rwlock_init(session, l));                           \
	(l)->stat_read_count_off = (int16_t)WT_STATS_FIELD_TO_OFFSET(   \
	    S2C(session)->stats, lock_##name##_read_count);             \
	(l)->stat_write_count_off = (int16_t)WT_STATS_FIELD_TO_OFFSET(  \
	    S2C(session)->stats, lock_##name##_write_count);            \
	(l)->stat_app_usecs_off = (int16_t)WT_STATS_FIELD_TO_OFFSET(    \
	    S2C(session)->stats, lock_##name##_wait_application);       \
	(l)->stat_int_usecs_off = (int16_t)WT_STATS_FIELD_TO_OFFSET(    \
	    S2C(session)->stats, lock_##name##_wait_internal);          \
} while (0)

/*
 * Spin locks:
 *
 * WiredTiger uses spinlocks for fast mutual exclusion (where operations done
 * while holding the spin lock are expected to complete in a small number of
 * instructions).
 */
#define	SPINLOCK_GCC			0
#define	SPINLOCK_MSVC			1
#define	SPINLOCK_PTHREAD_MUTEX		2
#define	SPINLOCK_PTHREAD_MUTEX_ADAPTIVE	3

struct __wt_spinlock {
#if SPINLOCK_TYPE == SPINLOCK_GCC
	WT_CACHE_LINE_PAD_BEGIN
	volatile int lock;
#elif SPINLOCK_TYPE == SPINLOCK_PTHREAD_MUTEX ||			\
	SPINLOCK_TYPE == SPINLOCK_PTHREAD_MUTEX_ADAPTIVE ||		\
	SPINLOCK_TYPE == SPINLOCK_MSVC
	wt_mutex_t lock;
#else
#error Unknown spinlock type
#endif

	const char *name;		/* Mutex name */

	/*
	 * We track acquisitions and time spent waiting for some locks. For
	 * performance reasons and to make it possible to write generic code
	 * that tracks statistics for different locks, we store the offset
	 * of the statistics fields to be updated during lock acquisition.
	 */
	int16_t stat_count_off;		/* acquisitions offset */
	int16_t stat_app_usecs_off;	/* waiting application threads offset */
	int16_t stat_int_usecs_off;	/* waiting server threads offset */

	int8_t initialized;		/* Lock initialized, for cleanup */

#if SPINLOCK_TYPE == SPINLOCK_GCC
	WT_CACHE_LINE_PAD_END
#endif
};
