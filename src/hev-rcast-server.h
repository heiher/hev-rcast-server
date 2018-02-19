/*
 ============================================================================
 Name        : hev-rcast-server.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast server
 ============================================================================
 */

#ifndef __HEV_RCAST_SERVER_H__
#define __HEV_RCAST_SERVER_H__

typedef struct _HevRcastServer HevRcastServer;

HevRcastServer * hev_rcast_server_new (void);
void hev_rcast_server_destroy (HevRcastServer *self);

void hev_rcast_server_run (HevRcastServer *self);
void hev_rcast_server_quit (HevRcastServer *self);

#endif /* __HEV_RCAST_SERVER_H__ */

