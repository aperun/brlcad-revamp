/*                       C U R S O R S . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2013 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file lgt/cursors.h
 *	Author:		Gary S. Moss
 * From AT&T's Teletype 5620 $DMD/icon directory, reformatted slightly
 * to be an array of 32 unsigned chars, rather than a Texture16.
 */
unsigned char menucursor[32] =
{
    0xFF, 0xC0,
    0x80, 0x40,
    0x80, 0x40,
    0x80, 0x40,
    0xFF, 0xC0,
    0xFF, 0xC0,
    0xFE, 0x00,
    0xFE, 0xF0,
    0x80, 0xE0,
    0x80, 0xF0,
    0x80, 0xB8,
    0xFE, 0x1C,
    0x80, 0x0E,
    0x80, 0x47,
    0x80, 0x42,
    0xFF, 0xC0
};
unsigned char sweeportrack[32] =
{
    0xFF, 0xFF,
    0x80, 0x01,
    0xBF, 0xFD,
    0xA0, 0x05,
    0xA0, 0x05,
    0xA0, 0x05,
    0xA0, 0x05,
    0xA0, 0x05,
    0x8F, 0x05,
    0x87, 0x05,
    0x0F, 0x05,
    0x1D, 0x05,
    0x38, 0x05,
    0x70, 0xFD,
    0xE0, 0x01,
    0x43, 0xFF
};
unsigned char target1[32] =
{
    0x07, 0xE0,
    0x1F, 0xF8,
    0x39, 0x9C,
    0x63, 0xC6,
    0x6F, 0xF6,
    0xCD, 0xB3,
    0xD9, 0x9B,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xD9, 0x9B,
    0xCD, 0xB3,
    0x6F, 0xF6,
    0x63, 0xC6,
    0x39, 0x9C,
    0x1F, 0xF8,
    0x07, 0xE0
};
unsigned char	arrowcursor[32] =
{
    0xFF, 0xC0,
    0xFF, 0x00,
    0xFC, 0x00,
    0xFE, 0x00,
    0xFF, 0x00,
    0xEF, 0x80,
    0xC7, 0xC0,
    0xC3, 0xE0,
    0x81, 0xF0,
    0x80, 0xF8,
    0x00, 0x7C,
    0x00, 0x3E,
    0x00, 0x1F,
    0x00, 0x0F,
    0x00, 0x06,
    0x00, 0x00
};

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
