/* 
 * tkMacOSXApplication.r --
 *
 *	This file creates resources for use in the Wish application.
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#include <Carbon/Carbon.r>

/*
 * The folowing include and defines help construct
 * the version string for Tcl.
 */

#define RC_INVOKED
#include "tk.h"

#if (TK_RELEASE_LEVEL == 0)
#   define RELEASE_LEVEL alpha
#elif (TK_RELEASE_LEVEL == 1)
#   define RELEASE_LEVEL beta
#elif (TK_RELEASE_LEVEL == 2)
#   define RELEASE_LEVEL final
#endif

#if (TK_RELEASE_LEVEL == 2)
#   define MINOR_VERSION (TK_MINOR_VERSION * 16) + TK_RELEASE_SERIAL
#else
#   define MINOR_VERSION TK_MINOR_VERSION * 16
#endif

#define RELEASE_CODE 0x00

resource 'vers' (1) {
	TK_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, 0x00, verUS,
	TK_PATCH_LEVEL,
	TK_PATCH_LEVEL ", by Jim Ingham & Ian Reid" "\n" "� 2001 Apple Computer, Inc" "\n" "1998-2000 Scriptics Inc." "\n" "1996-1997 Sun Microsystems Inc."
};

resource 'vers' (2) {
	TK_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, 0x00, verUS,
	TK_PATCH_LEVEL,
	"Wish " TK_PATCH_LEVEL " � 1993-1999"
};

#define TK_APP_RESOURCES 128
#define TK_APP_CREATOR 'WIsH'

/*
 * The 'BNDL' resource is the primary link between a file's
 * creator/type and its icon.  This resource acts for all Tcl shared
 * libraries; other libraries will not need one and ought to use
 * custom icons rather than new file types for a different appearance.
 */

resource 'BNDL' (TK_APP_RESOURCES, "Tk app bundle", purgeable) 
{
	TK_APP_CREATOR,
	0,
	{
		'FREF',
		{
			0, TK_APP_RESOURCES,
			1, TK_APP_RESOURCES+1
		},
		'ICN#',
		{
			0, TK_APP_RESOURCES,
			1, TK_APP_RESOURCES+1
		}
	}
};

resource 'FREF' (TK_APP_RESOURCES, purgeable) 
{
	'APPL', 0, ""
};
resource 'FREF' (TK_APP_RESOURCES+1, purgeable) 
{
	'TEXT', 1, ""
};

type TK_APP_CREATOR as 'STR ';
resource TK_APP_CREATOR (0, purgeable) {
	"Wish " TK_PATCH_LEVEL " � 1996"
};

/*
 * The 'kind' resource works with a 'BNDL' in Macintosh Easy Open
 * to affect the text the Finder displays in the "kind" column and
 * file info dialog.  This information will be applied to all files
 * with the listed creator and type.
 */
resource 'kind' (TK_APP_RESOURCES, "Tcl kind", purgeable) {
	TK_APP_CREATOR,
	0, /* region = USA */
	{
		'APPL', "Wish",
		'TEXT', "Tcl/Tk Script"
	}
};

#define kIconHelpString 256

resource 'hfdr' (-5696, purgeable) {
   HelpMgrVersion, hmDefaultOptions, 0, 0,
   {HMSTRResItem {kIconHelpString}}
};
resource 'STR ' (kIconHelpString, purgeable) {
   "This is the interpreter for Tcl & Tk scripts"
   " running on Macintosh computers."
};

/*
 * The following resource define the icon used by Tcl scripts.  Any
 * TEXT file with the creator of WIsH will get this icon.
 */

data 'icl4' (TK_APP_RESOURCES + 1, "Tk Doc", purgeable) {
	$"000F FFFF FFFF FFFF FFFF FFF0 0000 0000"
	$"000F 3333 3333 3333 3333 33FF 0000 0000"
	$"000F 3333 3333 3333 3433 33F2 F000 0000"
	$"000F 3333 3333 3333 7D43 33F2 2F00 0000"
	$"000F 3333 3333 3335 5623 33F2 22F0 0000"
	$"000F 3333 3333 3356 6343 33FF FFFF 0000"
	$"000F 3333 3333 256F 5223 3333 333F 0000"
	$"000F 3333 3333 D666 2433 3333 333F 0000"
	$"000F 3333 3333 D5F6 6633 3333 333F 0000"
	$"000F 3333 3332 5666 6733 3333 333F 0000"
	$"000F 3333 3336 E56F 6633 3333 333F 0000"
	$"000F 3333 3336 5656 5733 3333 333F 0000"
	$"000F 3333 3336 E5B6 5233 3333 333F 0000"
	$"000F 3333 3336 5ED6 3333 3333 333F 0000"
	$"000F 3333 3376 6475 6233 3333 333F 0000"
	$"000F 3333 333D 5D56 7333 3333 333F 0000"
	$"000F 3333 3336 6C55 6333 3333 333F 0000"
	$"000F 3333 3336 5C56 7333 3333 333F 0000"
	$"000F 3333 3362 6CE6 D333 3333 333F 0000"
	$"000F 3333 3336 5C65 6333 3333 333F 0000"
	$"000F 3333 3336 EC5E 3333 3333 333F 0000"
	$"000F 3333 3336 5C56 6333 3333 333F 0000"
	$"000F 3333 3333 5C75 3333 3333 333F 0000"
	$"000F 3333 3333 5DD6 3333 3333 333F 0000"
	$"000F 3333 3333 3CDD 3333 3333 333F 0000"
	$"000F 3333 3333 3303 3333 3333 333F 0000"
	$"000F 3333 3333 3C33 3333 3333 333F 0000"
	$"000F 3333 3333 3C33 3333 3333 333F 0000"
	$"000F 3333 3333 3C33 3333 3333 333F 0000"
	$"000F 3333 3333 3333 3333 3333 333F 0000"
	$"000F 3333 3333 3333 3333 3333 333F 0000"
	$"000F FFFF FFFF FFFF FFFF FFFF FFFF 0000"
};

data 'ICN#' (TK_APP_RESOURCES + 1, "Tk Doc", purgeable) {
	$"1FFF FE00 1000 0300 1000 F280 1003 F240"
	$"1003 E220 1007 E3F0 100F C010 100F C010"
	$"100F C010 101F F010 101F F010 101F F010"
	$"101F F010 101F F010 101D E010 101D E010"
	$"101D E010 101D C010 101D C010 101D C010"
	$"101D C010 100D 8010 100D 8010 100D 8010"
	$"1005 8010 1002 0010 1002 0010 1002 0010"
	$"1002 0010 1002 0010 1000 0010 1FFF FFF0"
	$"1FFF FE00 1FFF FF00 1FFF FF80 1FFF FFC0"
	$"1FFF FFE0 1FFF FFF0 1FFF FFF0 1FFF FFF0"
	$"1FFF FFF0 1FFF FFF0 1FFF FFF0 1FFF FFF0"
	$"1FFF FFF0 1FFF FFF0 1FFF FFF0 1FFF FFF0"
	$"1FFF FFF0 1FFF FFF0 1FFF FFF0 1FFF FFF0"
	$"1FFF FFF0 1FFF FFF0 1FFF FFF0 1FFF FFF0"
	$"1FFF FFF0 1FFF FFF0 1FFF FFF0 1FFF FFF0"
	$"1FFF FFF0 1FFF FFF0 1FFF FFF0 1FFF FFF0"
};

data 'ics#' (TK_APP_RESOURCES + 1, "Tk Doc", purgeable) {
	$"7FF0 41D8 419C 4384 43C4 47C4 47C4 4784"
	$"4684 4684 4284 4284 4104 4104 4104 7FFC"
	$"7FE0 7FF0 7FF8 7FFC 7FFC 7FFC 7FFC 7FFC"
	$"7FFC 7FFC 7FFC 7FFC 7FFC 7FFC 7FFC 7FFC"
};

data 'ics4' (TK_APP_RESOURCES + 1, "Tk Doc", purgeable) {
	$"0FFF FFFF FFFF 0000 0F33 3333 53F2 F000"
	$"0F33 3335 52FF FF00 0F33 33E6 3333 3F00"
	$"0F33 3256 6333 3F00 0F33 3556 6333 3F00"
	$"0F33 3A5E 3333 3F00 0F33 65D6 D333 3F00"
	$"0F33 3655 5333 3F00 0F33 65C6 3333 3F00"
	$"0F33 3EC5 E333 3F00 0F33 36C6 3333 3F00"
	$"0F33 33CD 3333 3F00 0F33 33C3 3333 3F00"
	$"0F33 33C3 3333 3F00 0FFF FFFF FFFF FF00"
};

/* 
 * The following resources define the icons for the Wish
 * application.
 */

data 'icl4' (TK_APP_RESOURCES, "Tk App", purgeable) {
	$"0000 0000 0000 000F 0000 0000 0000 0000"
	$"0000 0000 0000 00FC F000 0000 0000 0000"
	$"0000 0000 0000 0FCC CF66 0000 0000 0000"
	$"0000 0000 0000 FCCC C556 0000 0000 0000"
	$"0000 0000 000F CCCC 566F 0000 0000 0000"
	$"0000 0000 00FC CCC5 6F5C F000 0000 0000"
	$"0000 0000 0FCC CC66 66CC CF00 0000 0000"
	$"0000 0000 FCCC CCD5 5666 CCF0 0000 0000"
	$"0000 000F CCCC C656 5667 CCCF 0000 0000"
	$"0000 00FC CCCC C6E5 5566 CCCC F000 0000"
	$"0000 0FCC CCCC C656 5657 CCCC CF00 0000"
	$"0000 FCCC CCCC C6E5 565C CCCC CCF0 0000"
	$"000F CCCC CCCC C655 565C CCCC CCCF 0000"
	$"00FC CCCC CCCC 7660 556C CCCC CCCC F000"
	$"0FCC CCCC CCCC CD5D 567C CCCC CCCC CF00"
	$"FCCC CCCC CCCC 6660 556C CCCC CCCC CCF0"
	$"0FCC CCCC CCCC 665C 565C CCCC CCCC C0CF"
	$"00FC CCCC CCCC 6660 E6DC CCCC CCCC CCF0"
	$"000F CCCC CCCC C650 656C CCCC CCCC CF00"
	$"0000 FCCC CCCC C6EC 5ECC CCCC CCCC F000"
	$"0000 0FCC CCCC C650 566C CCCC CCCF 0000"
	$"0000 00FC CCCC CC50 75CC CCCC CCF0 0000"
	$"0000 000F CCCC CC50 56CC CCCC CF00 0000"
	$"0000 0000 FCCC CCC0 5CCC CCCC F000 0000"
	$"0000 0000 0FCC CCC0 CCCC CCCF 0000 0000"
	$"0000 0000 00FC CCC0 CCCC CCF0 0000 0000"
	$"0000 0000 000F CCC0 CCCC CF00 0000 0000"
	$"0000 0000 0000 FCCC CCCC F000 0000 0000"
	$"0000 0000 0000 0FCC CCCF 0000 0000 0000"
	$"0000 0000 0000 00FC CCF0 0000 0000 0000"
	$"0000 0000 0000 000F CF00 0000 0000 0000"
	$"0000 0000 0000 0000 F000 0000 0000 0000"
};

data 'ICN#' (TK_APP_RESOURCES, "Tk App", purgeable) {
	$"0001 0000 0002 8000 0004 7000 0008 7000"
	$"0010 F000 0021 E800 0043 C400 0081 F200"
	$"0107 F100 0207 F080 0407 F040 0807 E020"
	$"1007 E010 200E E008 4002 E004 800E E002"
	$"400E E001 200E C002 1006 E004 0806 C008"
	$"0406 E010 0202 C020 0102 C040 0080 8080"
	$"0041 0100 0021 0200 0011 0400 0009 0800"
	$"0004 1000 0002 2000 0001 4000 0000 8000"
	$"0001 0000 0003 8000 0007 F000 000F F000"
	$"001F F000 003F F800 007F FC00 00FF FE00"
	$"01FF FF00 03FF FF80 07FF FFC0 0FFF FFE0"
	$"1FFF FFF0 3FFF FFF8 7FFF FFFC FFFF FFFE"
	$"7FFF FFFF 3FFF FFFE 1FFF FFFC 0FFF FFF8"
	$"07FF FFF0 03FF FFE0 01FF FFC0 00FF FF80"
	$"007F FF00 003F FE00 001F FC00 000F F800"
	$"0007 F000 0003 E000 0001 C000 0000 8000"
};

data 'ics#' (TK_APP_RESOURCES, "Tk App", purgeable) {
	$"01C0 0260 04E0 09D0 1388 23C4 43C2 8281"
	$"8282 4284 2188 1190 0920 0540 0280 0100"
	$"01C0 03E0 07E0 0FF0 1FF8 3FFC 7FFE FFFF"
	$"FFFE 7FFC 3FF8 1FF0 0FE0 07C0 0380 0100"
};

data 'ics4' (TK_APP_RESOURCES, "Tk App", purgeable) {
	$"0000 000F C000 0000 0000 00FC 6600 0000"
	$"0000 0FCC 6600 0000 0000 FCC6 66F0 0000"
	$"000F CCD5 56CF 0000 00FC CC66 57CC F000"
	$"0FCC CC65 56CC CF00 FCCC CC56 57CC CCF0"
	$"0FCC CCC6 6CCC CCCF 00FC CCC6 5CCC CCF0"
	$"000F CCC6 6CCC CF00 0000 FCCC 5CCC F000"
	$"0000 0FCC CCCF 0000 0000 00FC CCF0 0000"
	$"0000 000F CF00 0000 0000 0000 F000 0000"
};


