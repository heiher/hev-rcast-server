/*
 ============================================================================
 Name        : hev-rcast-http-session.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast http session
 ============================================================================
 */

#include <stdio.h>
#include <string.h>
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
hev_rcast_http_session_new (int fd, HevRcastBaseSessionNotify notify,
                            void *data)
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
    self->ref_count++;

    return self;
}

void
hev_rcast_http_session_unref (HevRcastHttpSession *self)
{
    self->ref_count--;
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

static int
task_io_yielder (HevTaskYieldType type, void *data)
{
    HevRcastHttpSession *self = data;

    hev_task_yield (type);

    return (self->base.hp > 0) ? 0 : -1;
}

static void
hev_rcast_task_entry (void *data)
{
    HevRcastHttpSession *self = data;
    HevTask *task = hev_task_self ();
    HevRcastBaseSessionNotifyAction action;
    HevRcastBuffer *buffer;
    char buf[4096], *http = NULL;
    size_t recv_len = 0, data_len;
    ssize_t len;
    unsigned int type = 0;
    static const char *http_response =
    "HTTP/1.1 200 OK\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: GET\r\n"
    "Content-Type: application/json; charset=utf-8\r\n"
    "Content-Length: 20\r\n\r\n"
    "{ \"result\": \"succ\" }";
    static size_t http_response_length = 0;

    if (!http_response_length)
        http_response_length = strlen (http_response);

    hev_task_add_fd (task, self->base.fd, EPOLLIN | EPOLLOUT);

    for (;;) {
        len = hev_task_io_socket_recv (self->base.fd, buf + recv_len,
                                       4095 - recv_len, 0, task_io_yielder,
                                       self);
        if (0 >= len)
            goto notify;
        recv_len += len;

        buf[recv_len] = '\0';
        if (recv_len >= (2 + 11)) { /* T / HTTP/1.1\r|\n */
            if (buf[0] != 'T' || buf[1] != ' ')
                goto notify;

            if ((http = strstr (buf, " HTTP/1.")))
                break;
        }
    }

    if (!http)
        goto notify;

    len = hev_task_io_socket_send (self->base.fd, http_response,
                                   http_response_length, MSG_WAITALL,
                                   task_io_yielder, self);
    if (len != http_response_length)
        goto notify;

    data_len = http - (buf + 2);
    if ((data_len >= (2 + 5)) && (0 == memcmp (buf + 2, "/conf", 5)))
        type = 1;

retry_alloc:
    buffer = hev_rcast_buffer_new (type, data_len);
    if (!buffer) {
        hev_task_sleep (100);
        goto retry_alloc;
    }

    data = hev_rcast_buffer_get_data (buffer);
    memcpy (data, buf + 2, data_len);

    /* dispatch frames */
    if (self->buffer)
        hev_rcast_buffer_unref (self->buffer);
    self->buffer = buffer;

    action = HEV_RCAST_BASE_SESSION_NOTIFY_DISPATCH;
    self->notify ((HevRcastBaseSession *)self, action, self->notify_data);

notify:
    action = HEV_RCAST_BASE_SESSION_NOTIFY_FREE;
    self->notify ((HevRcastBaseSession *)self, action, self->notify_data);
}
