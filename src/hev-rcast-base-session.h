/*
 ============================================================================
 Name        : hev-rcast-base-session.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast base session
 ============================================================================
 */

#ifndef __HEV_RCAST_BASE_SESSION_H__
#define __HEV_RCAST_BASE_SESSION_H__

#include <hev-task.h>

#define HEV_RCAST_BASE_SESSION_HP	(10)

typedef struct _HevRcastBaseSession HevRcastBaseSession;
typedef void (*HevRcastBaseSessionCloseNotify) (HevRcastBaseSession *self, void *data);

struct _HevRcastBaseSession
{
	HevRcastBaseSession *prev;
	HevRcastBaseSession *next;

	HevTask *task;

	int hp;
};

#endif /* __HEV_RCAST_BASE_SESSION_H__ */

