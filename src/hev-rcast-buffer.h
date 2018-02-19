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

struct _HevRcastBuffer
{
	int ref_count;

	unsigned int length;
	unsigned char type;

	unsigned char data[0];
};

#endif /* __HEV_RCAST_BUFFER_H__ */

