# rtsync.tcl
# A prototype GUI for rtsync.
# This file is executed by the rtsync program, not directly from the shell.
# Depends on rtnode defining [get_dbip] and [get_rtip].
# Depends on rtnode & mged having executed {wdb_open .inmem inmem [get_dbip]}
# Uses various RTSYNC built-in commands, as well as LIBRT's Tcl commands.
#  -Mike Muuss, ARL, March 97.

global red grn blu
set red 0
set grn 0
set blu 0

global sun_region_name
set sun_region_name "light.r"

global sun_solid_name
set sun_solid_name "LIGHT"
global sunx suny sunz
set sunx 0
set suny 0
set sunz 0

global air_shader1
set air_shader1 "200 200 255"
set air_shader2 "air dpm=.01"

puts "running rtsync.tcl"
##option add *background #ffffff
##. configure -background #ffffff

# Create main interaction widget
frame .mbar -relief raised -bd 2 ; pack .mbar -side top
frame .logo_fr ; pack .logo_fr -side top
frame .title_fr ; pack .title_fr -side top
frame .sunangle_fr -relief ridge -bd 2 ; pack .sunangle_fr -side top -expand 1 -fill x
frame .suncolor_fr -relief ridge -bd 2 ; pack .suncolor_fr -side top -expand 1 -fill x
frame .air_fr  -relief ridge -bd 2 ; pack .air_fr -side top -expand 1 -fill x
frame .button_fr ; pack .button_fr -side top

# Menu bar, acros very top
menubutton .mbar.file -text "File" -menu .mbar.file.menu
menubutton .mbar.debug -text "Debug" -menu .mbar.debug.menu
menubutton .mbar.spacepart -text "SpacePart" -menu .mbar.spacepart.menu
menubutton .mbar.help -text "Help" -menu .mbar.help.menu
pack .mbar.file -side left -in .mbar -expand 1 -fill x
pack .mbar.debug -side left -in .mbar -expand 1 -fill x
pack .mbar.spacepart -side left -in .mbar -expand 1 -fill x
pack .mbar.help -side right -in .mbar -expand 1 -fill x
menu .mbar.file.menu
.mbar.file.menu add command -label "Exit" -command "exit"
menu .mbar.help.menu
.mbar.help.menu add command -label "Exit" -command "exit"
menu .mbar.debug.menu
.mbar.debug.menu add command -label "Net Speed Test ON" -command "net_speed_test 1"
.mbar.debug.menu add command -label "Net Speed Test OFF" -command "net_speed_test 0"
menu .mbar.spacepart.menu
.mbar.spacepart.menu add command -label "Muuss NUBSP" -command "space_partitioning 1"
.mbar.spacepart.menu add command -label "Gigante NUgrid" -command "space_partitioning 0"

# Title, across the top
frame .words_fr
frame .title3
image create photo .eagle -file "/m/cad/remrt/eagleCAD.gif"
label .logo -image .eagle
label .title1 -text "RTSYNC"
label .title2 -text "The Real-Time Ray-Tracer"
label .title3a -textvariable cpu_count
label .title3b -text "CPUs active"
pack .title3a .title3b -side left -in .title3
pack .title1 .title2 .title3 -side top -in .words_fr
pack .logo .words_fr -side left -in .title_fr

# The sun angle
label .sunangle_title -text "Sun Angle"
scale .sunangle -label "West to East" -from 180 -to 0 -orient horizontal \
	-command new_sunangle
pack .sunangle_title .sunangle -side top -in .sunangle_fr -fill x
proc new_sunangle {degrees} {
	global sunx suny sunz

	set sunx [expr ( $degrees - 90) ]
	set suny -13
	set sunz 5
}
button .apply_angle -text "Apply" -command apply_angle
pack .apply_angle -side top -in .sunangle_fr

proc apply_angle {} {
	global sunx suny sunz
	global sun_solid_name

	puts "Sun pos = $sunx $suny $sunz"
	# send new stuff to servers
	node_send .inmem adjust $sun_solid_name V "{" $sunx $suny $sunz "}"

	# Have MGED update it's position too.
	vrmgr_send .inmem adjust $sun_solid_name V "{" $sunx $suny $sunz "}" ";" \
		redraw_vlist $sun_solid_name

	# indicate LIBRT re-prep required.
	reprep
	# Use new POV if one receieved, else repeat last POV.
	refresh
}

# The sun color
label .suncolor_title -text "Sun Color"
scale .suncolor -label "Yellow to Blue" -from 0 -to 100 -orient horizontal \
	-command new_suncolor
frame .suncolor_swatch -height 1c -width 6c
pack .suncolor_title .suncolor .suncolor_swatch -in .suncolor_fr -fill x
proc new_suncolor {percent} {
	global red grn blu
	# alpha is how much yellow, beta is how much blue.
	set beta [expr $percent / 100.0 ]
	set alpha [expr 1.0 - $beta ]
	# yellow is 255,255,0 and SkyBlue2 is 126 192 238
	set red [expr int(255 * $alpha + 126 * $beta)]
	set grn [expr int(255 * $alpha + 192 * $beta)]
	set blu [expr int(  0 * $alpha + 238 * $beta)]
	set hexcolor [format #%02x%02x%02x $red $grn $blu]
	.suncolor_swatch config -background $hexcolor
	#puts "Sun color $percent percent of yellow = $red $grn $blu"
}

button .apply_color -text "Apply" -command apply_color
pack .apply_color -side top -in .suncolor_fr

entry .suncolor_region -width 16 -relief sunken -bd 2 -textvariable sun_region_name
pack .suncolor_region -side top -in .suncolor_fr

proc apply_color {} {
	global red grn blu
	global sun_region_name

	# send new stuff to servers.  No reprep needed for this.
	# However, change the inmem database too, for consistency.
	node_send \
	  sh_directchange_rgb {[get_rtip]} $sun_region_name $red $grn $blu ";" \
	  .inmem adjust $sun_region_name rgb $red $grn $blu

	# Have MGED update it's color too.  This doesn't work.
	#vrmgr_send .inmem adjust $sun_region_name rgb $red $grn $blu ";" \
	#	redraw_vlist $sun_region_name

	# Use new POV if one receieved, else repeat last POV.
	refresh
}

# The air shader
label .air_title -text "Air Shader"
entry .air_string1 -width 32 -relief sunken -bd 2 -textvariable air_shader1
entry .air_string2 -width 32 -relief sunken -bd 2 -textvariable air_shader2
button .air_apply -text "Apply" -command apply_air
pack .air_title .air_string1 .air_string2 .air_apply -side top -in .air_fr

proc apply_air {} {
	global air_shader1
	global air_shader2

	# send new stuff to servers
	# XXX region name is hardcoded for now.
	node_send .inmem adjust air.r rgb "{" $air_shader1 "}" ";" \
		  .inmem adjust air.r shader "{" $air_shader2 "}"
	vrmgr_send .inmem adjust air.r rgb "{" $air_shader1 "}" ";" \
		   .inmem adjust air.r shader "{" $air_shader2 "}"

	# indicate LIBRT re-prep required.
	reprep
	# Use new POV if one receieved, else repeat last POV.
	refresh
}

proc net_speed_test {val} {
	node_send \
		set test_fb_speed $val ";" \
		set curframe 0
}

proc space_partitioning {val} {
	node_send \
		sh_opt -,$val ";" \
		set curframe 0
}

# Allow "send rtsync _stuff_" directives to reach us.
tk appname rtsync

puts "done rtsync.tcl"

# node_send sh_opt -P1 -x1 -X1
# node_send .inmem get LIGHT
# node_send {.inmem adjust LIGHT V {20 -13 5}}
# node_send {.inmem adjust LIGHT V {50 -13 5}} ; reprep; refresh
