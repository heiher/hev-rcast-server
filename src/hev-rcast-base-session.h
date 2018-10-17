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

#define HEV_RCAST_BASE_SESSION_HP (10)

typedef struct _HevRcastBaseSession HevRcastBaseSession;
typedef enum _HevRcastBaseSessionNotifyAction HevRcastBaseSessionNotifyAction;
typedef void (*HevRcastBaseSessionNotify) (
HevRcastBaseSession *self, HevRcastBaseSessionNotifyAction action, void *data);

enum _HevRcastBaseSessionNotifyAction
{
    HEV_RCAST_BASE_SESSION_NOTIFY_FREE = 0,
    HEV_RCAST_BASE_SESSION_NOTIFY_TO_INPUT,
    HEV_RCAST_BASE_SESSION_NOTIFY_TO_OUTPUT,
    HEV_RCAST_BASE_SESSION_NOTIFY_TO_CONTROL,
    HEV_RCAST_BASE_SESSION_NOTIFY_TO_HTTP,
    HEV_RCAST_BASE_SESSION_NOTIFY_DISPATCH,
};

struct _HevRcastBaseSession
{
    HevRcastBaseSession *prev;
    HevRcastBaseSession *next;

    HevTask *task;

    int hp;
    int fd;
};

void hev_rcast_base_session_quit (HevRcastBaseSession *self);
void hev_rcast_base_session_reset_hp (HevRcastBaseSession *self);

#endif /* __HEV_RCAST_BASE_SESSION_H__ */
