#			cad/Makefile
# The Master Makefile for the BRL CAD Software Distribution
#
# Source -
#	SECAD/VLD Computing Consortium, Bldg 394
#	The U. S. Army Ballistic Research Laboratory
#	Aberdeen Proving Ground, Maryland  21005-5066

DISTDIR		= /m/dist/.
FTPDIR		= /usr/spool/ftp/arch
ARCHDIR		= /m/.

# NOTE:  The following directories contain code which may not compile
# on all systems:
#	libpkg		BSD or SGI networking, remove if SYSV
#	rfbd		BSD or SGI networking, remove if SYSV
#	fbed		new, BSD or SGI only for now
#
DIRS		= h \
		  doc \
		  libsysv \
		  conv bench \
		  db pix \
		  libpkg \
		  libfb \
		  rfbd \
		  libtermio \
		  libplot3 \
		  librle \
		  librt rt \
		  mged \
		  util \
		  fbed \
		  libcursor \
		  lgt \
		  vdeck

all:
	@echo "Hopefully, all the Makefile.loc parameters are right"

	-@for dir in ${DIRS}; \
	do	echo " "; \
		echo ---------- $$dir; \
		(cd $$dir; make depend; make -k )\
	done

benchmark:
	cd libsysv; make depend; make -k
	cd conv; make -k
	cd db; make -k
	cd pix; make -k
	cd libpkg; make depend; make -k	  # needed for IF_REMOTE, rm if SYSV
	cd libfb; make depend; make -k
	cd librt; make depend; make -k
	cd rt; make depend; make -k

depend:
	-for dir in ${DIRS}; \
	do	echo ---------- $$dir; \
		(cd $$dir; make -k depend) \
	done

install:
	-for dir in ${DIRS}; \
	do	echo ---------- $$dir; \
		(cd $$dir; make -k install) \
	done

inst-man:
	-for dir in ${DIRS}; \
	do	echo ---------- $$dir; \
		(cd $$dir; make -k inst-man) \
	done

uninstall:
	-for dir in ${DIRS}; \
	do	echo ---------- $$dir; \
		(cd $$dir; make -k uninstall) \
	done

print:
	-for dir in ${DIRS}; \
	do	echo ---------- $$dir; \
		(cd $$dir; make -k print) \
	done

typeset:
	-for dir in ${DIRS}; \
	do	echo ---------- $$dir; \
		(cd $$dir; make -k typeset) \
	done

# Temporary hack for Mike, dont use!
inst-dist:
	-for dir in ${DIRS}; \
	do	echo ---------- $$dir; \
		(cd $$dir; make -k inst-dist) \
	done

clean:
	-for dir in ${DIRS}; \
	do	echo ---------- $$dir; \
		(cd $$dir; make -k clean) \
	done

clobber:
	-for dir in ${DIRS}; \
	do	echo ---------- $$dir; \
		(cd $$dir; make -k clobber) \
	done

# Do a "make install" as step 1 of making a distribution.
#
# Use "make dist" as step 2 of making a distribution.
dist:
	cp Copyright* README Makefile* cray-ar.sh ${DISTDIR}
	(cd bench; make clobber; make install)
	cd ${DISTDIR}; du -a > Contents

# Use as step 3 of making a distribution -- write the tape and/or FTP archive
tape0:
	cd ${DISTDIR}; tar cvf /dev/rmt0 *

tape1:
	cd ${DISTDIR}; tar cvf /dev/rmt1 *

arch:
	-mv -f ${ARCHDIR}/cad.tar ${ARCHDIR}/cad.tar.bak
	cd ${DISTDIR}; tar cfv ${ARCHDIR}/cad.tar *
	chmod 444 ${ARCHDIR}/cad*.tar
	echo "Please rename this tar file with the release number"

ftp:
	-rm -f ${FTPDIR}/cad.tar ${FTPDIR}/cad2.tar
	cd ${DISTDIR}; tar cfv ${FTPDIR}/cad.tar *
#	cd ${DISTDIR}; tar cfv ${FTPDIR}/cad2.tar [A-Za-oq-z]* paper
	chmod 444 ${FTPDIR}/cad*.tar

# Here, we assume that all the machine-specific files have been set up.
#
#HOSTS=brl-vmb brl-sem
#
#rdist:	# all
#	for host in ${HOSTS} ; do (rdist -d HOSTS=$$host all ; \
#		rsh $$host "(cd cad; make -k all)" ) \
#		; done
#		| mail mike -s CAD_Update ; done
