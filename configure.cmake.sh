#!/bin/sh
#
# This is a simple script to translate autotools style
# configure options into CMake variables.  The resulting
# translation is then run as a CMake configure.

srcpath=$(dirname $0)
options="$srcpath"
while [ "$1" != "" ]
do
   case $1
   in
     --enable-all)                options="$options -DBRLCAD_BUNDLED_LIBS=ON";
                                  shift;;
     --disable-all)                options="$options -DBRLCAD_BUNDLED_LIBS=OFF";
                                  shift;;
     --enable-opengl)                options="$options -DBRLCAD_ENABLE_OPENGL=ON";
                                  shift;;
     --disable-opengl)                options="$options -DBRLCAD_ENABLE_OPENGL=OFF";
                                  shift;;
     --enable-runtime-debug)                options="$options -DBRLCAD_ENABLE_RUNTIME_DEBUG=ON";
                                  shift;;
     --disable-runtime-debug)                options="$options -DBRLCAD_ENABLE_RUNTIME_DEBUG=OFF";
                                  shift;;
     --enable-run-time-debug)                options="$options -DBRLCAD_ENABLE_RUNTIME_DEBUG=ON";
                                  shift;;
     --disable-run-time-debug)                options="$options -DBRLCAD_ENABLE_RUNTIME_DEBUG=OFF";
                                  shift;;
     --enable-runtime-debugging)                options="$options -DBRLCAD_ENABLE_RUNTIME_DEBUG=ON";
                                  shift;;
     --disable-runtime-debugging)                options="$options -DBRLCAD_ENABLE_RUNTIME_DEBUG=OFF";
                                  shift;;
     --enable-run-time-debugging)                options="$options -DBRLCAD_ENABLE_RUNTIME_DEBUG=ON";
                                  shift;;
     --disable-run-time-debugging)                options="$options -DBRLCAD_ENABLE_RUNTIME_DEBUG=OFF";
                                  shift;;
     --enable-debug)                options="$options -DBRLCAD_FLAGS_DEBUG=ON";
                                  shift;;
     --disable-debug)                options="$options -DBRLCAD_FLAGS_DEBUG=OFF";
                                  shift;;
     --enable-flags-debug)                options="$options -DBRLCAD_FLAGS_DEBUG=ON";
                                  shift;;
     --disable-flags-debug)                options="$options -DBRLCAD_FLAGS_DEBUG=OFF";
                                  shift;;
     --enable-debug-flags)                options="$options -DBRLCAD_FLAGS_DEBUG=ON";
                                  shift;;
     --disable-debug-flags)                options="$options -DBRLCAD_FLAGS_DEBUG=OFF";
                                  shift;;
     --enable-warnings)                options="$options -DBRLCAD_ENABLE_COMPILER_WARNINGS=ON";
                                  shift;;
     --disable-warnings)                options="$options -DBRLCAD_ENABLE_COMPILER_WARNINGS=OFF";
                                  shift;;
     --enable-compiler-warnings)                options="$options -DBRLCAD_ENABLE_COMPILER_WARNINGS=ON";
                                  shift;;
     --disable-compiler-warnings)                options="$options -DBRLCAD_ENABLE_COMPILER_WARNINGS=OFF";
                                  shift;;
     --enable-strict)                options="$options -DBRLCAD_ENABLE_STRICT=ON";
                                  shift;;
     --disable-strict)                options="$options -DBRLCAD_ENABLE_STRICT=OFF";
                                  shift;;
     --enable-strict-compile)                options="$options -DBRLCAD_ENABLE_STRICT=ON";
                                  shift;;
     --disable-strict-compile)                options="$options -DBRLCAD_ENABLE_STRICT=OFF";
                                  shift;;
     --enable-strict-compile-flags)                options="$options -DBRLCAD_ENABLE_STRICT=ON";
                                  shift;;
     --disable-strict-compile-flags)                options="$options -DBRLCAD_ENABLE_STRICT=OFF";
                                  shift;;
     --enable-docs)                options="$options -DBRLCAD_EXTRADOCS=ON";
                                  shift;;
     --disable-docs)                options="$options -DBRLCAD_EXTRADOCS=OFF";
                                  shift;;
     --enable-extra-docs)                options="$options -DBRLCAD_EXTRADOCS=ON";
                                  shift;;
     --disable-extra-docs)                options="$options -DBRLCAD_EXTRADOCS=OFF";
                                  shift;;
     --enable-docbook)                options="$options -DBRLCAD_EXTRADOCS=ON";
                                  shift;;
     --disable-docbook)                options="$options -DBRLCAD_EXTRADOCS=OFF";
                                  shift;;
     --enable-regex)                options="$options -DBRLCAD_REGEX=BUNDLED";
                                  shift;;
     --disable-regex)                options="$options -DBRLCAD_REGEX=SYSTEM";
                                  shift;;
     --enable-zlib)                options="$options -DBRLCAD_ZLIB=BUNDLED";
                                  shift;;
     --disable-zlib)                options="$options -DBRLCAD_ZLIB=SYSTEM";
                                  shift;;
     --enable-libz)                options="$options -DBRLCAD_ZLIB=BUNDLED";
                                  shift;;
     --disable-libz)                options="$options -DBRLCAD_ZLIB=SYSTEM";
                                  shift;;
     --enable-lemon)                options="$options -DBRLCAD_LEMON=BUNDLED";
                                  shift;;
     --disable-lemon)                options="$options -DBRLCAD_LEMON=SYSTEM";
                                  shift;;
     --enable-re2c)                options="$options -DBRLCAD_RE2C=BUNDLED";
                                  shift;;
     --disable-re2c)                options="$options -DBRLCAD_RE2C=SYSTEM";
                                  shift;;
     --enable-perplex)                options="$options -DBRLCAD_PERPLEX=BUNDLED";
                                  shift;;
     --disable-perplex)                options="$options -DBRLCAD_PERPLEX=SYSTEM";
                                  shift;;
     --enable-xsltproc)                options="$options -DBRLCAD_XSLTPROC=BUNDLED";
                                  shift;;
     --disable-xsltproc)                options="$options -DBRLCAD_XSLTPROC=SYSTEM";
                                  shift;;
     --enable-xmllint)                options="$options -DBRLCAD_XMLLINT=BUNDLED";
                                  shift;;
     --disable-xmllint)                options="$options -DBRLCAD_XMLLINT=SYSTEM";
                                  shift;;
     --enable-termlib)                options="$options -DBRLCAD_TERMLIB=BUNDLED";
                                  shift;;
     --disable-termlib)                options="$options -DBRLCAD_TERMLIB=SYSTEM";
                                  shift;;
     --enable-png)                options="$options -DBRLCAD_PNG=BUNDLED";
                                  shift;;
     --disable-png)                options="$options -DBRLCAD_PNG=SYSTEM";
                                  shift;;
     --enable-utahrle)                options="$options -DBRLCAD_UTAHRLE=BUNDLED";
                                  shift;;
     --disable-utahrle)                options="$options -DBRLCAD_UTAHRLE=SYSTEM";
                                  shift;;
     --enable-tcl)                options="$options -DBRLCAD_TCL=BUNDLED";
                                  shift;;
     --disable-tcl)                options="$options -DBRLCAD_TCL=SYSTEM";
                                  shift;;
     --enable-tk)                options="$options -DBRLCAD_TK=BUNDLED";
                                  shift;;
     --disable-tk)                options="$options -DBRLCAD_TK=SYSTEM";
                                  shift;;
     --enable-itcl)                options="$options -DBRLCAD_ITCL=BUNDLED";
                                  shift;;
     --disable-itcl)                options="$options -DBRLCAD_ITCL=SYSTEM";
                                  shift;;
     --enable-itk)                options="$options -DBRLCAD_ITK=BUNDLED";
                                  shift;;
     --disable-itk)                options="$options -DBRLCAD_ITK=SYSTEM";
                                  shift;;
     --enable-togl)                options="$options -DBRLCAD_TOGL=BUNDLED";
                                  shift;;
     --disable-togl)                options="$options -DBRLCAD_TOGL=SYSTEM";
                                  shift;;
     --enable-iwidgets)                options="$options -DBRLCAD_IWIDGETS=BUNDLED";
                                  shift;;
     --disable-iwidgets)                options="$options -DBRLCAD_IWIDGETS=SYSTEM";
                                  shift;;
     --enable-tkhtml)                options="$options -DBRLCAD_TKHTML=BUNDLED";
                                  shift;;
     --disable-tkhtml)                options="$options -DBRLCAD_TKHTML=SYSTEM";
                                  shift;;
     --enable-tkpng)                options="$options -DBRLCAD_TKPNG=BUNDLED";
                                  shift;;
     --disable-tkpng)                options="$options -DBRLCAD_TKPNG=SYSTEM";
                                  shift;;
     --enable-tktable)                options="$options -DBRLCAD_TKTABLE=BUNDLED";
                                  shift;;
     --disable-tktable)                options="$options -DBRLCAD_TKTABLE=SYSTEM";
                                  shift;;
     --enable-tktreectrl)                options="$options -DBRLCAD_TKTREECTRL=BUNDLED";
                                  shift;;
     --disable-tktreectrl)                options="$options -DBRLCAD_TKTREECTRL=SYSTEM";
                                  shift;;
     --enable-tkdnd)                options="$options -DBRLCAD_TKDND=BUNDLED";
                                  shift;;
     --disable-tkdnd)                options="$options -DBRLCAD_TKDND=SYSTEM";
                                  shift;;
     --enable-opennurbs)                options="$options -DBRLCAD_OPENNURBS=BUNDLED";
                                  shift;;
     --disable-opennurbs)                options="$options -DBRLCAD_OPENNURBS=SYSTEM";
                                  shift;;
     --enable-scl)                options="$options -DBRLCAD_SCL=BUNDLED";
                                  shift;;
     --disable-scl)                options="$options -DBRLCAD_SCL=SYSTEM";
                                  shift;;
     --enable-step)                options="$options -DBRLCAD_SCL=BUNDLED";
                                  shift;;
     --disable-step)                options="$options -DBRLCAD_SCL=SYSTEM";
                                  shift;;
     --enable-step-class-libraries)                options="$options -DBRLCAD_SCL=BUNDLED";
                                  shift;;
     --disable-step-class-libraries)                options="$options -DBRLCAD_SCL=SYSTEM";
                                  shift;;
     --enable-vds)                options="$options -DBRLCAD_LIBVDS=BUNDLED";
                                  shift;;
     --disable-vds)                options="$options -DBRLCAD_LIBVDS=SYSTEM";
                                  shift;;
     --prefix=*)   	          inputstr=$1;
				  options="$options -DCMAKE_INSTALL_PREFIX=${inputstr#--prefix=}";
				  shift;;
     *) 	   	          echo "Warning: unknown option $1";
				  shift;;
   esac
done
echo cmake $options
cmake $options
