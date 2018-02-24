/*
 ============================================================================
 Name        : hev-rcast-protocol.h
 Author      : Heiher <r@hev.cc>
 Copyright   : Copyright (c) 2018 everyone.
 Description : Rcast protocol
 ============================================================================
 */

#ifndef __HEV_RCAST_PROTOCOL_H__
#define __HEV_RCAST_PROTOCOL_H__

typedef struct _HevRcastMessage HevRcastMessage;
typedef struct _HevRcastMessageLogin HevRcastMessageLogin;
typedef struct _HevRcastMessageFrame HevRcastMessageFrame;
typedef struct _HevRcastMessageControl HevRcastMessageControl;

typedef enum _HevRcastMessageType HevRcastMessageType;
typedef enum _HevRcastMessageLoginDirection HevRcastMessageLoginDirection;
typedef enum _HevRcastMessageFrameType HevRcastMessageFrameType;

enum _HevRcastMessageType
{
	HEV_RCAST_MESSAGE_LOGIN = 0,
	HEV_RCAST_MESSAGE_FRAME,
	HEV_RCAST_MESSAGE_RSYNC,
	HEV_RCAST_MESSAGE_CONTROL,
};

enum _HevRcastMessageLoginDirection
{
	HEV_RCAST_MESSAGE_LOGIN_INPUT = 0,
	HEV_RCAST_MESSAGE_LOGIN_OUTPUT,
	HEV_RCAST_MESSAGE_LOGIN_CONTROL,
};

enum _HevRcastMessageFrameType
{
	HEV_RCAST_MESSAGE_CFG_FRAME = 0,
	HEV_RCAST_MESSAGE_KEY_FRAME,
	HEV_RCAST_MESSAGE_REF_FRAME,
	HEV_RCAST_MESSAGE_KAL_FRAME,
};

struct _HevRcastMessageLogin
{
	unsigned char type;
} __attribute__((packed));

struct _HevRcastMessageFrame
{
	unsigned char type;
	unsigned int length;
} __attribute__((packed));

struct _HevRcastMessageControl
{
	unsigned int length;
	char command[0];
} __attribute__((packed));

struct _HevRcastMessage
{
	unsigned char type;

	union {
		HevRcastMessageLogin login;
		HevRcastMessageFrame frame;
		HevRcastMessageControl control;
	};
} __attribute__((packed));

#endif /* __HEV_RCAST_PROTOCOL_H__ */

