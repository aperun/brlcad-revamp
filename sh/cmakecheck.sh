#!/bin/sh
#                   C M A K E C H E C K . S H
# BRL-CAD
#
# Copyright (c) 2008 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following
# disclaimer in the documentation and/or other materials provided
# with the distribution.
#
# 3. The name of the author may not be used to endorse or promote
# products derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
###
#
# This is a simple helper script to check whether the CMakeLists.txt
# files are in sync with the Makefile.am files.
#
###


for dir in src/libbn src/libbu src/libged src/librt src/libsysv src/libwdb ; do

    if test ! -f $dir/CMakeLists.txt ; then
	echo "ERROR: cannot find $dir/CMakeLists.txt"
	exit 1
    fi

    # get a list of all the source files listed in CMakeLists.txt
    cmfiles="`cat $dir/CMakeLists.txt | sed '/set([A-Z_]*SOURCES/,/)/{s/[[:space:]]*\(.*\)[[:space:]]*/::\1/;}' | grep -v '[)(}{]' | grep '::' | sed 's/:://' | sort | uniq`"

    if test ! -f $dir/Makefile.am ; then
	echo "ERROR: cannot find $dir/Makefile.am"
	exit 1
    fi

    # get a list of all the source files listed in Makefile.am
    amfiles="`cat $dir/Makefile.am | perl -pi -e 's/\\\\\n//g' | grep \"^[a-zA-Z_]*SOURCES\" | sed 's/.*SOURCES[[:space:]]*=[[:space:]]*//;/[{(]/d' | sort | uniq`"

    for file in $amfiles ; do
	result="`echo \"$cmfiles\" | grep $file`"
	if test "x$result" = "x" ; then
	    echo "MISSING from $dir/CMakeLists.txt: $file"
	fi
    done

done



# Local Variables:
# tab-width: 8
# mode: sh
# sh-indentation: 4
# sh-basic-offset: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
