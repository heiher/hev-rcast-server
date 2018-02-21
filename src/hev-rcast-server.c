/*
 ============================================================================
 Name        : hev-rcast-server.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast server
 ============================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <hev-task.h>
#include <hev-task-io-socket.h>
#include <hev-memory-allocator.h>

#include "hev-rcast-server.h"
#include "hev-rcast-protocol.h"
#include "hev-rcast-base-session.h"
#include "hev-rcast-temp-session.h"
#include "hev-rcast-input-session.h"
#include "hev-rcast-output-session.h"
#include "hev-config.h"

#define TIMEOUT		(30 * 1000)

struct _HevRcastServer
{
	HevTask *task_listen;
	HevTask *task_rsync_manager;
	HevTask *task_session_manager;

	int fd;
	int quit;
	int rsync;

	HevRcastBaseSession *input_session;
	HevRcastBaseSession *output_sessions;
	HevRcastBaseSession *temp_sessions;
};

static void hev_rcast_task_listen_entry (void *data);
static void hev_rcast_task_rsync_manager_entry (void *data);
static void hev_rcast_task_session_manager_entry (void *data);
static void hev_rcast_dispatch_buffer (HevRcastServer *self);
static void rsync_manager_request_rsync (HevRcastServer *self);
static void session_manager_insert_session (HevRcastBaseSession **list,
			HevRcastBaseSession *session);
static void session_manager_remove_session (HevRcastBaseSession **list,
			HevRcastBaseSession *session);
static void temp_session_notify_handler (HevRcastBaseSession *session,
			HevRcastBaseSessionNotifyAction action, void *data);
static void input_session_notify_handler (HevRcastBaseSession *session,
			HevRcastBaseSessionNotifyAction action, void *data);
static void output_session_notify_handler (HevRcastBaseSession *session,
			HevRcastBaseSessionNotifyAction action, void *data);

HevRcastServer *
hev_rcast_server_new (void)
{
	HevRcastServer *self;
	struct sockaddr_in addr;
	int ret, nonblock = 1, reuseaddr = 1;

	self = hev_malloc0 (sizeof (HevRcastServer));
	if (!self) {
		fprintf (stderr, "Alloc HevRcastServer failed!\n");
		return NULL;
	}

	self->fd = socket (AF_INET, SOCK_STREAM, 0);
	if (self->fd == -1) {
		fprintf (stderr, "Create socket failed!\n");
		hev_free (self);
		return NULL;
	}

	ret = setsockopt (self->fd, SOL_SOCKET, SO_REUSEADDR,
				&reuseaddr, sizeof (reuseaddr));
	if (ret == -1) {
		fprintf (stderr, "Set reuse address failed!\n");
		close (self->fd);
		hev_free (self);
		return NULL;
	}
	ret = ioctl (self->fd, FIONBIO, (char *) &nonblock);
	if (ret == -1) {
		fprintf (stderr, "Set non-blocking failed!\n");
		close (self->fd);
		hev_free (self);
		return NULL;
	}

	memset (&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr (hev_config_get_listen_address ());
	addr.sin_port = htons (hev_config_get_port ());
	ret = bind (self->fd, (struct sockaddr *) &addr, (socklen_t) sizeof (addr));
	if (ret == -1) {
		fprintf (stderr, "Bind address failed!\n");
		close (self->fd);
		hev_free (self);
		return NULL;
	}
	ret = listen (self->fd, 10);
	if (ret == -1) {
		fprintf (stderr, "Listen failed!\n");
		close (self->fd);
		hev_free (self);
		return NULL;
	}

	self->task_listen = hev_task_new (-1);
	if (!self->task_listen) {
		fprintf (stderr, "Create task listen failed!\n");
		close (self->fd);
		hev_free (self);
		return NULL;
	}

	self->task_rsync_manager = hev_task_new (-1);
	if (!self->task_rsync_manager) {
		fprintf (stderr, "Create task rsync manager failed!\n");
		hev_task_unref (self->task_listen);
		close (self->fd);
		hev_free (self);
		return NULL;
	}

	self->task_session_manager = hev_task_new (-1);
	if (!self->task_session_manager) {
		fprintf (stderr, "Create task session manager failed!\n");
		hev_task_unref (self->task_rsync_manager);
		hev_task_unref (self->task_listen);
		close (self->fd);
		hev_free (self);
		return NULL;
	}

	hev_task_ref (self->task_listen);
	hev_task_ref (self->task_rsync_manager);
	hev_task_ref (self->task_session_manager);

	return self;
}

void hev_rcast_server_destroy (HevRcastServer *self)
{
	hev_task_unref (self->task_listen);
	hev_task_unref (self->task_rsync_manager);
	hev_task_unref (self->task_session_manager);

	close (self->fd);
	hev_free (self);
}

void
hev_rcast_server_run (HevRcastServer *self)
{
	hev_task_run (self->task_listen, hev_rcast_task_listen_entry, self);
	hev_task_run (self->task_rsync_manager, hev_rcast_task_rsync_manager_entry, self);
	hev_task_run (self->task_session_manager, hev_rcast_task_session_manager_entry, self);
}

void
hev_rcast_server_quit (HevRcastServer *self)
{
	HevRcastBaseSession *session;

	self->quit = 1;

	hev_task_wakeup (self->task_listen);
	hev_task_wakeup (self->task_rsync_manager);
	hev_task_wakeup (self->task_session_manager);

	/* input session */
	if (self->input_session)
		hev_rcast_base_session_quit (self->input_session);

	/* temp sessions */
	for (session=self->temp_sessions; session; session=session->next)
		hev_rcast_base_session_quit (session);

	/* output sessions */
	for (session=self->output_sessions; session; session=session->next)
		hev_rcast_base_session_quit (session);
}

static int
task_listen_io_yielder (HevTaskYieldType type, void *data)
{
	HevRcastServer *self = data;

	hev_task_yield (type);

	return (self->quit) ? -1 : 0;
}

static void
hev_rcast_task_listen_entry (void *data)
{
	HevRcastServer *self = data;
	HevTask *task = hev_task_self ();

	hev_task_add_fd (task, self->fd, EPOLLIN);

	for (;;) {
		int fd, ret, nonblock = 1;
		struct sockaddr_in addr;
		struct sockaddr *in_addr = (struct sockaddr *) &addr;
		socklen_t addr_len = sizeof (addr);
		HevRcastTempSession *session;

		fd = hev_task_io_socket_accept (self->fd, in_addr, &addr_len,
					task_listen_io_yielder, self);
		if (-1 == fd) {
			fprintf (stderr, "Accept failed!\n");
			continue;
		} else if (-2 == fd) {
			break;
		}

#ifdef _DEBUG
		printf ("Worker %p: New client %d enter from %s:%u\n", self, fd,
					inet_ntoa (addr.sin_addr), ntohs (addr.sin_port));
#endif

		ret = ioctl (fd, FIONBIO, (char *) &nonblock);
		if (ret == -1) {
			fprintf (stderr, "Set non-blocking failed!\n");
			close (fd);
		}

		session = hev_rcast_temp_session_new (fd, temp_session_notify_handler, self);
		if (!session) {
			close (fd);
			continue;
		}

		session_manager_insert_session (&self->temp_sessions, (HevRcastBaseSession *) session);
		hev_rcast_temp_session_run (session);
	}
}

static void
hev_rcast_task_rsync_manager_entry (void *data)
{
	HevRcastServer *self = data;

	for (; !self->quit;) {
		unsigned int delay = hev_config_get_rcast_rsync_interval ();
		HevRcastInputSession *is;

		for (; !self->rsync;) {
			hev_task_yield (HEV_TASK_WAITIO);
			if (self->quit)
				return;
		}

		do {
			delay = hev_task_sleep (delay);
		} while (!self->quit && delay);

retry:
		if (!self->rsync)
			continue;

		for (; !self->input_session;) {
			hev_task_yield (HEV_TASK_WAITIO);
			if (self->quit)
				return;
		}

		is = (HevRcastInputSession *) self->input_session;
		if (hev_rcast_input_session_rsync (is) < 0)
			goto retry;

		self->rsync = 0;
	}
}

static void
hev_rcast_task_session_manager_entry (void *data)
{
	HevRcastServer *self = data;

	for (;;) {
		HevRcastBaseSession *session;

		hev_task_sleep (TIMEOUT);
		if (self->quit)
			break;

		/* input session */
		if (self->input_session) {
			self->input_session->hp --;
			if (self->input_session->hp == 0)
				hev_rcast_base_session_quit (self->input_session);
		}
		hev_task_yield (HEV_TASK_YIELD);

		/* temp sessions */
#ifdef _DEBUG
		printf ("Enumerating temp session list ...\n");
#endif
		for (session=self->temp_sessions; session; session=session->next) {
#ifdef _DEBUG
			printf ("Session %p's hp %d\n", session, session->hp);
#endif
			session->hp --;
			if (session->hp > 0)
				continue;

			hev_rcast_base_session_quit (session);
		}
		hev_task_yield (HEV_TASK_YIELD);

		/* output sessions */
#ifdef _DEBUG
		printf ("Enumerating output session list ...\n");
#endif
		for (session=self->output_sessions; session; session=session->next) {
#ifdef _DEBUG
			printf ("Session %p's hp %d\n", session, session->hp);
#endif
			session->hp --;
			if (session->hp > 0)
				continue;

			hev_rcast_base_session_quit (session);
		}
	}
}

static void
hev_rcast_dispatch_buffer (HevRcastServer *self)
{
	HevRcastInputSession *input_session;
	HevRcastBuffer *buffer;
	HevRcastBaseSession *session;

	input_session = (HevRcastInputSession *) self->input_session;
	buffer = hev_rcast_input_session_get_buffer (input_session, 0);

	if (HEV_RCAST_MESSAGE_KEY_FRAME == hev_rcast_buffer_get_type (buffer))
		self->rsync = 0;

	for (session=self->output_sessions; session; session=session->next) {
		HevRcastOutputSession *s = (HevRcastOutputSession *) session;

		hev_rcast_buffer_ref (buffer);
		if (hev_rcast_output_session_push_buffer (s, buffer))
			rsync_manager_request_rsync (self);
	}

	hev_rcast_buffer_unref (buffer);
}

static void
rsync_manager_request_rsync (HevRcastServer *self)
{
	self->rsync = 1;
	hev_task_wakeup (self->task_rsync_manager);
}

static void
session_manager_insert_session (HevRcastBaseSession **list, HevRcastBaseSession *session)
{
#ifdef _DEBUG
	printf ("Insert session: %p\n", session);
#endif
	/* insert session to temp sessions */
	session->prev = NULL;
	session->next = *list;
	if (*list)
		(*list)->prev = session;
	*list = session;
}

static void
session_manager_remove_session (HevRcastBaseSession **list, HevRcastBaseSession *session)
{
#ifdef _DEBUG
	printf ("Remove session: %p\n", session);
#endif
	/* remove session from temp sessions */
	if (session->prev) {
		session->prev->next = session->next;
	} else {
		*list = session->next;
	}
	if (session->next) {
		session->next->prev = session->prev;
	}
}

static void
temp_session_notify_handler (HevRcastBaseSession *session,
			HevRcastBaseSessionNotifyAction action, void *data)
{
	HevRcastServer *self = data;

	switch (action) {
	case HEV_RCAST_BASE_SESSION_NOTIFY_TO_INPUT:
	{
		HevRcastInputSession *s;

		s = hev_rcast_input_session_new (session->fd, input_session_notify_handler, self);
		if (s) {
			if (self->input_session)
				hev_rcast_base_session_quit (self->input_session);
			self->input_session = (HevRcastBaseSession *) s;
			hev_rcast_input_session_run (s);
			hev_task_wakeup (self->task_rsync_manager);
		} else {
			close (session->fd);
		}
		break;
	}
	case HEV_RCAST_BASE_SESSION_NOTIFY_TO_OUTPUT:
	{
		HevRcastOutputSession *s;

		s = hev_rcast_output_session_new (session->fd, output_session_notify_handler, self);
		if (s) {
			session_manager_insert_session (&self->output_sessions, (HevRcastBaseSession *) s);
			hev_rcast_output_session_run (s);
			if (self->input_session) {
				HevRcastInputSession *is;
				HevRcastBuffer *buffer;

				is = (HevRcastInputSession *) self->input_session;
				buffer = hev_rcast_input_session_get_buffer (is, 1);
				if (buffer)
					hev_rcast_output_session_push_buffer (s, buffer);
			}
			rsync_manager_request_rsync (self);
		} else {
			close (session->fd);
		}
		break;
	}
	default:
		close (session->fd);
		break;
	}

	session_manager_remove_session (&self->temp_sessions, session);
	hev_rcast_temp_session_unref ((HevRcastTempSession *) session);
}

static void
input_session_notify_handler (HevRcastBaseSession *session,
			HevRcastBaseSessionNotifyAction action, void *data)
{
	HevRcastServer *self = data;

	switch (action) {
	case HEV_RCAST_BASE_SESSION_NOTIFY_DISPATCH:
		hev_rcast_dispatch_buffer (self);
		break;
	default:
		if (self->input_session == session)
			self->input_session = NULL;
		hev_rcast_input_session_unref ((HevRcastInputSession *) session);
	}
}

static void
output_session_notify_handler (HevRcastBaseSession *session,
			HevRcastBaseSessionNotifyAction action, void *data)
{
	HevRcastServer *self = data;

	session_manager_remove_session (&self->output_sessions, session);
	hev_rcast_output_session_unref ((HevRcastOutputSession *) session);
}

