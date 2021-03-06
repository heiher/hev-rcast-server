/*
 ============================================================================
 Name        : hev-rcast-base-session.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast base session
 ============================================================================
 */

#include "hev-rcast-base-session.h"

void
hev_rcast_base_session_quit (HevRcastBaseSession *self)
{
    self->hp = 0;
    hev_task_wakeup (self->task);
}

void
hev_rcast_base_session_reset_hp (HevRcastBaseSession *self)
{
    if (self->hp)
        self->hp = HEV_RCAST_BASE_SESSION_HP;
}
