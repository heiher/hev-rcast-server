/*
 ============================================================================
 Name        : hev-rcast-http-session.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast http session
 ============================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "hev-rcast-http-session.h"
#include "hev-rcast-protocol.h"
#include "hev-rcast-server.h"

#include <hev-task.h>
#include <hev-task-io-socket.h>
#include <hev-memory-allocator.h>

struct _HevRcastHttpSession
{
	HevRcastBaseSession base;

	int ref_count;

	HevRcastBaseSessionNotify notify;
	void *notify_data;

	HevRcastBuffer *buffer;
};

static void hev_rcast_task_entry (void *data);

HevRcastHttpSession *
hev_rcast_http_session_new (int fd,
			HevRcastBaseSessionNotify notify, void *data)
{
	HevRcastHttpSession *self;

	self = hev_malloc0 (sizeof (HevRcastHttpSession));
	if (!self) {
		fprintf (stderr, "Alloc HevRcastHttpSession failed!\n");
		return NULL;
	}

	self->base.task = hev_task_new (-1);
	if (!self->base.task) {
		fprintf (stderr, "Create http session task failed!\n");
		hev_free (self);
		return NULL;
	}

	self->base.hp = HEV_RCAST_BASE_SESSION_HP;
	self->base.fd = fd;

	self->ref_count = 1;
	self->notify = notify;
	self->notify_data = data;

	return self;
}

HevRcastHttpSession *
hev_rcast_http_session_ref (HevRcastHttpSession *self)
{
	self->ref_count ++;

	return self;
}

void
hev_rcast_http_session_unref (HevRcastHttpSession *self)
{
	self->ref_count --;
	if (self->ref_count)
		return;

	close (self->base.fd);

	if (self->buffer)
		hev_rcast_buffer_unref (self->buffer);

	hev_free (self);
}

void
hev_rcast_http_session_run (HevRcastHttpSession *self)
{
	hev_task_run (self->base.task, hev_rcast_task_entry, self);
}

HevRcastBuffer *
hev_rcast_http_session_get_buffer (HevRcastHttpSession *self)
{
	HevRcastBuffer *buffer;

	buffer = self->buffer;
	self->buffer = NULL;

	return buffer;
}

static void
hev_rcast_task_entry (void *data)
{
	HevRcastHttpSession *self = data;
	HevTask *task = hev_task_self ();
	HevRcastBaseSessionNotifyAction action;
	HevRcastBuffer *buffer;

	hev_task_add_fd (task, self->base.fd, EPOLLIN | EPOLLOUT);

retry_alloc:
	buffer = hev_rcast_buffer_new (0, 0);
	if (!buffer) {
		hev_task_sleep (100);
		goto retry_alloc;
	}

	/* dispatch frames */
	if (self->buffer)
		hev_rcast_buffer_unref (self->buffer);
	self->buffer = buffer;

	action = HEV_RCAST_BASE_SESSION_NOTIFY_DISPATCH;
	self->notify ((HevRcastBaseSession *) self, action, self->notify_data);

	action = HEV_RCAST_BASE_SESSION_NOTIFY_FREE;
	self->notify ((HevRcastBaseSession *) self, action, self->notify_data);
}

