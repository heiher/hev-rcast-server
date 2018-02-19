/*
 ============================================================================
 Name        : hev-rcast-buffer.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast buffer
 ============================================================================
 */

#ifndef __HEV_RCAST_BUFFER_H__
#define __HEV_RCAST_BUFFER_H__

typedef struct _HevRcastBuffer HevRcastBuffer;

HevRcastBuffer * hev_rcast_buffer_new (unsigned char type, unsigned int length);
HevRcastBuffer * hev_rcast_buffer_ref (HevRcastBuffer *self);
void hev_rcast_buffer_unref (HevRcastBuffer *self);

void * hev_rcast_buffer_get_data (HevRcastBuffer *self);
unsigned int hev_rcast_buffer_get_data_length (HevRcastBuffer *self);
unsigned char hev_rcast_buffer_get_type (HevRcastBuffer *self);

#endif /* __HEV_RCAST_BUFFER_H__ */

