/*
 ============================================================================
 Name        : hev-main.c
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Main
 ============================================================================
 */

#include <stdio.h>
#include <signal.h>

#include <hev-task-system.h>

#include "hev-main.h"
#include "hev-config.h"
#include "hev-rcast-server.h"

static HevRcastServer *server;

static void
show_help (const char *self_path)
{
    printf ("%s CONFIG_PATH\n", self_path);
    printf ("Version: %u.%u.%u\n", MAJOR_VERSION, MINOR_VERSION, MICRO_VERSION);
}

static void
signal_handler (int signum)
{
    quit ();
}

int
main (int argc, char *argv[])
{
    if (2 != argc) {
        show_help (argv[0]);
        return -1;
    }

    if (0 < hev_config_init (argv[1])) {
        return -1;
    }

    if (-1 == hev_task_system_init ()) {
        fprintf (stderr, "Initialize task system failed!\n");
        return -1;
    }

    server = hev_rcast_server_new ();
    if (!server) {
        return -1;
    }

    signal (SIGPIPE, SIG_IGN);
    signal (SIGINT, signal_handler);

    hev_rcast_server_run (server);
    hev_task_system_run ();

    hev_rcast_server_destroy (server);
    hev_task_system_fini ();
    hev_config_fini ();

    return 0;
}

void
quit (void)
{
    hev_rcast_server_quit (server);
}
