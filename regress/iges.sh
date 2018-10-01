#!/bin/sh
#                         I G E S . S H
# BRL-CAD
#
# Copyright (c) 2010-2018 United States Government as represented by
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

# Ensure /bin/sh
export PATH || (echo "This isn't sh."; sh $0 $*; kill $$)

# source common library functionality, setting ARGS, NAME_OF_THIS,
# PATH_TO_THIS, and THIS.
. "$1/regress/library.sh"

if test "x$LOGFILE" = "x" ; then
    LOGFILE=`pwd`/iges.log
    rm -f $LOGFILE
fi
log "=== TESTING iges conversion ==="

MGED="`ensearch mged`"
if test ! -f "$MGED" ; then
    log "Unable to find mged, aborting"
    exit 1
fi

GIGES="`ensearch g-iges`"
if test ! -f "$MGED" ; then
    log "Unable to find g-iges, aborting"
    exit 1
fi

IGESG="`ensearch iges-g`"
if test ! -f "$MGED" ; then
    log "Unable to find iges-g, aborting"
    exit 1
fi


STATUS=0

log "... running mged to create facetized geometry (iges.g)"
$MGED -c >> $LOGFILE 2>&1 <<EOF
opendb iges.g y

units mm
size 1000
make box.s arb8
facetize -n box.nmg box.s
kill box.s
q
EOF

TFILS='iges.log iges.g iges_file.iges iges_stdout_new.g iges_new.g iges_stdout.iges iges_file.iges'
TFILS="$TFILS iges_file2.iges iges_file3.iges iges.m35.asc iges.m35.g"

# .g to iges:
# these two commands should produce almost identical output
rm -f iges.export.iges
run $GIGES -o iges.export.iges iges.g box.nmg
run $GIGES iges.g box.nmg

# convert back to .g
rm -f iges.import.g
run $IGESG -o iges.import.g -p -N box.nmg iges.export.iges

# check round trip back to iges: vertex permutation?
# these two commands should produce almost identical output
rm -f iges.export2.g
run $GIGES -o iges.export2.iges iges.import.g box.nmg
run $GIGES iges.import.g box.nmg

# these two files should be identical
#    iges_file.iges
#    iges_file2.iges

# these two files should be identical
#    iges_stdout.iges
#    iges_stdout2.iges

$IGESG -o iges_stdout_new.g -p iges_stdout.iges 2>> iges.log
if [ $? != 0 ] ; then
    log "...iges-g (2) FAILED"
    STATUS=1
else
    log "...iges-g (2) succeeded"
fi

# check one other TGM known to have a conversion failure which should be graceful
ASC2G="`ensearch asc2g`"
if test ! -f "$ASC2G" ; then
    log "Unable to find asc2g, aborting"
    exit 1
fi
GZIP="`which gzip`"
if test ! -f "$GZIP" ; then
    log "Unable to find gzip, aborting"
    exit 1
fi

# make our starting database
$GZIP -d -c "$PATH_TO_THIS/tgms/m35.asc.gz" > iges.m35.asc
$ASC2G iges.m35.asc iges.m35.g

# and test it (note it should work with the '-f' option, but fail
# without any options)
$GIGES -f -o iges_file3.iges iges.m35.g r516 2>> iges.log > /dev/null
STATUS=$?


if [ X$STATUS = X0 ] ; then
    log "-> iges.sh succeeded"
else
    log "-> iges.sh FAILED, see $LOGFILE"
fi

exit $STATUS

# Local Variables:
# mode: sh
# tab-width: 8
# sh-indentation: 4
# sh-basic-offset: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
