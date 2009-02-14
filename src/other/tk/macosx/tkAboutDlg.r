/*
 * tkAboutDlg.r --
 *
 *	This file creates resources for the Tk "About Box" dialog.
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 * Copyright (c) 2006-2008 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

/*
 * The folowing include and defines help construct
 * the version string for Tcl.
 */

#define RC_INVOKED
#include <Carbon.r>
#include <tcl.h>
#include "tk.h"

/*
 * The following two resources define the default "About Box" for Mac Tk.
 * This dialog appears if the "About Tk..." menu item is selected from
 * the Apple menu. This dialog may be overridden by defining a Tcl procedure
 * with the name of "tkAboutDialog". If this procedure is defined the
 * default dialog will not be shown and the Tcl procedure is expected to
 * create and manage an About Dialog box.
 */

resource 'DLOG' (128, "About Box", purgeable) {
    {60, 40, 332, 404},
    kWindowMovableModalDialogProc,
    visible,
    noGoAway,
    0x0,
    128,
    "About Tcl & Tk",
    centerMainScreen
};

resource 'DITL' (128, "About Box", purgeable) {
    {
	{232, 147, 252, 217}, Button	    {enabled, "Ok"},
	{ 20, 108, 212, 344}, StaticText    {disabled,
	    "Tcl " TCL_PATCH_LEVEL " & Tk " TK_PATCH_LEVEL "\n\n"
	    "� 2002-2008 Tcl Core Team." "\n\n"
	    "� 2002-2008 Daniel A. Steffen." "\n\n"
	    "Jim Ingham & Ian Reid" "\n"
	    "� 2001-2002 Apple Computer, Inc." "\n\n"
	    "Jim Ingham & Ray Johnson" "\n"
	    "� 1998-2000 Scriptics Inc." "\n"
	    "� 1996-1997 Sun Microsystems Inc."},
	{ 20,  24, 120, 92}, Picture  {enabled, 128}
    }
};

resource 'dlgx' (128, "About Box", purgeable) {
    versionZero {
	kDialogFlagsUseThemeBackground | kDialogFlagsUseControlHierarchy
	| kDialogFlagsHandleMovableModal | kDialogFlagsUseThemeControls
    }
};

data 'PICT' (128, purgeable) {
	$"13A4 0000 0000 0064 0044 0011 02FF 0C00"
	$"FFFE 0000 0048 0000 0048 0000 0000 0000"
	$"0064 0044 0000 0000 0001 000A 0000 0000"
	$"0064 0044 0099 8044 0000 0000 0064 0044"
	$"0000 0000 0000 0000 0048 0000 0048 0000"
	$"0000 0008 0001 0008 0000 0000 0108 00D8"
	$"0000 0000 0001 5A5A 8000 00FF 3736 FF00"
	$"FF00 FF00 3535 FF00 FF00 CC00 3434 FF00"
	$"FF00 9900 3333 FF00 FF00 6600 3736 FF00"
	$"FF00 3300 3535 FF00 FF00 0000 3434 FF00"
	$"CC00 FF00 3333 FF00 CC00 CC00 3736 FF00"
	$"CC00 9900 3535 FF00 CC00 6600 FAFA FF00"
	$"CC00 3300 3333 FF00 CC00 0000 3130 FF00"
	$"9900 FF00 2F2F FF00 9900 CC00 FAFA FF00"
	$"9900 9900 F9F9 FF00 9900 6600 3130 FF00"
	$"9900 3300 2F2F FF00 9900 0000 2E2E FF00"
	$"6600 FF00 F9F9 FF00 6600 CC00 3130 FF00"
	$"6600 9900 2F2F FF00 6600 6600 2E2E FF00"
	$"6600 3300 2D2D FF00 6600 0000 3130 FF00"
	$"3300 FF00 2F2F FF00 3300 CC00 2E2E FF00"
	$"3300 9900 2D2D FF00 3300 6600 3130 FF00"
	$"3300 3300 2F2F FF00 3300 0000 2E2E FF00"
	$"0000 FF00 2D2D FF00 0000 CC00 3130 FF00"
	$"0000 9900 2F2F FF00 0000 6600 2E2E FF00"
	$"0000 3300 2DF8 FF00 0000 0000 2B2A CC00"
	$"FF00 FF00 2929 CC00 FF00 CC00 2828 CC00"
	$"FF00 9900 27F8 CC00 FF00 6600 2B2A CC00"
	$"FF00 3300 2929 CC00 FF00 0000 2828 CC00"
	$"CC00 FF00 2727 CC00 CC00 CC00 2B2A CC00"
	$"CC00 9900 2929 CC00 CC00 6600 2828 CC00"
	$"CC00 3300 2727 CC00 CC00 0000 2B2A CC00"
	$"9900 FF00 2929 CC00 9900 CC00 2828 CC00"
	$"9900 9900 2727 CC00 9900 6600 DBDB CC00"
	$"9900 3300 4747 CC00 9900 0000 4646 CC00"
	$"6600 FF00 4545 CC00 6600 CC00 DBDB CC00"
	$"6600 9900 4747 CC00 6600 6600 4646 CC00"
	$"6600 3300 4545 CC00 6600 0000 DBDB CC00"
	$"3300 FF00 4747 CC00 3300 CC00 4646 CC00"
	$"3300 9900 4545 CC00 3300 6600 DBDB CC00"
	$"3300 3300 4141 CC00 3300 0000 4040 CC00"
	$"0000 FF00 3F3F CC00 0000 CC00 4342 CC00"
	$"0000 9900 4141 CC00 0000 6600 4040 CC00"
	$"0000 3300 3F3F CC00 0000 0000 4342 9900"
	$"FF00 FF00 4141 9900 FF00 CC00 4040 9900"
	$"FF00 9900 3F3F 9900 FF00 6600 4342 9900"
	$"FF00 3300 4141 9900 FF00 0000 4040 9900"
	$"CC00 FF00 3F3F 9900 CC00 CC00 4342 9900"
	$"CC00 9900 4141 9900 CC00 6600 4040 9900"
	$"CC00 3300 3F3F 9900 CC00 0000 4342 9900"
	$"9900 FF00 4141 9900 9900 CC00 4040 9900"
	$"9900 9900 3F3F 9900 9900 6600 3D3C 9900"
	$"9900 3300 3B3B 9900 9900 0000 3A3A 9900"
	$"6600 FF00 3939 9900 6600 CC00 3D3C 9900"
	$"6600 9900 3B3B 9900 6600 6600 3A3A 9900"
	$"6600 3300 3939 9900 6600 0000 3D3C 9900"
	$"3300 FF00 3B3B 9900 3300 CC00 3A3A 9900"
	$"3300 9900 3939 9900 3300 6600 3D3C 9900"
	$"3300 3300 3B3B 9900 3300 0000 3A3A 9900"
	$"0000 FF00 3939 9900 0000 CC00 3D3C 9900"
	$"0000 9900 3B3B 9900 0000 6600 3A3A 9900"
	$"0000 3300 3939 9900 0000 0000 3D3C 6600"
	$"FF00 FF00 3B3B 6600 FF00 CC00 3A3A 6600"
	$"FF00 9900 3939 6600 FF00 6600 3D3C 6600"
	$"FF00 3300 3B3B 6600 FF00 0000 3A3A 6600"
	$"CC00 FF00 3939 6600 CC00 CC00 3736 6600"
	$"CC00 9900 3535 6600 CC00 6600 3434 6600"
	$"CC00 3300 3333 6600 CC00 0000 3736 6600"
	$"9900 FF00 3535 6600 9900 CC00 3434 6600"
	$"9900 9900 3333 6600 9900 6600 3736 6600"
	$"9900 3300 3535 6600 9900 0000 3434 6600"
	$"6600 FF00 3333 6600 6600 CC00 3736 6600"
	$"6600 9900 3535 6600 6600 6600 3434 6600"
	$"6600 3300 3333 6600 6600 0000 3736 6600"
	$"3300 FF00 3535 6600 3300 CC00 3434 6600"
	$"3300 9900 3333 6600 3300 6600 3736 6600"
	$"3300 3300 3535 6600 3300 0000 3434 6600"
	$"0000 FF00 3333 6600 0000 CC00 3130 6600"
	$"0000 9900 2F2F 6600 0000 6600 2E2E 6600"
	$"0000 3300 F9F9 6600 0000 0000 3130 3300"
	$"FF00 FF00 2F2F 3300 FF00 CC00 2E2E 3300"
	$"FF00 9900 F9F9 3300 FF00 6600 3130 3300"
	$"FF00 3300 2F2F 3300 FF00 0000 2E2E 3300"
	$"CC00 FF00 2D2D 3300 CC00 CC00 3130 3300"
	$"CC00 9900 2F2F 3300 CC00 6600 2E2E 3300"
	$"CC00 3300 2D2D 3300 CC00 0000 3130 3300"
	$"9900 FF00 2F2F 3300 9900 CC00 2E2E 3300"
	$"9900 9900 2D2D 3300 9900 6600 3130 3300"
	$"9900 3300 2F2F 3300 9900 0000 2E2E 3300"
	$"6600 FF00 2DF8 3300 6600 CC00 2B2A 3300"
	$"6600 9900 2929 3300 6600 6600 2828 3300"
	$"6600 3300 27F8 3300 6600 0000 2B2A 3300"
	$"3300 FF00 2929 3300 3300 CC00 2828 3300"
	$"3300 9900 2727 3300 3300 6600 2B2A 3300"
	$"3300 3300 2929 3300 3300 0000 2828 3300"
	$"0000 FF00 2727 3300 0000 CC00 2B2A 3300"
	$"0000 9900 2929 3300 0000 6600 2828 3300"
	$"0000 3300 2727 3300 0000 0000 4948 0000"
	$"FF00 FF00 4747 0000 FF00 CC00 4646 0000"
	$"FF00 9900 4545 0000 FF00 6600 4948 0000"
	$"FF00 3300 4747 0000 FF00 0000 4646 0000"
	$"CC00 FF00 4545 0000 CC00 CC00 4948 0000"
	$"CC00 9900 4747 0000 CC00 6600 4646 0000"
	$"CC00 3300 4545 0000 CC00 0000 4342 0000"
	$"9900 FF00 4141 0000 9900 CC00 4040 0000"
	$"9900 9900 3F3F 0000 9900 6600 4342 0000"
	$"9900 3300 4141 0000 9900 0000 4040 0000"
	$"6600 FF00 3F3F 0000 6600 CC00 4342 0000"
	$"6600 9900 4141 0000 6600 6600 4040 0000"
	$"6600 3300 3F3F 0000 6600 0000 4342 0000"
	$"3300 FF00 4141 0000 3300 CC00 4040 0000"
	$"3300 9900 3F3F 0000 3300 6600 4342 0000"
	$"3300 3300 4141 0000 3300 0000 4040 0000"
	$"0000 FF00 3F3F 0000 0000 CC00 4342 0000"
	$"0000 9900 4141 0000 0000 6600 4040 0000"
	$"0000 3300 3F3F EE00 0000 0000 3D3C DD00"
	$"0000 0000 3B3B BB00 0000 0000 3A3A AA00"
	$"0000 0000 3939 8800 0000 0000 3D3C 7700"
	$"0000 0000 3B3B 5500 0000 0000 3A3A 4400"
	$"0000 0000 3939 2200 0000 0000 3D3C 1100"
	$"0000 0000 3B3B 0000 EE00 0000 3A3A 0000"
	$"DD00 0000 3939 0000 BB00 0000 3D3C 0000"
	$"AA00 0000 3B3B 0000 8800 0000 3A3A 0000"
	$"7700 0000 3939 0000 5500 0000 3D3C 0000"
	$"4400 0000 3B3B 0000 2200 0000 3A3A 0000"
	$"1100 0000 3939 0000 0000 EE00 3D3C 0000"
	$"0000 DD00 3B3B 0000 0000 BB00 3A3A 0000"
	$"0000 AA00 3939 0000 0000 8800 3D3C 0000"
	$"0000 7700 3B3B 0000 0000 5500 3A3A 0000"
	$"0000 4400 3939 0000 0000 2200 3736 0000"
	$"0000 1100 3535 EE00 EE00 EE00 3434 DD00"
	$"DD00 DD00 3333 BB00 BB00 BB00 3736 AA00"
	$"AA00 AA00 3535 8800 8800 8800 3434 7700"
	$"7700 7700 3333 5500 5500 5500 3736 4400"
	$"4400 4400 3535 2200 2200 2200 3434 1100"
	$"1100 1100 3333 0000 0000 0000 0000 0000"
	$"0064 0044 0000 0000 0064 0044 0000 000A"
	$"0000 0000 0064 0044 02BD 0013 E800 01F5"
	$"F6FE 07FE 0E02 3232 33FD 3900 0EE6 001D"
	$"FC00 01F5 F5FE 0700 08FE 0E02 3232 33FE"
	$"3900 3AFC 40F2 4102 4033 07E9 0017 0100"
	$"0EFC 40DC 4102 390E F5F5 0002 F5F5 F6FE"
	$"0702 0E07 0016 0100 32D5 4104 4039 0E32"
	$"33FD 3900 3AFC 40FC 4101 3200 0801 000E"
	$"C141 010E 0008 0100 0EC1 4101 0800 0801"
	$"000E C141 0107 0008 0100 0EC1 4101 0700"
	$"0901 0007 C241 0240 F500 0E01 0007 E841"
	$"0147 47DD 4102 4000 0012 0100 07F0 4100"
	$"47FA 4101 3B3B DD41 0240 0000 1901 0007"
	$"F141 0C47 3B0B 3B47 4141 4711 0505 3B47"
	$"DF41 023A 0000 1701 00F6 F041 010B 0BFE"
	$"4105 473B 0505 113B DE41 0239 0000 1A02"
	$"00F5 40F3 410C 473B 053B 4741 4741 0B0B"
	$"3B47 47DE 4102 3900 0018 0200 F540 F341"
	$"0247 110B FE41 0447 1105 4147 DC41 0233"
	$"0000 1B02 0000 40F3 4103 4711 1147 FE41"
	$"0205 3547 F741 FD47 E941 0232 0000 1E02"
	$"0000 40F2 4106 113B 4741 4735 0BF7 4106"
	$"4741 390E 0E40 47EA 4102 0E00 0021 0200"
	$"0040 F241 0711 3B47 4141 0B35 47F9 4102"
	$"4740 07FE 0002 F640 47EB 4102 0E00 0023"
	$"0200 0040 F341 0847 3541 4147 3B05 4147"
	$"FA41 0947 3AF6 00F5 4F55 F50E 47EB 4102"
	$"0700 0022 0200 003A F341 0147 3BFE 4101"
	$"0B0B F941 0547 3AF5 0055 C8FE CE01 5640"
	$"EB41 0207 0000 1F02 0000 39F0 4104 4741"
	$"053B 47FB 4104 4740 F5F5 A4FC CE01 C85D"
	$"EB41 02F6 0000 1F02 0000 39F0 4104 473B"
	$"0541 47FC 4104 4740 07F6 C8FA CE00 64EC"
	$"4103 40F5 0000 1C02 0000 39F0 4102 4711"
	$"0BFA 4103 4708 2AC8 FACE 0164 D8EC 4100"
	$"40FE 0025 0200 0039 EF41 020B 3B47 FC41"
	$"0347 0FF5 A4FB CE02 C887 D8FC 41FE 47FC"
	$"4100 47F9 4100 3AFE 0028 0200 0039 EF41"
	$"020B 3B47 FD41 0347 3900 A4FA CE00 ABFA"
	$"4109 3B11 3B41 4147 3B0B 3B47 FA41 0039"
	$"FE00 2402 0000 33F1 4102 4741 0BFA 4101"
	$"0779 F9CE 0064 FA41 0235 050B FD41 010B"
	$"0BF9 4100 39FE 0028 0200 0032 F141 0247"
	$"3B0B FC41 0247 39F6 F9CE 0187 D8FB 4103"
	$"4741 050B FE41 0247 110B F941 0039 FE00"
	$"2C02 0000 32F1 4102 473B 11FB 4101 0879"
	$"FACE 05AA 4041 4147 47FE 410A 4741 0511"
	$"4741 4147 3511 47FA 4100 32FE 002F 0200"
	$"000E F141 0347 3B11 47FE 4103 4740 F6C8"
	$"FACE 0564 D841 4039 39FE 4104 473B 053B"
	$"47FE 4102 3541 47FA 4100 0EFE 0027 0200"
	$"000E F141 0347 3B3B 47FE 4102 470F 79FA"
	$"CE0C 8741 4032 F500 003A 4741 473B 05F2"
	$"4100 0EFE 0027 0200 000E F141 0347 3B3B"
	$"47FD 4101 0EA4 FACE 01AB AAFE C808 7900"
	$"3947 4147 110B 47F3 4100 07FE 001C 0200"
	$"000E EA41 0240 2BC8 F5CE 0881 0033 4741"
	$"410B 3B47 F341 0007 FE00 1A02 0000 08EB"
	$"4102 473A 55F4 CE06 5D00 3947 4741 0BF1"
	$"4100 F6FE 001C 0200 0007 EB41 0247 3979"
	$"F4CE 0739 0039 4747 3511 47F3 4101 40F5"
	$"FE00 1C02 0000 07EB 4102 4739 A4F5 CE08"
	$"AB0E 0040 4741 1141 47F3 4100 40FD 001B"
	$"0200 0007 EB41 0247 39A4 F5CE 0787 0707"
	$"4147 4111 47F2 4100 40FD 001B 0200 0007"
	$"EB41 0247 39C8 F5CE 0763 F532 4747 3B3B"
	$"47F2 4100 3AFD 001A 0300 00F6 40EC 4102"
	$"4739 C8F5 CE05 39F5 4047 413B F041 0039"
	$"FD00 1C03 0000 F540 EB41 0140 C8FD CE01"
	$"C8A4 FCCE 03AB 080E 47ED 4100 39FD 001A"
	$"FE00 0040 EB41 0040 FCCE 01A4 C8FC CE03"
	$"FA07 4047 ED41 0032 FD00 1AFE 0000 40EA"
	$"4100 AAFE CE02 87F9 C8FC CE02 560F 47EC"
	$"4100 32FD 0019 FE00 0040 EA41 00AB FECE"
	$"0264 56C8 FDCE 01C8 32EA 4100 0EFD 001B"
	$"FE00 0040 ED41 030E 4047 87FE CE01 4055"
	$"FCCE 01FA 40EA 4100 08FD 001A FE00 003A"
	$"ED41 0807 0740 FBCE CEAB 3979 FDCE 00AB"
	$"E841 0007 FD00 1CFE 0000 3AED 4108 0700"
	$"F6A4 CECE 8733 79FD CE02 4147 47EA 4100"
	$"07FD 001E FE00 0039 ED41 0807 2AA4 C8CE"
	$"CE88 0E9D FECE 0364 1C39 39EB 4101 40F5"
	$"FD00 1CFE 0000 39ED 4101 074F FDCE 0264"
	$"F7A4 FECE 03AB 80F6 07EB 4100 40FC 001C"
	$"FE00 0039 ED41 0108 79FE CE03 AB40 2BA4"
	$"FCCE 02F7 0E47 EC41 0040 FC00 1CFE 0000"
	$"39ED 4101 0879 FECE 03AB 40F6 C8FC CE02"
	$"F615 47EC 4100 40FC 001E FE00 003A EE41"
	$"0247 0E79 FECE 03AB 40F5 C8FD CE03 A4F5"
	$"3A47 EC41 0040 FC00 1EFE 0000 3AEE 4102"
	$"470E 56FE CE03 FB3A F6C8 FDCE 0280 F540"
	$"EB41 0140 F5FD 001E FE00 0040 EE41 0947"
	$"0F56 CECE C888 39F6 C8FD CE02 5601 40EB"
	$"4101 40F5 FD00 1CFE 0000 40EE 4109 4739"
	$"32CE CEC8 8839 2AC8 FDCE 0156 07E9 4100"
	$"F6FD 001B FE00 0040 EE41 0847 3A32 CECE"
	$"C864 152A FCCE 0132 07E9 4100 07FD 001A"
	$"FE00 0040 ED41 0740 32AB CEC8 6439 4EFC"
	$"CE01 3A07 E941 0007 FD00 1D03 0000 F540"
	$"ED41 0740 0EAB CECE 640F 4EFD CE03 AB40"
	$"0840 EA41 0007 FD00 1B03 0000 F540 EC41"
	$"060F 81CE CE64 334E FDCE 02AB 400E E941"
	$"000E FD00 1C02 0000 F6EC 4107 4715 FACE"
	$"CE64 334E FDCE 0387 0F0E 47EA 4100 0EFD"
	$"001C 0200 0007 EC41 0747 16F9 CEC8 6433"
	$"4EFD CE03 6308 4047 EA41 000E FD00 1A02"
	$"0000 07EB 4106 40F9 CEC8 6439 4EFD CE02"
	$"3940 47E9 4100 32FD 001B 0200 0007 EA41"
	$"0539 CECE 8839 F6FE CE04 AB41 4139 40EA"
	$"4100 32FD 001C 0200 0007 EB41 0E47 3AC8"
	$"CE88 39F6 C8CE CE64 15F6 F540 EA41 0033"
	$"FD00 1A02 0000 07EA 410C 40A4 CE87 392A"
	$"C8CE AB41 40F8 F6E9 4100 39FD 001B 0200"
	$"000E EB41 0D47 41AB C887 39F5 C8CE ABAB"
	$"CEA4 07E9 4100 39FD 001C 0200 000E ED41"
	$"0947 3939 4787 C8AB 40F5 C8FD CE01 A40E"
	$"E941 0039 FD00 1D02 0000 0EED 4109 473A"
	$"0007 80CE AB40 F5C8 FDCE 0255 0E47 EA41"
	$"0039 FD00 1B02 0000 0EEB 4107 0779 C8CE"
	$"CE40 F6A4 FDCE 022B 3947 EA41 003A FD00"
	$"1C02 0000 0EEC 4102 4739 79FE CE02 6407"
	$"A4FE CE02 A407 40E9 4100 40FD 001A 0200"
	$"0032 EA41 0632 A4CE CE88 0879 FECE 02F9"
	$"0F47 E941 0040 FD00 1A02 0000 32EB 4107"
	$"4740 F7C8 CE87 0E79 FECE 0132 40E8 4100"
	$"40FD 0019 0200 0033 EA41 0B47 40F8 C8AB"
	$"0E55 CECE 8015 47E8 4100 40FD 0017 0200"
	$"0033 E941 0847 40F9 A439 4FCE CE5D E641"
	$"0140 F5FE 0014 0200 0039 E841 0647 64FB"
	$"392B C8AB E441 00F6 FE00 1102 0000 39E5"
	$"4103 40F6 8764 E441 0007 FE00 1E02 0000"
	$"39EB 4102 3A0E 0EFD 4102 0740 47F6 4104"
	$"400F 0839 47F4 4100 07FE 0027 0200 0039"
	$"FB41 0147 47F2 4102 0800 40FE 4102 0839"
	$"47FC 4101 4747 FC41 0339 0039 47F4 4100"
	$"07FE 0029 0200 0039 FB41 0140 39F3 4109"
	$"470E F540 4141 470E 3347 FC41 0139 3AFD"
	$"4104 4739 0039 47F4 4100 08FE 0036 0200"
	$"003A FC41 0347 0E00 40FC 4102 4741 40FC"
	$"4109 470E F540 4141 4733 0E47 FE41 0447"
	$"4000 0E47 FE41 0447 3900 3941 FE40 F741"
	$"000E FE00 3A02 0000 3AFD 410E 4740 0700"
	$"0E40 4741 4147 390E 390E 40FE 4108 470E"
	$"F540 4141 4739 0EFC 4103 0F00 0739 FE41"
	$"0747 3900 3940 080F 39F7 4100 0EFE 0035"
	$"0200 0040 FB41 020E 0040 FE41 0D47 4000"
	$"3941 0032 4741 4147 0EF5 40FE 4101 4008"
	$"FC41 023A 000E FD41 0547 3900 3939 33F5"
	$"4100 0EFE 0039 0200 0040 FC41 0347 0E00"
	$"40FE 4106 4732 0040 4139 40FE 4103 470E"
	$"F540 FD41 0108 40FE 4104 4740 000E 47FE"
	$"4106 4739 0007 F540 47F6 4100 32FE 003A"
	$"0200 0040 FC41 0C47 0E00 4047 4141 470E"
	$"0040 4747 FD41 0347 0EF5 40FE 410A 470E"
	$"3947 4141 4740 000E 47FE 4107 4739 000E"
	$"0007 4147 F741 0032 FE00 3802 0000 40FC"
	$"4102 470E 00FD 4106 4739 003A 4740 39FE"
	$"4102 470E F5FD 410A 4733 3347 4141 4740"
	$"000E 47FE 4106 4739 0039 3900 0EF6 4100"
	$"33FE 003A 0200 F540 FC41 0447 3200 0E39"
	$"FD41 0B0E 0E40 333A 4741 413A 07F5 39FE"
	$"4102 473A 0EFD 410F 40F5 0733 4041 4140"
	$"0E00 0E40 0700 0E40 F841 0039 FE00 2902"
	$"00F5 40FA 4101 3939 FB41 023A 3A40 FD41"
	$"FD40 FD41 0240 0E40 FD41 0240 3940 FD41"
	$"FA40 F741 0039 FE00 2A01 00F6 F941 0147"
	$"47FB 4101 4747 FB41 0147 47FB 4101 3940"
	$"FD41 0147 47FB 4100 47FE 4100 47F6 4100"
	$"39FE 000D 0100 07E1 4100 40E4 4100 3AFE"
	$"0009 0100 07C3 4100 3AFE 0009 0100 07C3"
	$"4100 40FE 0009 0100 07C3 4100 40FE 0009"
	$"0100 07C3 4100 40FE 000A 0100 0EC3 4103"
	$"40F5 0000 0901 000E C241 02F6 0000 0901"
	$"000E C241 0207 0000 0901 000E C241 0207"
	$"0000 1101 000E ED41 FE40 003A F940 E241"
	$"0207 0000 2B01 0032 F941 FE40 FE39 0632"
	$"0E0E 0707 F6F5 F800 02F5 F5F6 FB07 FB0E"
	$"0332 3233 33FB 3901 3A3A FB40 0207 0000"
	$"0E0A 000E 3939 320E 0E07 07F6 F5C8 0002"
	$"BD00 00FF"
};
