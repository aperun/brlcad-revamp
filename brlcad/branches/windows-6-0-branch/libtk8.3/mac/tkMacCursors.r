/* 
 * tkMacCursors.r --
 *
 *	This file defines a set of Macintosh cursor resources that
 * 	are only available on the Macintosh platform.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

/*
 * These are resource definitions for Macintosh cursors.
 * The are identified and loaded by the "name" of the
 * cursor.  However, the ids must be unique.
 */

data 'CURS' (1000, "hand") {
	$"0180 1A70 2648 264A 124D 1249 6809 9801"
	$"8802 4002 2002 2004 1004 0808 0408 0408"
	$"0180 1BF0 3FF8 3FFA 1FFF 1FFF 6FFF FFFF"
	$"FFFE 7FFE 3FFE 3FFC 1FFC 0FF8 07F8 07F8"
	$"0009 0008"                              
};

data 'CURS' (1002, "bucket") {
	$"0000 0000 0600 0980 0940 0B30 0D18 090C"
	$"129C 212C 104C 088C 050C 0208 0000 0000"
	$"0000 0000 0600 0980 09C0 0BF0 0FF8 0FFC"
	$"1FFC 3FEC 1FCC 0F8C 070C 0208 0000 0000"
	$"000D 000C"                              
};

data 'CURS' (1003, "cancel") {
	$"0000 0000 0000 0000 3180 4A40 4A40 3F80"
	$"0A00 3F80 4A40 4A46 3186 0000 0000 0000"
	$"0000 0000 0000 3180 7BC0 FFE0 FFE0 7FC0"
	$"3F80 7FC0 FFE6 FFEF 7BCF 3186 0000 0000"
	$"0008 0005"                              
};

data 'CURS' (1004, "Resize") {
	$"FFFF 8001 BF01 A181 A1F9 A18D A18D BF8D"
	$"9F8D 880D 880D 880D 8FFD 87FD 8001 FFFF"
	$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
	$"FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF"
	$"0008 0008"                              
};

data 'CURS' (1005, "eyedrop") {
	$"000E 001F 001F 00FF 007E 00B8 0118 0228"
	$"0440 0880 1100 2200 4400 4800 B000 4000"
	$"000E 001F 001F 00FF 007E 00F8 01F8 03E8"
	$"07C0 0F80 1F00 3E00 7C00 7800 F000 4000"
	$"000F 0000"                              
};

data 'CURS' (1006, "eyedrop-full") {
	$"000E 001F 001F 00FF 007E 00B8 0118 0328"
	$"07C0 0F80 1F00 3E00 7C00 7800 F000 4000"
	$"000E 001F 001F 00FF 007E 00F8 01F8 03E8"
	$"07C0 0F80 1F00 3E00 7C00 7800 F000 4000"
	$"000F 0000"                              
};

data 'CURS' (1007, "zoom-in") {
	$"0780 1860 2790 5868 5028 A014 AFD4 AFD4"
	$"A014 5028 5868 2798 187C 078E 0007 0003"
	$"0780 1FE0 3FF0 7878 7038 E01C EFDC EFDC"
	$"E01C 7038 7878 3FF8 1FFC 078E 0007 0003"
	$"0007 0007"                              
};

data 'CURS' (1008, "zoom-out") {
	$"0780 1860 2790 5868 5328 A314 AFD4 AFD4"
	$"A314 5328 5868 2798 187C 078E 0007 0003"
	$"0780 1FE0 3FF0 7878 7338 E31C EFDC EFDC"
	$"E31C 7338 7878 3FF8 1FFC 078E 0007 0003"
	$"0007 0007"                              
};

/*
 * The following are resource definitions for color
 * cursors on the Macintosh.  If a color cursor and
 * a black & white cursor are both defined with the 
 * same name preference will be given to the color
 * cursors.
 */

data 'crsr' (1000, "hand") {
	$"8001 0000 0060 0000 0092 0000 0000 0000"
	$"0000 0000 0180 1A70 2648 264A 124D 1249"
	$"6809 9801 8802 4002 2002 2004 1004 0808"
	$"0408 0408 0180 1BF0 3FF8 3FFA 1FFF 1FFF"
	$"6FFF FFFF FFFE 7FFE 3FFE 3FFC 1FFC 0FF8"
	$"07F8 07F8 0008 0008 0000 0000 0000 0000"
	$"0000 0000 8004 0000 0000 0010 0010 0000"
	$"0000 0000 0000 0048 0000 0048 0000 0000"
	$"0002 0001 0002 0000 0000 0000 00D2 0000"
	$"0000 0003 C000 03CD 7F00 0D7D 75C0 0D7D"
	$"75CC 035D 75F7 035D 75D7 3CD5 55D7 D7D5"
	$"5557 D5D5 555C 3555 555C 0D55 555C 0D55"
	$"5570 0355 5570 00D5 55C0 0035 55C0 0035"
	$"55C0 0000 0000 0000 0002 0000 FFFF FFFF"
	$"FFFF 0001 FFFF CCCC 9999 0003 0000 0000"
	$"0000"                                   
};

data 'crsr' (1001, "fist") {
	$"8001 0000 0060 0000 0092 0000 0000 0000"
	$"0000 0000 0000 0000 0000 0000 0DB0 124C"
	$"100A 0802 1802 2002 2002 2004 1004 0808"
	$"0408 0408 0000 0000 0000 0000 0DB0 1FFC"
	$"1FFE 0FFE 1FFE 3FFE 3FFE 3FFC 1FFC 0FF8"
	$"07F8 07F8 0008 0008 0000 0000 0000 0000"
	$"0000 0000 8004 0000 0000 0010 0010 0000"
	$"0000 0000 0000 0048 0000 0048 0000 0000"
	$"0002 0001 0002 0000 0000 0000 00D2 0000"
	$"0000 0000 0000 0000 0000 0000 0000 0000"
	$"0000 00F3 CF00 035D 75F0 0355 55DC 00D5"
	$"555C 03D5 555C 0D55 555C 0D55 555C 0D55"
	$"5570 0355 5570 00D5 55C0 0035 55C0 0035"
	$"55C0 0000 0000 0000 0002 0000 FFFF FFFF"
	$"FFFF 0001 FFFF CCCC 9999 0003 0000 0000"
	$"0000"                                   
};

