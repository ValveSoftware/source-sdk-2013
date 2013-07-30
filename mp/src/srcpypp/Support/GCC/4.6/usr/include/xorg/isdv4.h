/*
 * Copyright 2010 by Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef ISDV4_H
#define ISDV4_H

#define ISDV4_QUERY "*"       /* ISDV4 query command */
#define ISDV4_RESET "&"       /* ISDV4 touch panel reset command */
#define ISDV4_TOUCH_QUERY "%" /* ISDV4 touch query command */
#define ISDV4_STOP "0"        /* ISDV4 stop command */
#define ISDV4_SAMPLING "1"    /* ISDV4 sampling command */

/* packet length for individual models */
#define ISDV4_PKGLEN_TOUCH93    5
#define ISDV4_PKGLEN_TOUCH9A    7
#define ISDV4_PKGLEN_TPCPEN     9
#define ISDV4_PKGLEN_TPCCTL     11
#define ISDV4_PKGLEN_TOUCH2FG   13

#define HEADER_BIT      0x80
#define CONTROL_BIT     0x40
#define DATA_ID_MASK    0x3F
#define TOUCH_CONTROL_BIT 0x10

/* Only for touch devices: use serial ID as index to get packet length for device */
int ISDV4PacketLengths[] = {
	/* 0x00 => */ ISDV4_PKGLEN_TOUCH93,
	/* 0x01 => */ ISDV4_PKGLEN_TOUCH9A,
	/* 0x02 => */ ISDV4_PKGLEN_TOUCH93,
	/* 0x03 => */ ISDV4_PKGLEN_TOUCH9A,
	/* 0x04 => */ ISDV4_PKGLEN_TOUCH9A,
	/* 0x05 => */ ISDV4_PKGLEN_TOUCH2FG
};

/* ISDV4 protocol parsing structs. */

/* Query reply data */
typedef struct {
	unsigned char data_id;	 /* always 00H */
	uint16_t x_max;
	uint16_t y_max;
	uint16_t pressure_max;
	uint8_t  tilt_x_max;
	uint8_t  tilt_y_max;
	uint16_t version;
} ISDV4QueryReply;

/* Touch Query reply data */
typedef struct {
	uint8_t data_id;	/* always 01H */
	uint8_t panel_resolution;
	uint8_t sensor_id;
	uint16_t x_max;
	uint16_t y_max;
	uint8_t capacity_resolution;
	uint16_t version;
} ISDV4TouchQueryReply;

/* Touch Data format. Note that capacity and finger2 are only set for some
 * devices (0 on all others) */
typedef struct {
	uint8_t status;		/* touch down/up */
	uint16_t x;
	uint16_t y;
	uint16_t capacity;
	struct {
		uint8_t status;		/* touch down/up */
		uint16_t x;
		uint16_t y;
	} finger2;
} ISDV4TouchData;

/* Coordinate data format */
typedef struct {
	uint8_t proximity;	/* in proximity? */
	uint8_t tip;		/* tip/eraser pressed? */
	uint8_t side;		/* side switch pressed? */
	uint8_t eraser;		/* eraser pressed? */
	uint16_t x;
	uint16_t y;
	uint16_t pressure;
	uint8_t tilt_x;
	uint8_t tilt_y;
} ISDV4CoordinateData;

static int isdv4ParseQuery(const unsigned char *buffer, const size_t len,
		    ISDV4QueryReply *reply)
{
	int header, control;

	if (!reply || len < ISDV4_PKGLEN_TPCCTL)
		return 0;

	header = !!(buffer[0] & HEADER_BIT);
	control = !!(buffer[0] & CONTROL_BIT);

	if (!header || !control)
		return -1;

	reply->data_id = buffer[0] & DATA_ID_MASK;

	/* FIXME: big endian? */
	reply->x_max = (buffer[1] << 9) | (buffer[2] << 2) | ((buffer[6] >> 5) & 0x3);
	reply->y_max = (buffer[3] << 9) | (buffer[4] << 2) | ((buffer[6] >> 3) & 0x3);
	reply->pressure_max = (buffer[6] & 0x7) << 7 | buffer[5];
	reply->tilt_y_max = buffer[7];
	reply->tilt_x_max = buffer[8];
	reply->version = buffer[9] << 7 | buffer[10];

	return ISDV4_PKGLEN_TPCCTL;
}

static int isdv4ParseTouchQuery(const unsigned char *buffer, const size_t len,
			 ISDV4TouchQueryReply *reply)
{
	int header, control;

	if (!reply || len < ISDV4_PKGLEN_TPCCTL)
		return 0;

	header = !!(buffer[0] & HEADER_BIT);
	control = !!(buffer[0] & CONTROL_BIT);

	if (!header || !control)
		return -1;

	reply->data_id = buffer[0] & DATA_ID_MASK;
	reply->sensor_id = buffer[2] & 0x7;
	reply->panel_resolution = buffer[1];
	/* FIXME: big endian? */
	reply->x_max = (buffer[3] << 9) | (buffer[4] << 2) | ((buffer[2] >> 5) & 0x3);
	reply->y_max = (buffer[5] << 9) | (buffer[6] << 2) | ((buffer[2] >> 3) & 0x3);
	reply->capacity_resolution = buffer[7];
	reply->version = buffer[9] << 7 | buffer[10];

	return ISDV4_PKGLEN_TPCCTL;
}

/* pktlen defines what touch type we parse */
static int isdv4ParseTouchData(const unsigned char *buffer, const size_t buff_len,
		        const size_t pktlen, ISDV4TouchData *touchdata)
{
	int header, touch;

	if (!touchdata || buff_len < pktlen)
		return 0;

	header = !!(buffer[0] & HEADER_BIT);
	touch = !!(buffer[0] & TOUCH_CONTROL_BIT);

	if (header != 1 || touch != 1)
		return -1;

	memset(touchdata, 0, sizeof(*touchdata));

	touchdata->status = buffer[0] & 0x1;
	/* FIXME: big endian */
	touchdata->x = buffer[1] << 7 | buffer[2];
	touchdata->y = buffer[3] << 7 | buffer[4];
	if (pktlen == ISDV4_PKGLEN_TOUCH9A)
		touchdata->capacity = buffer[5] << 7 | buffer[6];

	if (pktlen == ISDV4_PKGLEN_TOUCH2FG)
	{
		touchdata->finger2.x = buffer[7] << 7 | buffer[8];
		touchdata->finger2.y = buffer[9] << 7 | buffer[10];
		touchdata->finger2.status = !!(buffer[0] & 0x2);
		/* FIXME: is there a fg2 capacity? */
	}

	return pktlen;
}

static int isdv4ParseCoordinateData(const unsigned char *buffer, const size_t len,
			     ISDV4CoordinateData *coord)
{
	int header, control;

	if (!coord || len < ISDV4_PKGLEN_TPCPEN)
		return 0;

	header = !!(buffer[0] & HEADER_BIT);
	control = !!(buffer[0] & TOUCH_CONTROL_BIT);

	if (header != 1 || control != 0)
		return -1;

	coord->proximity = (buffer[0] >> 5) & 0x1;
	coord->tip = buffer[0] & 0x1;
	coord->side = (buffer[0] >> 1) & 0x1;
	coord->eraser = (buffer[0] >> 2) & 0x1;
	/* FIXME: big endian */
	coord->x = (buffer[1] << 9) | (buffer[2] << 2) | ((buffer[6] >> 5) & 0x3);
	coord->y = (buffer[3] << 9) | (buffer[4] << 2) | ((buffer[6] >> 3) & 0x3);

	coord->pressure = ((buffer[6] & 0x7) << 7) | buffer[5];
	coord->tilt_x = buffer[7];
	coord->tilt_y = buffer[8];

	return ISDV4_PKGLEN_TPCPEN;
}

#endif /* ISDV4_H */

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
