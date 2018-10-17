/*
 ============================================================================
 Name        : hev-rcast-input-session.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast input session
 ============================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "hev-rcast-input-session.h"
#include "hev-rcast-protocol.h"
#include "hev-rcast-server.h"

#include <hev-task.h>
#include <hev-task-io-socket.h>
#include <hev-memory-allocator.h>

struct _HevRcastInputSession
{
    HevRcastBaseSession base;

    int ref_count;

    HevRcastBaseSessionNotify notify;
    void *notify_data;

    HevRcastBuffer *buffer;
    HevRcastBuffer *buffer_cfg;
};

static void hev_rcast_task_entry (void *data);

HevRcastInputSession *
hev_rcast_input_session_new (int fd, HevRcastBaseSessionNotify notify,
                             void *data)
{
    HevRcastInputSession *self;

    self = hev_malloc0 (sizeof (HevRcastInputSession));
    if (!self) {
        fprintf (stderr, "Alloc HevRcastInputSession failed!\n");
        return NULL;
    }

    self->base.task = hev_task_new (-1);
    if (!self->base.task) {
        fprintf (stderr, "Create input session task failed!\n");
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

HevRcastInputSession *
hev_rcast_input_session_ref (HevRcastInputSession *self)
{
    self->ref_count++;

    return self;
}

void
hev_rcast_input_session_unref (HevRcastInputSession *self)
{
    self->ref_count--;
    if (self->ref_count)
        return;

    close (self->base.fd);

    if (self->buffer_cfg)
        hev_rcast_buffer_unref (self->buffer_cfg);

    if (self->buffer)
        hev_rcast_buffer_unref (self->buffer);

    hev_free (self);
}

void
hev_rcast_input_session_run (HevRcastInputSession *self)
{
    hev_task_run (self->base.task, hev_rcast_task_entry, self);
}

HevRcastBuffer *
hev_rcast_input_session_get_buffer (HevRcastInputSession *self, int cfg)
{
    HevRcastBuffer *buffer;

    if (cfg) {
        if (!self->buffer_cfg)
            return NULL;
        return hev_rcast_buffer_ref (self->buffer_cfg);
    }

    buffer = self->buffer;
    self->buffer = NULL;

    return buffer;
}

int
hev_rcast_input_session_rsync (HevRcastInputSession *self)
{
    HevRcastMessage msg;

    msg.type = HEV_RCAST_MESSAGE_RSYNC;

    return send (self->base.fd, &msg, sizeof (msg.type), 0);
}

static int
task_io_yielder (HevTaskYieldType type, void *data)
{
    HevRcastInputSession *self = data;

    hev_task_yield (type);

    return (self->base.hp > 0) ? 0 : -1;
}

static void
hev_rcast_task_entry (void *data)
{
    HevRcastInputSession *self = data;
    HevTask *task = hev_task_self ();
    HevRcastMessage msg;
    size_t msg_len;
    ssize_t len;
    HevRcastBaseSessionNotifyAction action;

    hev_task_add_fd (task, self->base.fd, EPOLLIN);

    for (;;) {
        HevRcastBuffer *buffer;
        size_t data_len;
        void *data;

        msg_len = sizeof (msg.type) + sizeof (HevRcastMessageFrame);
        len = hev_task_io_socket_recv (self->base.fd, &msg, msg_len,
                                       MSG_WAITALL, task_io_yielder, self);
        if (len != msg_len || HEV_RCAST_MESSAGE_FRAME != msg.type) {
            goto notify;
        }

        data_len = ntohl (msg.frame.length);

    retry_alloc:
        buffer = hev_rcast_buffer_new (msg.frame.type, data_len);
        if (!buffer) {
            hev_task_sleep (100);
            goto retry_alloc;
        }

        data = hev_rcast_buffer_get_data (buffer);
        len = hev_task_io_socket_recv (self->base.fd, data, data_len,
                                       MSG_WAITALL, task_io_yielder, self);
        if (len != data_len) {
            hev_rcast_buffer_unref (buffer);
            goto notify;
        }

        /* config frame */
        if (HEV_RCAST_MESSAGE_CFG_FRAME == msg.frame.type) {
            if (self->buffer_cfg)
                hev_rcast_buffer_unref (self->buffer_cfg);
            self->buffer_cfg = hev_rcast_buffer_ref (buffer);
        }

        /* dispatch frames */
        if (self->buffer)
            hev_rcast_buffer_unref (self->buffer);
        self->buffer = buffer;

        hev_rcast_base_session_reset_hp (&self->base);

        action = HEV_RCAST_BASE_SESSION_NOTIFY_DISPATCH;
        self->notify ((HevRcastBaseSession *)self, action, self->notify_data);
    }

notify:
    action = HEV_RCAST_BASE_SESSION_NOTIFY_FREE;
    self->notify ((HevRcastBaseSession *)self, action, self->notify_data);
}
