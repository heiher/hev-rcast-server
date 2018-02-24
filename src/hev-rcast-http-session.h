/*
 ============================================================================
 Name        : hev-rcast-http-session.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast http session
 ============================================================================
 */

#ifndef __HEV_RCAST_HTTP_SESSION_H__
#define __HEV_RCAST_HTTP_SESSION_H__

#include "hev-rcast-base-session.h"
#include "hev-rcast-buffer.h"

typedef struct _HevRcastHttpSession HevRcastHttpSession;

HevRcastHttpSession * hev_rcast_http_session_new (int fd,
			HevRcastBaseSessionNotify notify, void *data);

HevRcastHttpSession * hev_rcast_http_session_ref (HevRcastHttpSession *self);
void hev_rcast_http_session_unref (HevRcastHttpSession *self);

void hev_rcast_http_session_run (HevRcastHttpSession *self);

HevRcastBuffer * hev_rcast_http_session_get_buffer (HevRcastHttpSession *self);

#endif /* __HEV_RCAST_HTTP_SESSION_H__ */

