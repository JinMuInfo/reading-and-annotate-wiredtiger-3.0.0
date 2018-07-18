/*-
 * Copyright (c) 2014-2017 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

/*
WiredTiger.basecfg�洢����������Ϣ
WiredTiger.lock���ڷ�ֹ�����������ͬһ��Wiredtiger���ݿ�
table*.wt�洢����tale�����ݿ��еı�������
WiredTiger.wt�������table�����ڴ洢��������table��Ԫ������Ϣ
WiredTiger.turtle�洢WiredTiger.wt��Ԫ������Ϣ
journal�洢Write ahead log
*/

//��¼�汾��Ϣ�����ݼ�__conn_single
#define	WT_WIREDTIGER		"WiredTiger"		/* Version file */
//�ļ�������ʼ��ֵ��__conn_single
#define	WT_SINGLETHREAD		"WiredTiger.lock"	/* Locking file */

//д��������__conn_write_base_config�����config_baseʹ�ܣ�����ȡ�����ļ�ʹ�ü�__conn_config_file
#define	WT_BASECONFIG		"WiredTiger.basecfg"	/* Base configuration */
#define	WT_BASECONFIG_SET	"WiredTiger.basecfg.set"/* Base config temp */

//���Ŀ¼���и��ļ�������ȡ���ļ�������Ϊ�����ļ� ��__conn_config_file
#define	WT_USERCONFIG		"WiredTiger.config"	/* User configuration */

//�����͸�ֵ��__backup_start  backup���
#define	WT_BACKUP_TMP		"WiredTiger.backup.tmp"	/* Backup tmp file */
#define	WT_METADATA_BACKUP	"WiredTiger.backup"	/* Hot backup file */
#define	WT_INCREMENTAL_BACKUP	"WiredTiger.ibackup"	/* Incremental backup */
#define	WT_INCREMENTAL_SRC	"WiredTiger.isrc"	/* Incremental source */

//����Ĭ����__wt_turtle_read�й��죬�����ļ���д��������__wt_turtle_update
#define	WT_METADATA_TURTLE	"WiredTiger.turtle"	/* Metadata metadata */
//turtle���µ�ʱ����ʱ�ļ�����__wt_turtle_update
#define	WT_METADATA_TURTLE_SET	"WiredTiger.turtle.set"	/* Turtle temp file */

#define	WT_METADATA_URI		"metadata:"		/* Metadata alias */
//���ļ���__create_file�д���
#define	WT_METAFILE		"WiredTiger.wt"		/* Metadata table */
#define	WT_METAFILE_URI		"file:WiredTiger.wt"	/* Metadata table URI */

//__wt_las_create�д���
#define	WT_LAS_URI		"file:WiredTigerLAS.wt"	/* Lookaside table URI*/

/*
 * Optimize comparisons against the metafile URI, flag handles that reference
 * the metadata file.
 */
#define	WT_IS_METADATA(dh)      F_ISSET((dh), WT_DHANDLE_IS_METADATA)
#define	WT_METAFILE_ID		0			/* Metadata file ID */

#define	WT_METADATA_VERSION	"WiredTiger version"	/* Version keys */
#define	WT_METADATA_VERSION_STR	"WiredTiger version string"

/*
 * WT_WITH_TURTLE_LOCK --
 *	Acquire the turtle file lock, perform an operation, drop the lock.
 */
#define	WT_WITH_TURTLE_LOCK(session, op) do {				\
	WT_ASSERT(session, !F_ISSET(session, WT_SESSION_LOCKED_TURTLE));\
	WT_WITH_LOCK_WAIT(session,					\
	    &S2C(session)->turtle_lock, WT_SESSION_LOCKED_TURTLE, op);	\
} while (0)

/*
 * WT_CKPT --
 *	Encapsulation of checkpoint information, shared by the metadata, the
 * btree engine, and the block manager.
 */
#define	WT_CHECKPOINT		"WiredTigerCheckpoint"
#define	WT_CKPT_FOREACH(ckptbase, ckpt)					\
	for ((ckpt) = (ckptbase); (ckpt)->name != NULL; ++(ckpt))

/*checkpoint��Ϣ�ṹָ��, __ckpt_load�л�ȡ���õ�checkPoint��Ϣ���ýṹ */
struct __wt_ckpt {
	char	*name;				/* Name or NULL */

	WT_ITEM  addr;				/* Checkpoint cookie string */
	WT_ITEM  raw;				/* Checkpoint cookie raw */

	int64_t	 order;				/* Checkpoint order */

	uintmax_t sec;				/* Timestamp */

	uint64_t ckpt_size;			/* Checkpoint size */

	uint64_t write_gen;			/* Write generation */

	void	*bpriv;				/* Block manager private */

#define	WT_CKPT_ADD	0x01			/* Checkpoint to be added */
#define	WT_CKPT_DELETE	0x02			/* Checkpoint to be deleted */
#define	WT_CKPT_FAKE	0x04			/* Checkpoint is a fake */
#define	WT_CKPT_UPDATE	0x08			/* Checkpoint requires update */
	uint32_t flags;
};
