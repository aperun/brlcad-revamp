#!/bin/sh
#			S E T U P . S H
#
# This shell script is to be run as the very first step in installing
# the BRL CAD Package
# Secret option:  -f, (fast) to skip re-compiling Cake.
#
#  $Header$

# Ensure that all subordinate scripts run with the Bourne shell,
# rather than the user's shell
SHELL=/bin/sh
export SHELL

# Confirm that the installation directory is correct,
# or change it in:
# Cakefile.defs, cake/Makefile, cakeaux/Makefile, setup.sh, cray.sh
# This is unfortunate, and needs to come from some file, somehow.

BINDIR=/usr/brlcad/bin

echo "  BINDIR = ${BINDIR}"

#
# Sanity check
# Make sure that BINDIR is in the current user's search path
# For this purpose, specifically exclude "dot" from the check.
#
PATH_ELEMENTS=`echo $PATH | sed 's/^://
				s/:://g
				s/:$//
				s/:\\.:/:/g
				s/:/ /g'`

not_found=1		# Assume cmd not found
for PREFIX in ${PATH_ELEMENTS}
do
	if test ${PREFIX} = ${BINDIR}
	then
		# This was -x, but older BSD systems don't do -x.
		if test -d ${PREFIX}
		then
			# all is well
			not_found=0
			break
		fi
		echo "$0 WARNING:  ${PREFIX} is in the search path, but is not a directory."
	fi
done
if test ${not_found} -ne 0
then
	echo "$0 ERROR:  ${BINDIR} (BINDIR) is not in your Shell search path!"
	echo "$0 ERROR:  Software setup can not proceed until this has been fixed."
	echo "$0 ERROR:  Consult installation directions for more information,"
	echo "$0 ERROR:  file: install.doc, section INSTALLATION DIRECTORIES."
	exit 1		# Die
fi

# Acquire current machine type, etc.
eval `sh machinetype.sh -b`

#  Ensure that machinetype.sh and Cakefile.defs are set up the same way.
#  This is just a double-check on people porting to new machines.
FILE=/tmp/cadsetup$$
trap '/bin/rm -f ${FILE}; exit 1' 1 2 3 15	# Clean up temp file

/lib/cpp -DDEFINES_ONLY << EOF > ${FILE}
#line 1 "$0"
#include "Cakefile.defs"

C_MACHINE=MTYPE;
#if UNIXTYPE == BSD
	C_UNIXTYPE="BSD";
#else
	C_UNIXTYPE="SYSV";
#endif
C_HAS_TCP=HAS_TCP;
EOF

# Note that we depend on CPP's "#line" messages to be ignored as comments
# when sourced by the "." command here:
. ${FILE}
/bin/rm -f ${FILE}

# See if things match up
if test x${MACHINE} != x${C_MACHINE} -o \
	x${UNIXTYPE} != x${C_UNIXTYPE} -o \
	x${HAS_TCP} != x${C_HAS_TCP}
then
	echo "$0 ERROR:  Mis-match between machinetype.sh and Cakefile.defs"
	echo "$0 ERROR:  machinetype.sh claims ${MACHINE} ${UNIXTYPE} ${HAS_TCP}"
	echo "$0 ERROR:  Cakefile.defs claims ${C_MACHINE} ${C_UNIXTYPE} ${C_HAS_TCP}"
	exit 1		# Die
fi

# Install the necessary shell scripts in BINDIR

SCRIPTS="machinetype.sh cakeinclude.sh cray.sh"

for i in ${SCRIPTS}
do
	if test -f ${BINDIR}/${i}
	then
		mv -f ${BINDIR}/${i} ${BINDIR}/`basename ${i} .sh`.bak
	fi
	cp ${i} ${BINDIR}/.
done

if test x$1 != x-f
then
	# Make and install "cake" and "cakeaux"

	cd cake
	make clobber
	make install
	make clobber

	cd ../cakeaux
	make clobber
	make cakesub			# XXX, should do "install"
	if test -f ${BINDIR}/cakesub
	then
		mv -f ${BINDIR}/cakesub ${BINDIR}/cakesub.bak
	fi
	cp cakesub ${BINDIR}/.
	make clobber
	cd ..
fi

#
# Handle any vendor-specific configurations and setups.
# This is messy, and should be avoided where possible.
chmod 664 Cakefile.defs

case X$MACHINE in

X4gt|X4d|X4d2)
	# SGI 4D foolishness
	case $MACHINE in
	4gt)	SGI_TYPE=1;;
	4d)	SGI_TYPE=2;;
	4d2)	SGI_TYPE=3;;
	esac
	ed - Cakefile.defs << EOF
g/define	SGI_TYPE/d
/SGI_4D_TYPE_MARKER/a
#	define	SGI_TYPE	$SGI_TYPE
.
w
EOF
#	End of SGI stuff

esac
