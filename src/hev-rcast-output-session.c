/*
 ============================================================================
 Name        : hev-rcast-output-session.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast output session
 ============================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "hev-rcast-output-session.h"
#include "hev-rcast-protocol.h"
#include "hev-rcast-server.h"

#include <hev-task.h>
#include <hev-task-io-socket.h>
#include <hev-memory-allocator.h>

#define BUFFERS_COUNT		(72)

struct _HevRcastOutputSession
{
	HevRcastBaseSession base;

	int ref_count;
	int skip_ref_buffer;

	HevRcastBaseSessionNotify notify;
	void *notify_data;

	unsigned int buffers_r;
	unsigned int buffers_w;
	HevRcastBuffer *buffers[BUFFERS_COUNT];
};

static void hev_rcast_task_entry (void *data);

HevRcastOutputSession *
hev_rcast_output_session_new (int fd,
			HevRcastBaseSessionNotify notify, void *data)
{
	HevRcastOutputSession *self;

	self = hev_malloc0 (sizeof (HevRcastOutputSession));
	if (!self) {
		fprintf (stderr, "Alloc HevRcastOutputSession failed!\n");
		return NULL;
	}

	self->base.task = hev_task_new (-1);
	if (!self->base.task) {
		fprintf (stderr, "Create output session task failed!\n");
		hev_free (self);
		return NULL;
	}

	self->base.hp = HEV_RCAST_BASE_SESSION_HP;
	self->base.fd = fd;

	self->ref_count = 1;
	self->skip_ref_buffer = 1;
	self->notify = notify;
	self->notify_data = data;

	return self;
}

HevRcastOutputSession *
hev_rcast_output_session_ref (HevRcastOutputSession *self)
{
	self->ref_count ++;

	return self;
}

void
hev_rcast_output_session_unref (HevRcastOutputSession *self)
{
	self->ref_count --;
	if (self->ref_count)
		return;

	close (self->base.fd);

	for (; self->buffers_r != self->buffers_w;) {
		hev_rcast_buffer_unref (self->buffers[self->buffers_r]);
		self->buffers_r = (self->buffers_r + 1) % BUFFERS_COUNT;
	}

	hev_free (self);
}

void
hev_rcast_output_session_run (HevRcastOutputSession *self)
{
	hev_task_run (self->base.task, hev_rcast_task_entry, self);
}

int
hev_rcast_output_session_push_buffer (HevRcastOutputSession *self,
			HevRcastBuffer *buffer)
{
	int retval = 0;
	unsigned int next_w;

	next_w = (self->buffers_w + 1) % BUFFERS_COUNT;
	if (self->buffers_r == next_w) {
		retval = 1;
		self->skip_ref_buffer = 1;

		for (;;) {
			if (self->buffers_r == self->buffers_w)
				break;

			hev_rcast_buffer_unref (self->buffers[self->buffers_r]);
			self->buffers_r = (self->buffers_r + 1) % BUFFERS_COUNT;
		}
	}

	if (self->skip_ref_buffer) {
		unsigned char type;

		type = hev_rcast_buffer_get_type (buffer);
		switch (type) {
		case HEV_RCAST_MESSAGE_REF_FRAME:
			hev_rcast_buffer_unref (buffer);
			return retval;
		case HEV_RCAST_MESSAGE_KEY_FRAME:
			self->skip_ref_buffer = 0;
		}
	}

	self->buffers[self->buffers_w] = buffer;
	self->buffers_w = next_w;

	hev_task_wakeup (self->base.task);

	return retval;
}

static int
task_io_yielder (HevTaskYieldType type, void *data)
{
	HevRcastOutputSession *self = data;

	hev_task_yield (type);

	return (self->base.hp > 0) ? 0 : -1;
}

static void
hev_rcast_task_entry (void *data)
{
	HevRcastOutputSession *self = data;
	HevTask *task = hev_task_self ();
	HevRcastMessage msg;
	size_t msg_len;
	ssize_t len;
	HevRcastBaseSessionNotifyAction action;

	hev_task_add_fd (task, self->base.fd, EPOLLOUT);

	for (;;) {
		HevRcastBuffer *buffer;
		size_t data_len;
		void *data;

		if (self->buffers_r == self->buffers_w) {
			hev_task_yield (HEV_TASK_WAITIO);
			if (self->base.hp == 0)
				break;
			continue;
		}

		buffer = self->buffers[self->buffers_r];
		self->buffers_r = (self->buffers_r + 1) % BUFFERS_COUNT;

		data = hev_rcast_buffer_get_data (buffer);
		data_len = hev_rcast_buffer_get_data_length (buffer);

		msg.type = HEV_RCAST_MESSAGE_FRAME;
		msg.frame.type = hev_rcast_buffer_get_type (buffer);
		msg.frame.length = htonl (data_len);
		msg_len = sizeof (msg.type) + sizeof (HevRcastMessageFrame);
		len = hev_task_io_socket_send (self->base.fd, &msg, msg_len, MSG_WAITALL,
					task_io_yielder, self);
		if (len != msg_len) {
			goto notify;
		}

		len = hev_task_io_socket_send (self->base.fd, data, data_len, MSG_WAITALL,
					task_io_yielder, self);
		if (len != data_len) {
			goto notify;
		}

		hev_rcast_buffer_unref (buffer);

		self->base.hp = HEV_RCAST_BASE_SESSION_HP;
	}

notify:
	action = HEV_RCAST_BASE_SESSION_NOTIFY_FREE;
	self->notify ((HevRcastBaseSession *) self, action, self->notify_data);
}

