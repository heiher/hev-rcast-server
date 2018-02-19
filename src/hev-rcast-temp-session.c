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
#include "hev-rcast-protocol.h"
#include "hev-rcast-server.h"

#include <hev-task.h>
#include <hev-task-io-socket.h>
#include <hev-memory-allocator.h>

struct _HevRcastTempSession
{
	HevRcastBaseSession base;

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
	self->base.fd = fd;

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

static int
task_io_yielder (HevTaskYieldType type, void *data)
{
	HevRcastTempSession *self = data;

	self->base.hp = HEV_RCAST_BASE_SESSION_HP;

	hev_task_yield (type);

	return (self->base.hp > 0) ? 0 : -1;
}

static void
hev_rcast_task_entry (void *data)
{
	HevRcastTempSession *self = data;
	HevTask *task = hev_task_self ();
	HevRcastMessage msg;
	size_t msg_len;
	ssize_t len;
	HevRcastBaseSessionCloseNotifyAction action;

	action = HEV_RCAST_BASE_SESSION_CLOSE_NOTIFY_FREE;

	hev_task_add_fd (task, self->base.fd, EPOLLIN);

	msg_len = sizeof (msg.type) + sizeof (HevRcastMessageLogin);
	len = hev_task_io_socket_recv (self->base.fd, &msg, msg_len, MSG_WAITALL,
				task_io_yielder, self);
	if (len != msg_len || HEV_RCAST_MESSAGE_LOGIN != msg.type) {
		goto notify;
	}

	hev_task_del_fd (task, self->base.fd);

	switch (msg.login.direction) {
	case HEV_RCAST_MESSAGE_LOGIN_INPUT:
		action = HEV_RCAST_BASE_SESSION_CLOSE_NOTIFY_TO_INPUT;
		break;
	case HEV_RCAST_MESSAGE_LOGIN_OUTPUT:
		action = HEV_RCAST_BASE_SESSION_CLOSE_NOTIFY_TO_OUTPUT;
		break;
	default:
		break;
	}

notify:
	self->notify ((HevRcastBaseSession *) self, action, self->notify_data);
}

