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
typedef enum _HevRcastBaseSessionCloseNotifyAction HevRcastBaseSessionCloseNotifyAction;
typedef void (*HevRcastBaseSessionCloseNotify) (HevRcastBaseSession *self,
			HevRcastBaseSessionCloseNotifyAction action, void *data);

enum _HevRcastBaseSessionCloseNotifyAction
{
	HEV_RCAST_BASE_SESSION_CLOSE_NOTIFY_FREE = 0,
	HEV_RCAST_BASE_SESSION_CLOSE_NOTIFY_TO_INPUT,
	HEV_RCAST_BASE_SESSION_CLOSE_NOTIFY_TO_OUTPUT,
};

struct _HevRcastBaseSession
{
	HevRcastBaseSession *prev;
	HevRcastBaseSession *next;

	HevTask *task;

	int hp;
	int fd;
};

#endif /* __HEV_RCAST_BASE_SESSION_H__ */

