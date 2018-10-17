/*
 ============================================================================
 Name        : hev-rcast-control-session.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast control session
 ============================================================================
 */

#ifndef __HEV_RCAST_CONTROL_SESSION_H__
#define __HEV_RCAST_CONTROL_SESSION_H__

#include "hev-rcast-base-session.h"
#include "hev-rcast-buffer.h"

typedef struct _HevRcastControlSession HevRcastControlSession;

HevRcastControlSession *
hev_rcast_control_session_new (int fd, HevRcastBaseSessionNotify notify,
                               void *data);

HevRcastControlSession *
hev_rcast_control_session_ref (HevRcastControlSession *self);
void hev_rcast_control_session_unref (HevRcastControlSession *self);

void hev_rcast_control_session_run (HevRcastControlSession *self);

void hev_rcast_control_session_push_buffer (HevRcastControlSession *self,
                                            HevRcastBuffer *buffer);

#endif /* __HEV_RCAST_CONTROL_SESSION_H__ */
