#!/bin/sh
#                         A M I . T C L
# BRL-CAD
#
# Copyright (c) 2004-2006 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this file; see the file named COPYING for more
# information.
#
###
# This is a comment \
/bin/echo "This is not a shell script"
# This is a comment \
exit

# make the tclIndex
foreach arg $argv {
    catch {auto_mkindex $arg *.tcl *.itcl *.itk *.sh}
    puts $arg
}

lappend tclIndex
lappend header

# sort the tclIndex
set fd [open tclIndex]
while {[gets $fd data] >= 0} {
    if {[string compare -length 3 $data "set"] == 0} {
	lappend tclIndex $data
    } else {
	lappend header $data
    }
}
close $fd

# write out the sorted tclIndex
set fd [open tclIndex {WRONLY TRUNC CREAT}]
foreach line $header {
    puts $fd $line
}
foreach line [lsort $tclIndex] {
    puts $fd $line
}
close $fd

# Local Variables:
# mode: Tcl
# tab-width: 8
# c-basic-offset: 4
# tcl-indent-level: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
