/*
 ============================================================================
 Name        : hev-rcast-buffer.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast buffer
 ============================================================================
 */

#include "hev-rcast-buffer.h"

#include <hev-memory-allocator.h>

struct _HevRcastBuffer
{
	int ref_count;

	unsigned int length;
	unsigned char type;

	unsigned char data[0];
};

HevRcastBuffer *
hev_rcast_buffer_new (unsigned char type, unsigned int length)
{
	HevRcastBuffer *self;

	self = hev_malloc (sizeof (HevRcastBuffer) + length);
	if (!self)
		return NULL;

	self->type = type;
	self->length = length;

	return self;
}

HevRcastBuffer *
hev_rcast_buffer_ref (HevRcastBuffer *self)
{
	self->ref_count ++;

	return self;
}

void
hev_rcast_buffer_unref (HevRcastBuffer *self)
{
	self->ref_count --;
	if (self->ref_count)
		return;

	hev_free (self);
}

void *
hev_rcast_buffer_get_data (HevRcastBuffer *self)
{
	return self->data;
}

unsigned int
hev_rcast_buffer_get_data_length (HevRcastBuffer *self)
{
	return self->length;
}

unsigned char
hev_rcast_buffer_get_type (HevRcastBuffer *self)
{
	return self->type;
}

