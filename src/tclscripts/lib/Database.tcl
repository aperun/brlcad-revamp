#                    D A T A B A S E . T C L
# BRL-CAD
#
# Copyright (c) 1998-2004 United States Government as represented by
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
#
# Author -
#	Bob Parker
#
# Source -
#	The U. S. Army Research Laboratory
#	Aberdeen Proving Ground, Maryland  21005
#
# Distribution Notice -
#	Re-distribution of this software is restricted, as described in
#       your "Statement of Terms and Conditions for the Release of
#       The BRL-CAD Package" agreement.
#
# Copyright Notice -
#       This software is Copyright (C) 1998-2004 by the United States Army
#       in all countries except the USA.  All rights reserved.
#
# Description -
#	The Database class inherits from Db and Drawable.
#
::itcl::class Database {
    inherit Db Drawable

    constructor {file} {
	Db::constructor $file
	Drawable::constructor [Db::get_dbname]
    } {}

    destructor {}

    public method ? {}
    public method apropos {key}
    public method help {args}
    public method getUserCmds {}
}

::itcl::body Database::? {} {
    return "[Db::?]\n[Drawable::?]"
}

::itcl::body Database::apropos {key} {
    return "[Db::apropos $key] [Drawable::apropos $key]"
}

::itcl::body Database::help {args} {
    if {[llength $args] && [lindex $args 0] != {}} {
	if {[catch {eval Db::help $args} result]} {
	    set result [eval Drawable::help $args]
	}

	return $result
    }

    # list all help messages for Db and Drawable
    return "[Db::help][Drawable::help]"
}

::itcl::body Database::getUserCmds {} {
    return "[Db::getUserCmds] [Drawable::getUserCmds]"
}

# Local Variables:
# mode: Tcl
# tab-width: 8
# c-basic-offset: 4
# tcl-indent-level: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
