#!/bin/sh
#########################################################################
#									#
#			    build_def_index.sh				#
#									#
# Use this shell script to build an HTML index file from one or more	#
# definition files. In this context, a definition file is one that	#
# contains HTML definition terms (i.e. <DT>some term</DT>).		#
#									#
# Author -								#
#	Robert G. Parker						#
#									#
# Source -								#
#	SLAD/BND/ACST							#
#	The U. S. Army Research Laboratory				#
#	Aberdeen Proving Ground, Maryland  21005-5066			#
#									#
#########################################################################

NUM_COLUMNS=7
CELLSPACING=4

# Note that spaces are not allowed. Here we use comma's instead of spaces.
DEF_FILES_AND_TITLES="{{mged_cmds.html}{MGED,Commands}}
    {{mged_devel_cmds.html}{MGED,Developer,Commands}}"
TARGET_FILE=mged_cmds_index.html

if test -f $TARGET_FILE
then
    rm $TARGET_FILE
fi

echo "<!-- ******************** DO NOT EDIT ********************* -->" >> $TARGET_FILE
echo "<!-- This file was generated by running build_def_index.sh. -->" >> $TARGET_FILE
echo "" >> $TARGET_FILE
echo "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">" >> $TARGET_FILE
echo "<HTML>" >> $TARGET_FILE
echo "" >> $TARGET_FILE
echo "<HEAD>" >> $TARGET_FILE
echo "<TITLE>MGED Command Index</Title>" >> $TARGET_FILE
echo "</HEAD>" >> $TARGET_FILE
echo "" >> $TARGET_FILE
echo "<BODY BGCOLOR=\"#E0D8c8\" TEXT=\"#000000\">" >> $TARGET_FILE
echo "" >> $TARGET_FILE

for DEF_FILE_AND_TITLE in ${DEF_FILES_AND_TITLES}
do
	DEF_FILE=`echo $DEF_FILE_AND_TITLE |
	    sed 's/{{\([^{}][^{}]*\)}{[^{}][^{}]*}}/\1/g'`
	TITLE=`echo $DEF_FILE_AND_TITLE |
	    sed 's/{{[^{}][^{}]*}{\([^{}][^{}]*\)}}/\1/g
		s/,/ /g'`

	DEFS_AND_ANCHORS=`sed -n 's/<DT>[\t ]*<a[\t ][\t ]*NAME="\([a-zA-Z0-9_?%][a-zA-Z0-9_?%]*\)"[\t ]*>[\t ]*<B>[\t ]*\([a-zA-Z0-9_?%][a-zA-Z0-9_?%]*\)[\t ]*<\/B>.*/{{\2}{\1}}/p' $DEF_FILE`

	echo "<CENTER><H1>$TITLE</H1></CENTER>" >> $TARGET_FILE
	echo "<TABLE CELLSPACING=$CELLSPACING>" >> $TARGET_FILE
	count=-1

	for DEF_AND_ANCHOR in ${DEFS_AND_ANCHORS}
	do
	    count=`expr $count + 1`
	    if test `expr $count % $NUM_COLUMNS` = 0
	    then
		if test $count -gt 0
		then
		    echo "</TR>" >> $TARGET_FILE
		fi

		echo "<TR>" >> $TARGET_FILE
	    fi

	    DEF=`echo $DEF_AND_ANCHOR |
		    sed 's/{{\([a-zA-Z0-9_?%][a-zA-Z0-9_?%]*\)}{[a-zA-Z0-9_?%][a-zA-Z0-9_?%]*}}/\1/g'`
	    ANCHOR=`echo $DEF_AND_ANCHOR |
		    sed 's/{{[a-zA-Z0-9_?%][a-zA-Z0-9_?%]*}{\([a-zA-Z0-9_?%][a-zA-Z0-9_?%]*\)}}/\1/g'`
	    echo "<TD><a href=\"$TARGET_FILE#$ANCHOR\">$DEF</a></TD>" >> $TARGET_FILE
	done

	echo "</TR>" >> $TARGET_FILE
	echo "</TABLE>" >> $TARGET_FILE
	echo "<HR>" >> $TARGET_FILE
	echo "" >> $TARGET_FILE
	cat $DEF_FILE >> $TARGET_FILE
done

echo "" >> $TARGET_FILE
echo "</BODY>" >> $TARGET_FILE
echo "</HTML>" >> $TARGET_FILE
