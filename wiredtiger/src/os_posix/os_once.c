/*-
 * Copyright (c) 2014-2017 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

/*
 * __wt_once --
 *	One-time initialization per process.
 */
int
__wt_once(void (*init_routine)(void))
{
    //������ʹ�ó�ֵΪPTHREAD_ONCE_INIT��once_control������֤init_routine()�����ڱ�����ִ�������н�ִ��һ�Ρ�
	static pthread_once_t once_control = PTHREAD_ONCE_INIT;

	return (pthread_once(&once_control, init_routine));
}
