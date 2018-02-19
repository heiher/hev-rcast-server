/*
 ============================================================================
 Name        : hev-rcast-temp-session.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast temp session
 ============================================================================
 */

#ifndef __HEV_RCAST_TEMP_SESSION_H__
#define __HEV_RCAST_TEMP_SESSION_H__

#include "hev-rcast-base-session.h"

typedef struct _HevRcastTempSession HevRcastTempSession;

HevRcastTempSession * hev_rcast_temp_session_new (int fd,
			HevRcastBaseSessionCloseNotify notify, void *data);

HevRcastTempSession * hev_rcast_temp_session_ref (HevRcastTempSession *self);
void hev_rcast_temp_session_unref (HevRcastTempSession *self);

void hev_rcast_temp_session_run (HevRcastTempSession *self);

#endif /* __HEV_RCAST_TEMP_SESSION_H__ */

