/*
 * Copyright 2003 by John Joganic <john@joganic.com>
 * Copyright 2003 - 2009 by Ping Cheng <pingc@wacom.com> 
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

/**
 * @mainpage The X.Org Wacom Input Driver API Documentation
 * @section intro Introduction
 * The Linux Wacom Project manages the drivers, libraries, and documentation
 * for configuring and running Wacom tablets under the Linux operating system.
 * It contains diagnostic applications and X.Org input drivers
 * for servers 1.7 and later.
 */

#ifndef __XORG_XWACOM_H
#define __XORG_XWACOM_H

#include <X11/keysym.h>

#define TV_NONE 		0
#define TV_ABOVE_BELOW 		1
#define TV_LEFT_RIGHT		2
#define TV_BELOW_ABOVE		3
#define TV_RIGHT_LEFT		4
#define TV_MAX			4

#define ROTATE_NONE 		0
#define ROTATE_CW 		1
#define ROTATE_CCW 		2
#define ROTATE_HALF 		3

/* The following flags are used for button action property values to mark
 * the type of event that should be emitted when that button is pressed;
 * combined together they form an Action Code (AC). Each button has up to
 * 256 actions on press, where a zero terminates the actions.
 *
 * e.g.
 * AC_KEY | AC_KEYBTNPRESS | <keycode> is a key press for key <keycode>.
 * AC_BUTTON | AC_KEYBTNPRESS | 1 is a button press for 1
 * AC_BUTTON | 1 is a button release for 1
 *
 * if no action is set for a button, the button behaves normally.
 */
#define AC_CODE             0x0000ffff	/* Mask to isolate button number or key code */
#define AC_KEY              0x00010000	/* Emit key events */
#define AC_MODETOGGLE       0x00020000	/* Toggle absolute/relative mode */
#define AC_DBLCLICK         0x00030000	/* DEPRECATED: use two button events instead */
#define AC_DISPLAYTOGGLE    0x00040000 /* Toggle among screens */
#define AC_BUTTON           0x00080000	/* Emit button events */
#define AC_TYPE             0x000f0000	/* The mask to isolate event type bits */
#define AC_KEYBTNPRESS      0x00100000  /* bit set for key/button presses */
#define AC_CORE             0x10000000	/* DEPRECATED: has no effect */
#define AC_EVENT            0xf00f0000	/* Mask to isolate event flag */

#endif /* __XORG_XWACOM_H */
