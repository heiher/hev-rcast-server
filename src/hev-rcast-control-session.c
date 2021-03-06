/*
 ============================================================================
 Name        : hev-rcast-control-session.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast control session
 ============================================================================
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "hev-rcast-control-session.h"
#include "hev-rcast-protocol.h"
#include "hev-rcast-server.h"
#include "hev-config.h"

#include <hev-task.h>
#include <hev-task-io.h>
#include <hev-task-io-socket.h>
#include <hev-memory-allocator.h>

struct _HevRcastControlSession
{
    HevRcastBaseSession base;

    int ref_count;

    HevRcastBaseSessionNotify notify;
    void *notify_data;

    unsigned int buffers_r;
    unsigned int buffers_w;
    unsigned int buffers_count;
    HevRcastBuffer *buffers[0];
};

static void hev_rcast_task_entry (void *data);

HevRcastControlSession *
hev_rcast_control_session_new (int fd, HevRcastBaseSessionNotify notify,
                               void *data)
{
    HevRcastControlSession *self;
    unsigned int buffers_count = 16;
    unsigned int buffers_size;

    buffers_size = sizeof (HevRcastBuffer *) * buffers_count;
    self = hev_malloc0 (sizeof (HevRcastControlSession) + buffers_size);
    if (!self) {
        fprintf (stderr, "Alloc HevRcastControlSession failed!\n");
        return NULL;
    }

    self->base.task = hev_task_new (-1);
    if (!self->base.task) {
        fprintf (stderr, "Create control session task failed!\n");
        hev_free (self);
        return NULL;
    }

    self->base.hp = HEV_RCAST_BASE_SESSION_HP;
    self->base.fd = fd;

    self->ref_count = 1;
    self->notify = notify;
    self->notify_data = data;
    self->buffers_count = buffers_count;

    return self;
}

HevRcastControlSession *
hev_rcast_control_session_ref (HevRcastControlSession *self)
{
    self->ref_count++;

    return self;
}

void
hev_rcast_control_session_unref (HevRcastControlSession *self)
{
    self->ref_count--;
    if (self->ref_count)
        return;

    close (self->base.fd);

    for (; self->buffers_r != self->buffers_w;) {
        hev_rcast_buffer_unref (self->buffers[self->buffers_r]);
        self->buffers_r = (self->buffers_r + 1) % self->buffers_count;
    }

    hev_free (self);
}

void
hev_rcast_control_session_run (HevRcastControlSession *self)
{
    hev_task_run (self->base.task, hev_rcast_task_entry, self);
}

void
hev_rcast_control_session_push_buffer (HevRcastControlSession *self,
                                       HevRcastBuffer *buffer)
{
    unsigned int next_w;

    next_w = (self->buffers_w + 1) % self->buffers_count;
    if (self->buffers_r == next_w) {
        hev_rcast_buffer_unref (buffer);
        hev_rcast_base_session_quit (&self->base);
        return;
    }

    self->buffers[self->buffers_w] = buffer;
    self->buffers_w = next_w;

    hev_task_wakeup (self->base.task);
}

static int
task_io_recv (HevRcastControlSession *self)
{
    unsigned char buf[16];
    ssize_t len;
    HevRcastBuffer *buffer;

    len = recv (self->base.fd, buf, 16, 0);
    switch (len) {
    case -1:
        if (EAGAIN != errno)
            return -1;
        break;
    case 0:
        return -1;
    default:
        buffer = hev_rcast_buffer_new (0, 0);
        hev_rcast_control_session_push_buffer (self, buffer);
    }

    return 0;
}

static int
task_io_yielder (HevTaskYieldType type, void *data)
{
    HevRcastControlSession *self = data;

    hev_task_yield (type);

    return (self->base.hp > 0) ? 0 : -1;
}

static void
hev_rcast_task_entry (void *data)
{
    HevRcastControlSession *self = data;
    HevTask *task = hev_task_self ();
    HevRcastMessage msg;
    size_t msg_len;
    ssize_t len;
    HevRcastBaseSessionNotifyAction action;

    hev_task_add_fd (task, self->base.fd, POLLIN | POLLOUT);

    for (;;) {
        HevRcastBuffer *buffer;
        size_t data_len;
        void *data;

        if (self->buffers_r == self->buffers_w) {
            hev_task_yield (HEV_TASK_WAITIO);
            if (0 > task_io_recv (self))
                break;
            if (self->base.hp == 0)
                break;
            continue;
        }

        buffer = self->buffers[self->buffers_r];
        self->buffers_r = (self->buffers_r + 1) % self->buffers_count;

        data = hev_rcast_buffer_get_data (buffer);
        data_len = hev_rcast_buffer_get_data_length (buffer);

        msg.type = HEV_RCAST_MESSAGE_CONTROL;
        msg.control.length = htonl (data_len);
        msg_len = sizeof (msg.type) + sizeof (HevRcastMessageControl);
        len = hev_task_io_socket_send (self->base.fd, &msg, msg_len,
                                       MSG_WAITALL, task_io_yielder, self);
        if (len != msg_len) {
            goto notify;
        }

        if (data_len) {
            len = hev_task_io_socket_send (self->base.fd, data, data_len,
                                           MSG_WAITALL, task_io_yielder, self);
            if (len != data_len) {
                goto notify;
            }
        }

        hev_rcast_buffer_unref (buffer);

        hev_rcast_base_session_reset_hp (&self->base);
    }

notify:
    action = HEV_RCAST_BASE_SESSION_NOTIFY_FREE;
    self->notify ((HevRcastBaseSession *)self, action, self->notify_data);
}
