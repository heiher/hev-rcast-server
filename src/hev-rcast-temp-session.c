/*
 ============================================================================
 Name        : hev-rcast-temp-session.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast temp session
 ============================================================================
 */

#include <stdio.h>

#include "hev-rcast-temp-session.h"

#include <hev-task.h>
#include <hev-memory-allocator.h>

struct _HevRcastTempSession
{
	HevRcastBaseSession base;

	int fd;
	int ref_count;

	HevRcastBaseSessionCloseNotify notify;
	void *notify_data;
};

static void hev_rcast_task_entry (void *data);

HevRcastTempSession *
hev_rcast_temp_session_new (int fd,
			HevRcastBaseSessionCloseNotify notify, void *data)
{
	HevRcastTempSession *self;

	self = hev_malloc0 (sizeof (HevRcastTempSession));
	if (!self) {
		fprintf (stderr, "Alloc HevRcastTempSession failed!\n");
		return NULL;
	}

	self->base.task = hev_task_new (-1);
	if (!self->base.task) {
		fprintf (stderr, "Create temp session task failed!\n");
		hev_free (self);
		return NULL;
	}

	self->base.hp = HEV_RCAST_BASE_SESSION_HP;

	self->fd = fd;
	self->ref_count = 1;
	self->notify = notify;
	self->notify_data = data;

	return self;
}

HevRcastTempSession *
hev_rcast_temp_session_ref (HevRcastTempSession *self)
{
	self->ref_count ++;

	return self;
}

void
hev_rcast_temp_session_unref (HevRcastTempSession *self)
{
	self->ref_count --;
	if (self->ref_count)
		return;

	hev_free (self);
}

void
hev_rcast_temp_session_run (HevRcastTempSession *self)
{
	hev_task_run (self->base.task, hev_rcast_task_entry, self);
}

static void
hev_rcast_task_entry (void *data)
{
}

