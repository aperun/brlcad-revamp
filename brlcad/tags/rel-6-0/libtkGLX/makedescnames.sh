#!/bin/sh
DOT_H=descnames.h

rm -f ${DOT_H}
(echo ""
echo "/* This file is generated automatically.                     */"
echo "/* DO NOT EDIT THIS FILE.  YOUR CHANGES MAY BE OVERWRITTEN.  */"
echo ""

echo "struct _tkglx_gldesc {"
echo "    int   value;"
echo "    char *name;"
echo "};"
echo ""

echo "static struct _tkglx_gldesc TkGLX_gldesc[] =  {"

egrep "#define GD_" /usr/include/gl/gl.h | \
egrep -v "(GD_WSYS_NONE|GD_WSYS_4S|GD_NOLIMIT|GD_SCRNTYPE_)" | \
awk '{printf "    { %-25s, \"%s\" },\n", $2, $2}'

echo '    { 0, (char *)NULL }' 
echo '};' ) > ${DOT_H}
