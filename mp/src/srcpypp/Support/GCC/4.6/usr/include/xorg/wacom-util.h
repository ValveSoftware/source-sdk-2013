/*
 * Copyright 2011 by Jason Gerecke, Wacom. <jason.gerecke@wacom.com>
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

#ifndef WACOM_UTIL_H_
#define WACOM_UTIL_H_

/**
 * Get the number of elements in an array
 */
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

/* to access kernel defined bits */
#define BIT(x)		(1UL<<((x) & (BITS_PER_LONG - 1)))
#define BITS_PER_LONG	(sizeof(long) * 8)
#define NBITS(x)	((((x)-1)/BITS_PER_LONG)+1)
#define ISBITSET(x,y)	((x)[LONG(y)] & BIT(y))
#define SETBIT(x,y)	((x)[LONG(y)] |= BIT(y))
#define CLEARBIT(x,y)	((x)[LONG(y)] &= ~BIT(y))
#define OFF(x)		((x)%BITS_PER_LONG)
#define LONG(x)		((x)/BITS_PER_LONG)

/**
 * Test if the mask is set in the given bitfield.
 * @return TRUE if set or FALSE otherwise.
 */
#define MaskIsSet(bitfield, mask) !!(((bitfield) & (mask)) == (mask))
/**
 * Set the given mask for the given bitfield.
 */
#define MaskSet(bitfield, mask) ((bitfield) |= (mask))
/**
 * Clear the given mask from the given bitfield
 */
#define MaskClear(bitfield, mask) ((bitfield) &= ~(mask))

#endif /* WACOM_UTIL_H_ */
