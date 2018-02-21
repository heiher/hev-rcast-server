/*
 ============================================================================
 Name        : hev-rcast-output-session.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast output session
 ============================================================================
 */

#ifndef __HEV_RCAST_OUTPUT_SESSION_H__
#define __HEV_RCAST_OUTPUT_SESSION_H__

#include "hev-rcast-base-session.h"
#include "hev-rcast-buffer.h"

typedef struct _HevRcastOutputSession HevRcastOutputSession;

HevRcastOutputSession * hev_rcast_output_session_new (int fd,
			HevRcastBaseSessionNotify notify, void *data);

HevRcastOutputSession * hev_rcast_output_session_ref (HevRcastOutputSession *self);
void hev_rcast_output_session_unref (HevRcastOutputSession *self);

void hev_rcast_output_session_run (HevRcastOutputSession *self);

int hev_rcast_output_session_push_buffer (HevRcastOutputSession *self,
			HevRcastBuffer *buffer);

#endif /* __HEV_RCAST_OUTPUT_SESSION_H__ */

