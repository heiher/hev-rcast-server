/*
 ============================================================================
 Name        : hev-config.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 Heiher.
 Description : Config
 ============================================================================
 */

#ifndef __HEV_CONFIG_H__
#define __HEV_CONFIG_H__

#define MAJOR_VERSION (2)
#define MINOR_VERSION (0)
#define MICRO_VERSION (2)

int hev_config_init (const char *config_path);
void hev_config_fini (void);

const char *hev_config_get_listen_address (void);
unsigned short hev_config_get_port (void);

unsigned int hev_config_get_rcast_frame_buffers (void);
unsigned int hev_config_get_rcast_rsync_interval (void);

#endif /* __HEV_CONFIG_H__ */
