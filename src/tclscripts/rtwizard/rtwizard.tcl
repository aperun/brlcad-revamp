#!/bin/sh
#                    R T W I Z A R D . T C L
# BRL-CAD
#
# Copyright (c) 2006-2012 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this file; see the file named COPYING for more
# information.
#
###
#
# Start-up script for both the command line and graphical modes of
# rtwizard
#

# The trailing backslash forces tcl to skip the next line \
STARTUP_HOME=`dirname $0`/..
#\
export STARTUP_HOME
# restart using btclsh \
TCLSH="btclsh"
#\
for clsh in btclsh btclsh_d ; do
# see if we're installed \
    if test -f ${STARTUP_HOME}/bin/$clsh ; then
#\
	TCLSH="${STARTUP_HOME}/bin/$clsh"
#\
	break;
#\
    fi
# see if we're not installed yet \
    if test -f ${STARTUP_HOME}/btclsh/$clsh ; then
#\
	TCLSH="${STARTUP_HOME}/btclsh/$clsh"
#\
	break;
#\
    fi
#\
done
#\
exec "$TCLSH" "$0" "$@"

#
# Begin Tcl here!
#

source [file join [bu_brlcad_data "tclscripts"] util getopt.tcl]
getopt::init {
        {verbose v {::verbose}}
        {gui gui  {::use_gui}}
        {no-gui no-gui  {::disable_gui}}
        {g-file   i  {::have_gfile ::g_file}}
        {output   o  {::output ::output_filename}}
        {full-color-objects  c  {::have_full_color_objs ::color_objlist ...}}
        {background-color background-color {::have_fc_bg_color ::fc_bg_color}}
        {ghost-objects  g  {::have_ghost_objs ::ghost_objlist ...}}
        {ghosting-intensity ghosting-intensity {::have_ghosting_intensity ::ghosting_intensity}}
        {edge-objects  e  {::have_edge_objs ::edge_objlist ...}}
        {line-color line-color {::have_line_color ::line_color}}
        {non-line-color non-line-color {::have_non_line_color ::non_line_color}}
        {occlusion occlusion {::have_occlusion_setting ::occlusion_setting}}
        {type t {::have_picture_type ::picture_type}}
        {matrix M {::have_matrix ::matrix}}
        {azimuth a  {::have_azimuth ::azimuth}}
        {elevation e {::have_elevation ::elevation}}
        {cpus P  {::have_cpu_cnt ::cpus_use}}
}
set argv2 [getopt::getopt $argv]

# During development, force default behavior (comment the following out to work on new code)
set ::use_gui 1

# If we have both gui and no-gui specified, use gui
if {[info exists ::use_gui] && [info exists ::disable_gui]} {
   puts "Warning - both -gui and -no-gui supplied - enabling gui"
   unset ::disable_gui
}

# If we have an explicit picture type, check whether we satisfy the minimum
# data input for that type.
if {[info exists ::have_picture_type] && ![info exists ::use_gui]} {
  switch $::picture_type {
    A   -
    1	{
          if {![info exists ::color_objlist]} {
             if ([info exists ::disable_gui]) {
               puts "Error - picture type $::picture_type specified, but no full color objects listed"
               puts "Please specify full color objects using the -c option\n"
               exit
             } else {
               set ::use_gui 1
             }
          }
        }
    B   -
    2	{
          if {![info exists ::edge_objlist]} {
             if ([info exists ::disable_gui]) {
               puts "Error - picture type $::picture_type specified, but no edge objects listed"
               puts "Please specify edge objects using the -e option\n"
               exit
             } else {
               set ::use_gui 1
             }
          }
        }
    C   -
    D   -
    3	-
    4	{
          if {![info exists ::color_objlist] || ![info exists ::edge_objlist]} {
            if (![info exists ::disable_gui]) {
               set ::use_gui 1
            } else {
              if {![info exists ::edge_objlist]} {
                puts "Error - picture type $::picture_type specified, but no edge objects listed"
                puts "Please specify edge objects using the -e option\n"
              }
              if {![info exists ::color_objlist]} {
                puts "Error - picture type $::picture_type specified, but no color objects listed"
                puts "Please specify full color objects using the -c option\n"
              }
             exit
            }
          }
        }
    E   -
    5	{
          if {![info exists ::color_objlist] || ![info exists ::ghost_objlist]} {
            if (![info exists ::disable_gui]) {
               set ::use_gui 1
            } else {
              if {![info exists ::ghost_objlist]} {
                puts "Error - picture type $::picture_type specified, but no ghost objects listed"
                puts "Please specify ghost objects using the -g option\n"
              }
              if {![info exists ::color_objlist]} {
                puts "Error - picture type $::picture_type specified, but no color objects listed"
                puts "Please specify full color objects using the -c option\n"
              }
             exit
            }
          }
        }
    F   -
    6	{
          if {![info exists ::color_objlist] || ![info exists ::edge_objlist] || ![info exists ::ghost_objlist]} {
            if (![info exists ::disable_gui]) {
               set ::use_gui 1
            } else {
              if {![info exists ::ghost_objlist]} {
                puts "Error - picture type $::picture_type specified, but no ghost objects listed"
                puts "Please specify ghost objects using the -g option\n"
              }
              if {![info exists ::color_objlist]} {
                puts "Error - picture type $::picture_type specified, but no color objects listed"
                puts "Please specify full color objects using the -c option\n"
              }
              if {![info exists ::edge_objlist]} {
                puts "Error - picture type $::picture_type specified, but no edge objects listed"
                puts "Please specify edge objects using the -e option\n"
              }
             exit
            }
          }
        }
    default {puts "Error - unknown picture type $::picture_type\n"; exit}
  }
}

# If we're launching without enough arguments to fully specify an rtwizard 
# run, a gui run has been specifically requested, or we've got arguments 
# that aren't understood, go graphical
if {$argc == 0 || "$argv2" != "" || [info exists ::use_gui]} {
   # Have to do these loads until we get "package require tclcad" and "package require dm" 
   # working - bwish loads them for us by default, but since rtwizard may be either
   # graphical or command line we need to start with btclsh
   load [file join [bu_brlcad_root "lib"] libtclcad[info sharedlibextension]]
   load [file join [bu_brlcad_root "lib"] libdm[info sharedlibextension]]
   # Now, load RaytraceWizard.tcl
   source [file join [bu_brlcad_data "tclscripts"] rtwizard RaytraceWizard.tcl]
   exit
} else {

puts "rtwizard arguments: $argv"
if {[info exists ::verbose]} {puts "rtwizard verbose ON"}

}

# Local Variables:
# mode: sh
# tab-width: 8
# sh-indentation: 4
# sh-basic-offset: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
