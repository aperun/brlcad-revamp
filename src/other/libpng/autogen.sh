#! /bin/sh
#
# Run 'autoreconf' to build 'configure', 'Makefile.in' and other configure
# control files.
#
# The first time this is run on a GIT checkout the only files that exist are
# configure.ac and Makefile.am; all of the autotools support scripts are
# missing.  They are instantiated with autoreconf --force --install.
#
# For regular ("tarball") distributions all the files should exist.  We do not
# want them to be updated *under any circumstances*.  It should never be
# necessary to rune autogen.sh because ./configure --enable-maintainer-mode says
# what to do if Makeile.am or configure.ac are changed.
#
# It is *probably* OK to update the files on a GIT checkout, because they have
# come from the local tools, but leave that to the user who is assumed to know
# whether it is ok or required.
#
# This script is intended to work without arguments, there are, however, hidden
# arguments for (a) use while testing the script and (b) to fix up systems that
# have been broken.  If (b) is required the script prompts for the correct
# options.  For this reason the options are *NOT* documented in the help; this
# is deliberate; UTSL.
#
clean=
maintainer=
while test $# -gt 0
do
   case "$1" in
      --maintainer)
         maintainer=1;;

      --clean)
         clean=1;;

      *)
         exec >&2
         echo "$0: usage: ./autogen.sh"
         if test -d .git
         then
            echo "  ./autogen.sh generates the configure script and"
            echo "  Makefile.in, or refreshes them after changes to Makefile.am"
            echo "  or configure.ac.  You may prefer to just run autoreconf."
         elif test -z "$maintainer"
         then
            echo "  DO NOT RUN THIS SCRIPT."
            echo "  If you need to change Makefile.am or configure.ac then you"
            echo "  also need to run ./configure --enable-maintainer-mode and"
            echo "  use the appropriate autotools, *NOT* this script, to update"
            echo "  everything, please check the documentation of autoreconf."
            echo "  WARNING: libpng is intentionally generated with a known,"
            echo "  fixed, set of autotools.  It is known *NOT* to work with"
            echo "  the collection of autotools distributed on highly reputable"
            echo "  operating systems."
            echo "  Remember: autotools is GNU software, you are expected to"
            echo "  pay for support."
         else
            echo "  You have run autogen.sh with --maintainer enabled and you"
            echo "  are not using a GIT distribution, then you have given an"
            echo "  unrecognized argument.  This is not good. --maintainer"
            echo "  switches off any assumptions that you might not know what"
            echo "  you are doing."
         fi
         exit 1;;
   esac

   shift
done
#
# First check for a set of the autotools files; if absent then this is assumed
# to be a GIT version and the local autotools must be used.  If present this
# is a tarball distribution and the script should not be used.  If partially
# present bad things are happening.
#
# The autotools generated files:
libpng_autotools_files="Makefile.in aclocal.m4 config.guess config.h.in\
   config.sub configure depcomp install-sh ltmain.sh missing"
#
# These are separate because 'maintainer-clean' does not remove them.
libpng_libtool_files="scripts/libtool.m4 scripts/ltoptions.m4\
   scripts/ltsugar.m4 scripts/ltversion.m4 scripts/lt~obsolete.m4"

libpng_autotools_dirs="autom4te.cache" # not required
#
# The configure generated files:
libpng_configure_files="Makefile config.h config.log config.status\
   libpng-config libpng.pc libtool stamp-h1"

libpng_configure_dirs=".deps"
#
# We must remove the configure generated files as well as the autotools
# generated files if autotools are regenerated because otherwise if configure
# has been run without "--enable-maintainer-mode" make can do a partial update
# of Makefile.  These functions do the two bits of cleaning.
clean_autotools(){
   rm -rf $libpng_autotools_files $libpng_libtool_files $libpng_autotools_dirs
}

clean_configure(){
   rm -rf $libpng_configure_files $libpng_configure_dirs
}
#
# Clean: remove everything (this is to help with testing)
if test -n "$clean"
then
   clean_configure
   if test -n "$maintainer"
   then
      clean_autotools
   fi

   exit 0
fi
#
# Validate the distribution.
libpng_autotools_file_found=
libpng_autotools_file_missing=
for file in $libpng_autotools_files
do
   if test -f  "$file"
   then
      libpng_autotools_file_found=1
   else
      libpng_autotools_file_missing=1
   fi
done
#
# Presence of one of these does not *invalidate* missing, but absence
# invalidates found.
for file in $libpng_libtool_files
do
   if test ! -f "$file"
   then
      libpng_autotools_file_missing=1
   fi
done
#
# The cache directory doesn't matter - it will be regenerated and does not exist
# anyway in a tarball.
#
# Either everything is missing or everything is there, the --maintainer option
# just changes this so that the mode is set to generate all the files.
mode=
if test -z "$libpng_autotools_file_found" -o -n "$maintainer"
then
   mode="autoreconf"
else
   if test -n "$libpng_autotools_file_missing"
   then
      mode="broken"
   else
      mode="configure"
   fi
fi
#
# So:
case "$mode" in
   autoreconf)
      # Clean in case configure files exist
      clean_configure
      clean_autotools
      # Everything must be initialized, so use --force
      if autoreconf --warnings=all --force --install
      then
         missing=
         for file in $libpng_autotools_files
         do
            test -f "$file" || missing=1
         done
         # ignore the cache directory
         test -z "$missing" || {
            exec >&2
            echo "autoreconf was run, but did not produce all the expected"
            echo "files.  It is likely that your autotools installation is"
            echo "not compatible with that expected by libpng."
            exit 1
         }
      else
         exec >&2
         echo "autoreconf failed: your version of autotools is incompatible"
         echo "with this libpng version.  Please use a distributed archive"
         echo "(which includes the autotools generated files) and run configure"
         echo "instead."
         exit 1
      fi;;

   configure)
      if test -d .git
      then
         exec >&2
         echo "ERROR: running autoreconf on an initialized sytem"
         echo "  This is not necessary; it is only necessary to remake the"
         echo "  autotools generated files if Makefile.am or configure.ac"
         echo "  change and make does the right thing with:"
         echo
         echo "     ./configure --enable-maintainer-mode."
         echo
         echo "  You can run autoreconf yourself if you don't like maintainer"
         echo "  mode and you can also just run autoreconf -f -i to initialize"
         echo "  everything in the first place; this script is only for"
         echo "  compatiblity with prior releases."
         exit 1
      else
         exec >&2
         echo "autogen.sh is intended only to generate 'configure' on systems"
         echo "that do not have it.  You have a complete 'configure', if you"
         echo "need to change Makefile.am or configure.ac you also need to"
         echo "run configure with the --enable-maintainer-mode option."
         exit 1
      fi;;

   broken)
      exec >&2
      echo "Your system has a partial set of autotools generated files."
      echo "autogen.sh is unable to proceed.  The full set of files is"
      echo "contained in the libpng 'tar' distribution archive and you do"
      echo "not need to run autogen.sh if you use it."
      exit 1;;
esac
