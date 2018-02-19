/*
 ============================================================================
 Name        : hev-rcast-input-session.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast input session
 ============================================================================
 */

#ifndef __HEV_RCAST_INPUT_SESSION_H__
#define __HEV_RCAST_INPUT_SESSION_H__

#include "hev-rcast-base-session.h"
#include "hev-rcast-buffer.h"

typedef struct _HevRcastInputSession HevRcastInputSession;

HevRcastInputSession * hev_rcast_input_session_new (int fd,
			HevRcastBaseSessionNotify notify, void *data);

HevRcastInputSession * hev_rcast_input_session_ref (HevRcastInputSession *self);
void hev_rcast_input_session_unref (HevRcastInputSession *self);

void hev_rcast_input_session_run (HevRcastInputSession *self);

HevRcastBuffer * hev_rcast_input_session_get_buffer (HevRcastInputSession *self, int cfg);

#endif /* __HEV_RCAST_INPUT_SESSION_H__ */

