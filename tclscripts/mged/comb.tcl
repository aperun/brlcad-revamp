#
#			C O M B . T C L
#
#	Widget for editing a combination.
#
#	Author - Robert G. Parker
#

check_externs "get_comb put_comb"

proc init_comb { id } {
    global mged_gui
    global comb_control
    global shader_params

    set top .$id.comb

    if [winfo exists $top] {
	raise $top
	return
    }

    # set the padding
    set comb_control($id,padx) 4
    set comb_control($id,pady) 2

    set comb_control($id,name) ""
    set comb_control($id,isRegion) "Yes"
    set comb_control($id,id) ""
    set comb_control($id,air) ""
    set comb_control($id,material) ""
    set comb_control($id,los) ""
    set comb_control($id,color) ""
    set comb_control($id,inherit) ""
    set comb_control($id,comb) ""
    set comb_control($id,shader) ""
    set comb_control($id,shader_gui) ""

    toplevel $top -screen $mged_gui($id,screen)

    frame $top.gridF1
    frame $top.gridF2 -relief groove -bd 2
    frame $top.gridF3
    frame $top.gridF4
    frame $top.gridF5
    frame $top.nameF -relief sunken -bd 2
    frame $top.idF -relief sunken -bd 2
    frame $top.airF -relief sunken -bd 2
    frame $top.materialF -relief sunken -bd 2
    frame $top.losF -relief sunken -bd 2
    frame $top.shaderF1
    frame $top.shaderF2
    frame $top.shaderEF -relief sunken -bd 2
    frame $top.combF

    set comb_control($id,shader_frame) $top.shaderF2
    set shader_params($id,shader_name) ""
    set shader_params($id,window) $comb_control($id,shader_frame)

    label $top.nameL -text "Name" -anchor e
    set hoc_data { { summary "The combination name is the name of a
region or a group. The region flag must be set
in order for the combination to be a region.
Note that a region defines a space containing
homogeneous material. In contrast, a group can
contain many different materials." } }
    hoc_register_data $top.nameL "Combination Name" $hoc_data
    entry $top.nameE -relief flat -width 12 -textvar comb_control($id,name)
    hoc_register_data $top.nameE "Combination Name" $hoc_data
    menubutton $top.nameMB -relief raised -bd 2\
	    -menu $top.nameMB.m -indicatoron 1
    hoc_register_data $top.nameMB "Combination Selection"\
	    { { summary "This pops up a menu of methods for selecting
a combination. At the moment there are two methods.
The first brings up a listbox containing the combinations
currently being displayed in the geometry window. The
user can select from among the combinations listed here.
The second selection method allows the user to use the
mouse to fire a ray at the combination of interest. If
only one combination is hit, that combination is selected.
If nothing is hit the user can simply fire another ray,
perhaps taking better aim :-). If more than one combination
is hit, a listbox of the hit combinations is presented
as with the first selection method. Note - When selecting
items from a listbox, a left buttonpress highlights the combination
in question until the button is released. To select a combination,
double click with the left mouse button." } }
    menu $top.nameMB.m -title "Combination Selection Method" -tearoff 0
    $top.nameMB.m add command -label "Select from all displayed"\
	    -command "winset \$mged_gui($id,active_dm); build_comb_menu_all"
    hoc_register_menu_data "Combination Selection Method" "Select from all displayed"\
	    "Select Combination From All Displayed"\
	    { { summary "This pops up a listbox containing the combinations
currently being displayed in the geometry window. The
user can select from among the combinations listed here.
Note - When selecting items from a listbox, a left buttonpress
highlights the combination in question until the button is
released. To select a combination, double click with the left
mouse button." } }
    $top.nameMB.m add command -label "Select along ray"\
	    -command "winset \$mged_gui($id,active_dm); set mouse_behavior c"
    hoc_register_menu_data "Combination Selection Method" "Select along ray"\
	    "Select Combination Along Ray"\
	    { { summary "This method allows the user to use the
mouse to fire a ray at the combination of interest.
If only one combination is hit, that combination is
selected. If nothing is hit the user can simply fire
another ray, perhaps taking better aim :-). If more
than one combination is hit, a listbox of the hit
combinations is presented. Note - When selecting
items from a listbox, a left buttonpress highlights
the combination in question until the button is
released. To select a combination, double click with
the left mouse button." } }

    label $top.idL -text "Region Id" -anchor e
    set hoc_data { { summary "The region id (i.e. item code) is a number
that is typically used for grouping regions
belonging to a particular component. If the
region id is non-zero it is considered to be
a model component. Otherwise, it is considered
to be air. The air code is then used to designate
the kind of air." } }
    hoc_register_data $top.idL "Region Id" $hoc_data
    entry $top.idE -relief flat -width 12 -textvar comb_control($id,id)
    hoc_register_data $top.idE "Region Id" $hoc_data

    label $top.airL -text "Air Code" -anchor e
    set hoc_data { { summary "The air code is a number that is typically
used to designate the kind of air a region
represents. An air code of \"1\" signifies
universal air. An air code that is greater
than \"1\" signifies some other kind of air.
While an air code of \"0\" means that the
region represents a component." } }
    hoc_register_data $top.airL "Air Code" $hoc_data
    entry $top.airE -relief flat -width 12 -textvar comb_control($id,air)
    hoc_register_data $top.airE "Air Code" $hoc_data

    label $top.materialL -text "Material Id" -anchor e
    set hoc_data { { summary "The material id represents a particular
material type as identified by a material
database. In the past, the gift material
database was used to identify the material
type." } }
    hoc_register_data $top.materialL "Material Id" $hoc_data
    entry $top.materialE -relief flat -width 12 -textvar comb_control($id,material)
    hoc_register_data $top.materialE "Material Id" $hoc_data

    label $top.losL -text "LOS" -anchor e
    set hoc_data { { summary "LOS is a number that represents the
percentage of material a component region
is composed of. For example, if some component
region is defined to be made of \"mild steel\", as
designated by its material id, with an LOS of 20
then the region is considered to be composed of
20% \"mild steel\"."  } }
    hoc_register_data $top.losL "LOS" $hoc_data
    entry $top.losE -relief flat -width 12 -textvar comb_control($id,los)
    hoc_register_data $top.losE "LOS" $hoc_data

    label $top.colorL -text "Color" -anchor e
    hoc_register_data $top.colorL "Color"\
	    { { summary "Material color is used by the ray tracer
when rendering. It is also used by MGED
when determining what color to use when
drawing an object" } }
    # $top.colorF is the name of the container created by color_entry_build
    # that contains the entry and menubutton for specifying a color
    color_entry_build $top color comb_control($id,color)\
	    "color_entry_chooser $id $top color \"Combination Color\"\
	    comb_control $id,color"\
	    12 $comb_control($id,color)

    label $top.shaderL -text "Shader" -anchor e
    set hoc_data { { summary "Use this to manually enter the shader
parameters. Note - when entering the
shader parameters directly, the shader
GUI will automatically be updated." } }
    hoc_register_data $top.shaderL "Shader" $hoc_data
    entry $top.shaderE -relief flat -width 12 -textvar comb_control($id,shader)
    hoc_register_data $top.shaderE "Shader" $hoc_data

    bind $top.shaderE <KeyRelease> "set_shader_params comb_control($id,shader) $id; \
	    comb_callback $id $top.gridF2"

    menubutton $top.shaderMB -relief raised -bd 2\
	    -menu $top.shaderMB.m -indicatoron 1
    hoc_register_data $top.shaderMB "Menu of Shaders"\
	    { { summary "This pops up a menu of shader types.
Selecting a shader from the menu will
reconfigure the shader's GUI to take on
the form of the selected shader type." } }
    menu $top.shaderMB.m -title "Shader" -tearoff 0
    $top.shaderMB.m add command -label plastic \
	    -command "comb_shader_gui $id plastic"
    hoc_register_menu_data "Shader" plastic "Shader - Plastic" \
	    { { summary "Set shader parameters to make this object appear as plastic." } }
    $top.shaderMB.m add command -label mirror \
	    -command "comb_shader_gui $id mirror"
    hoc_register_menu_data "Shader" mirror "Shader - Mirror" \
	    { { summary "Set shader parameters to make this object appear as a mirror." } }
    $top.shaderMB.m add command -label glass \
	    -command "comb_shader_gui $id glass"
    hoc_register_menu_data "Shader" glass "Shader - Glass" \
	    { { summary "Set shader parameters to make this object appear as glass." } }
    $top.shaderMB.m add command -label "texture (color)" \
	    -command "comb_shader_gui $id texture"
    hoc_register_menu_data "Shader" "texture (color)" "Shader - Texture (color)"\
	    { { summary "Map a color texture on this object." } }
    $top.shaderMB.m add command -label "texture (b/w)" \
	    -command "comb_shader_gui $id bwtexture"
    hoc_register_menu_data "Shader" "texture (b/w)" "Shader - Texture (b/w)" \
	    { { summary "Map a black and white texture on this object." } }
    $top.shaderMB.m add command -label "bump map" \
	    -command "comb_shader_gui $id bump"
    hoc_register_menu_data "Shader" "bump map" "Shader - Bump" \
		{ { summary "Apply a bump map to perturb the surface normals of this object." } }
    $top.shaderMB.m add command -label "checker" \
	    -command "comb_shader_gui $id checker"
    hoc_register_menu_data "Shader" "checker" "Shader - Checker" \
		{ { summary "texture map a checkerboard pattern on this object." } }
    $top.shaderMB.m add command -label "test map" \
	    -command "comb_shader_gui $id testmap"
    hoc_register_menu_data "Shader" "test map" "Shader - testmap" \
		{ { summary "Map a red and blue gradient on this object proportional to 'uv'\n\
			texture map coordinates." } }
    $top.shaderMB.m add command -label "fake star pattern" \
	    -command "comb_shader_gui $id fakestar"
    hoc_register_menu_data "Shader" "fake star pattern" "Shader - fakestar" \
		{ { summary "Map a fake star field on this object." } }
    $top.shaderMB.m add command -label "cloud" \
	-command "comb_shader_gui $id cloud"
    hoc_register_menu_data "Shader" "cloud" "Shader - cloud" \
	{ { summary "Map a cloud texture on this object." } }
    $top.shaderMB.m add command -label "stack" \
	-command "comb_shader_gui $id stack"
    hoc_register_menu_data "Shader" "stack" "Shader - stack" \
	{ { summary "Consecutively apply multiple shaders to this object." } }
    $top.shaderMB.m add command -label "envmap" \
	-command "comb_shader_gui $id envmap"
    hoc_register_menu_data "Shader" "envmap" "Shader - envmap" \
	{ { summary "Apply an environment map using this region." } }

    label $top.combL -text "Boolean Expression:" -anchor w
    set hoc_data { { summary "A boolean expression is used to combine
objects to form a region or group. This expression
can consist of three kinds of operators. The 'u'
operator indicates union. The union of two objects
is defined as the volume in both objects. The '-'
operator indicates difference. The difference of two
objects is defined as the volume of the first object
minus the volume of the second object. Lastly, the
'+' operator indicates intersection. The intersection
of two objects is defined as the volume common to
both objects. Note - an object can be a solid, region
or group." } }
    hoc_register_data $top.combL "Boolean Expression" $hoc_data
    text $top.combT -relief sunken -bd 2 -width 40 -height 10\
	    -yscrollcommand "$top.combS set" -setgrid true
    hoc_register_data $top.combT "Edit Boolean Expression" $hoc_data
    scrollbar $top.combS -relief flat -command "$top.combT yview"

    checkbutton $top.isRegionCB -relief raised -text "Is Region"\
	    -offvalue No -onvalue Yes -variable comb_control($id,isRegion)\
	    -command "comb_toggle_isRegion $id"
    hoc_register_data $top.isRegionCB "Is Region"\
	    { { summary "Toggle the region flag on/off. If the
region flag is toggled on (i.e. checkbutton
is highligted) the GUI reconfigures itself to
handle regions. If the region flag is toggled
off the GUI reconfigures itself to handle
groups. Note - both regions and groups are
combinations. However, a region is special
in that it has its region flag set. It is
also required to be homogeneous (i.e. consisting
of the same material)." } }
    checkbutton $top.inheritCB -relief raised -text "Inherit"\
	    -offvalue No -onvalue Yes -variable comb_control($id,inherit)
    hoc_register_data $top.inheritCB "Inherit"\
	    { { summary "Toggle inheritance on/off. If inheritance
is on, everything in the group takes on
the characteristics of the group. Note -
setting the inheritance for a region has
no effect (i.e. the members of a region
always take on the characteristics of the
region)." } }

    button $top.okB -relief raised -text "Ok"\
	    -command "comb_ok $id $top"
    hoc_register_data $top.okB "Ok"\
	    { { summary "Apply the data in the \"Combination Editor\"
to the combination then close the
\"Combination Editor\"." } }
    button $top.applyB -relief raised -text "Apply"\
	    -command "comb_apply $id"
    hoc_register_data $top.applyB "Apply"\
	    { { summary "Apply the data in the \"Combination Editor\"
to the combination." } }
    button $top.resetB -relief raised -text "Reset"\
	    -command "comb_reset $id"
    hoc_register_data $top.resetB "Reset"\
	    { { summary "Reset the \"Combination Editor\" with data
from the combination." } }
    button $top.dismissB -relief raised -text "Dismiss" \
	    -command "comb_dismiss $id $top"
    hoc_register_data $top.dismissB "Dismiss" \
	    { { summary "Dismiss (close) the \"Combination Editor\"." } }

    grid $top.nameE $top.nameMB -sticky "ew" -in $top.nameF
    grid columnconfigure $top.nameF 0 -weight 1
    grid $top.idE -sticky "ew" -in $top.idF
    grid columnconfigure $top.idF 0 -weight 1
    grid $top.nameL $top.nameF x $top.idL $top.idF \
	    -sticky "ew" -in $top.gridF1 -pady $comb_control($id,pady)

    grid $top.airE -sticky "ew" -in $top.airF
    grid columnconfigure $top.airF 0 -weight 1
    grid $top.colorL $top.colorF x $top.airL $top.airF \
	    -sticky "ew" -in $top.gridF1 -pady $comb_control($id,pady)

    grid $top.materialE -sticky "ew" -in $top.materialF
    grid columnconfigure $top.materialF 0 -weight 1
    grid $top.losE -sticky "ew" -in $top.losF
    grid columnconfigure $top.losF 0 -weight 1
    grid $top.materialL $top.materialF x $top.losL $top.losF \
	    -sticky "ew" -in $top.gridF1 -pady $comb_control($id,pady)

    grid columnconfigure $top.gridF1 1 -weight 100
    grid columnconfigure $top.gridF1 2 -weight 1
    grid columnconfigure $top.gridF1 4 -weight 100

    grid $top.shaderE $top.shaderMB \
	    -sticky "ew" \
	    -in $top.shaderEF
    grid columnconfigure $top.shaderEF 0 -weight 1

    grid $top.shaderL $top.shaderEF \
	    -sticky "ew" \
	    -in $top.shaderF1
    grid columnconfigure $top.shaderF1 1 -weight 1

    grid $top.shaderF1 \
	    -sticky "ew" \
	    -in $top.gridF2\
	    -padx $comb_control($id,padx) \
	    -pady $comb_control($id,pady)
#    grid $top.shaderF2 \
#	    -sticky "ew" \
#	    -in $top.gridF2 \
#	    -padx $comb_control($id,padx) \
#	    -pady $comb_control($id,pady)
    grid columnconfigure $top.gridF2 0 -weight 1

    grid $top.isRegionCB $top.inheritCB x -sticky "ew" -in $top.gridF3\
	    -ipadx 4 -ipady 4
    grid columnconfigure $top.gridF3 2 -weight 1

    grid $top.combL x -sticky "w" -in $top.gridF4
    grid $top.combT $top.combS -sticky "nsew" -in $top.gridF4
    grid rowconfigure $top.gridF4 1 -weight 1
    grid columnconfigure $top.gridF4 0 -weight 1

    grid $top.okB $top.applyB x $top.resetB x $top.dismissB -sticky "ew"\
	    -in $top.gridF5 -pady $comb_control($id,pady)
    grid columnconfigure $top.gridF5 2 -weight 1
    grid columnconfigure $top.gridF5 4 -weight 1

    grid $top.gridF1 -sticky "ew" -padx $comb_control($id,padx) -pady $comb_control($id,pady)
    grid $top.gridF2 -sticky "ew" -padx $comb_control($id,padx) -pady $comb_control($id,pady)
    grid $top.gridF3 -sticky "ew" -padx $comb_control($id,padx) -pady $comb_control($id,pady)
    grid $top.gridF4 -sticky "nsew" -padx $comb_control($id,padx) -pady $comb_control($id,pady)
    grid $top.gridF5 -sticky "ew" -padx $comb_control($id,padx) -pady $comb_control($id,pady)
    grid rowconfigure $top 3 -weight 1
    grid columnconfigure $top 0 -weight 1

    bind $top.nameE <Return> "comb_reset $id; break"

    set pxy [winfo pointerxy $top]
    set x [lindex $pxy 0]
    set y [lindex $pxy 1]
    wm geometry $top +$x+$y
    wm title $top "Combination Editor ($id)"
}

proc comb_ok { id top } {
    comb_apply $id
    comb_dismiss $id $top
}

proc comb_apply { id } {
    global mged_gui
    global comb_control

    set top .$id.comb
    set comb_control($id,comb) [$top.combT get 0.0 end]

# apply the parameters in the shader frame to the shader string
#    if { $comb_control($id,shader) != "" } then {
#	do_shader_apply comb_control($id,shader) $id
#    }

    if {$comb_control($id,isRegion)} {
	if {$comb_control($id,id) < 0} {
	    cad_dialog .$id.combDialog $mged_gui($id,screen)\
		    "Bad region id!"\
		    "Region id must be >= 0"\
		    "" 0 OK
	    return
	}

	if {$comb_control($id,air) < 0} {
	    cad_dialog .$id.combDialog $mged_gui($id,screen)\
		    "Bad air code!"\
		    "Air code must be >= 0"\
		    "" 0 OK
	    return
	}

	if {$comb_control($id,id) == 0 && $comb_control($id,air) == 0} {
	    cad_dialog .$id.combDialog $mged_gui($id,screen)\
		    "Warning: both region id and air code are 0"\
		    "Warning: both region id and air code are 0"\
		    "" 0 OK
	}

	set result [catch {put_comb $comb_control($id,name) $comb_control($id,isRegion) \
		$comb_control($id,id) $comb_control($id,air) $comb_control($id,material) \
		$comb_control($id,los) $comb_control($id,color) $comb_control($id,shader) \
		$comb_control($id,inherit) $comb_control($id,comb)} comb_error]

	if {$result} {
	    return $comb_error
	}

	return
    }

    set result [catch {put_comb $comb_control($id,name) $comb_control($id,isRegion)\
	    $comb_control($id,color) $comb_control($id,shader) $comb_control($id,inherit)\
	    $comb_control($id,comb)} comb_error]

    if {$result} {
	return $comb_error
    }
}

proc comb_reset { id } {
    global mged_gui
    global comb_control

    set top .$id.comb

    if {$comb_control($id,name) == ""} {
	cad_dialog .$id.combDialog $mged_gui($id,screen)\
		"You must specify a region/combination name!"\
		"You must specify a region/combination name!"\
		"" 0 OK
	return
    }

    set save_isRegion $comb_control($id,isRegion)
    set result [catch {get_comb $comb_control($id,name)} comb_defs]
    if {$result == 1} {
	cad_dialog .$id.combDialog $mged_gui($id,screen)\
		"comb_reset: Error"\
		$comb_defs\
		"" 0 OK
	return
    }

    set comb_control($id,isRegion) [lindex $comb_defs 1]

    if {$comb_control($id,isRegion) == "Yes"} {
	set comb_control($id,id) [lindex $comb_defs 2]
	set comb_control($id,air) [lindex $comb_defs 3]
	set comb_control($id,material) [lindex $comb_defs 4]
	set comb_control($id,los) [lindex $comb_defs 5]
	set comb_control($id,color) [lindex $comb_defs 6]
	set comb_control($id,shader) [lindex $comb_defs 7]
	set comb_control($id,inherit) [lindex $comb_defs 8]
	set comb_control($id,comb) [lindex $comb_defs 9]
    } else {
	set comb_control($id,color) [lindex $comb_defs 2]
	set comb_control($id,shader) [lindex $comb_defs 3]
	set comb_control($id,inherit) [lindex $comb_defs 4]
	set comb_control($id,comb) [lindex $comb_defs 5]
    }

    color_entry_update $top color $comb_control($id,color)
    $top.combT delete 0.0 end
    $top.combT insert end $comb_control($id,comb)

    if {$save_isRegion != $comb_control($id,isRegion)} {
	comb_toggle_isRegion $id
    }

    if { [llength $comb_control($id,shader)] > 0 } {
	set comb_control($id,shader_gui) [do_shader comb_control($id,shader) $id \
		$comb_control($id,shader_frame)]
	grid $comb_control($id,shader_frame) \
		-row 1 \
		-sticky "ew" \
		-in $top.gridF2 \
		-padx $comb_control($id,padx) \
		-pady $comb_control($id,pady)
    } else {
	grid forget $comb_control($id,shader_frame)
	catch { destroy $comb_control($id,shader_gui) }
    }
}

proc comb_dismiss { id top } {
    global comb_control

    catch { destroy $top }
    catch { destroy $comb_control($id,shader_gui) }
}

proc comb_toggle_isRegion { id } {
    global comb_control

    set top .$id.comb
    grid remove $top.gridF1

    if {$comb_control($id,isRegion) == "Yes"} {
	grid forget $top.nameL $top.nameF $top.colorL $top.colorF

	frame $top.idF -relief sunken -bd 2
	label $top.idL -text "Region Id" -anchor w
	entry $top.idE -relief flat -width 12 -textvar comb_control($id,id)
	grid $top.idE -sticky "ew" -in $top.idF
	grid columnconfigure $top.idF 0 -weight 1

	frame $top.airF -relief sunken -bd 2
	label $top.airL -text "Air Code" -anchor w
	entry $top.airE -relief flat -width 12 -textvar comb_control($id,air)
	grid $top.airE -sticky "ew" -in $top.airF
	grid columnconfigure $top.airF 0 -weight 1

	frame $top.materialF -relief sunken -bd 2
	label $top.materialL -text "Material" -anchor w
	entry $top.materialE -relief flat -width 12 -textvar comb_control($id,material)
	grid $top.materialE -sticky "ew" -in $top.materialF
	grid columnconfigure $top.materialF 0 -weight 1

	frame $top.losF -relief sunken -bd 2
	label $top.losL -text "LOS" -anchor w
	entry $top.losE -relief flat -width 12 -textvar comb_control($id,los)
	grid $top.losE -sticky "ew" -in $top.losF
	grid columnconfigure $top.losF 0 -weight 1

	grid $top.nameL $top.nameF x $top.idL $top.idF \
		-sticky "ew" -in $top.gridF1 -pady $comb_control($id,pady)
	grid $top.colorL $top.colorF x $top.airL $top.airF \
		-sticky "ew" -in $top.gridF1 -pady $comb_control($id,pady)
	grid $top.materialL $top.materialF x $top.losL $top.losF \
		-sticky "ew" -in $top.gridF1 -pady $comb_control($id,pady)
    } else {
	grid forget $top.nameL $top.nameF \
		$top.idL $top.idF \
		$top.colorL $top.colorF \
		$top.airL $top.airF \
		$top.materialL $top.materialF \
		$top.losL $top.losF

	destroy $top.idL $top.idE $top.idF \
		$top.airL $top.airE $top.airF \
		$top.materialL $top.materialE $top.materialF\
		$top.losL $top.losE $top.losF

	grid $top.nameL $top.nameF x $top.colorL $top.colorF \
		-sticky "ew" -row 0 -in $top.gridF1 -pady $comb_control($id,pady)
    }

    grid $top.gridF1 -row 0
}

proc comb_shader_gui { id shader_type } {
    global comb_control

    set top .$id.comb
    set current_shader_type [lindex $comb_control($id,shader) 0]

    if { $current_shader_type != $shader_type } {
	set comb_control($id,shader) $shader_type
    }

    set comb_control($id,shader_gui) [do_shader comb_control($id,shader) $id \
	    $comb_control($id,shader_frame)]
    grid $comb_control($id,shader_frame) \
	    -row 1 \
	    -sticky "ew" \
	    -in $top.gridF2 \
	    -padx $comb_control($id,padx) \
	    -pady $comb_control($id,pady)
}

# This is being called after a call to set_shader_params, which
# may build the shader GUI.
proc comb_callback { id master } {
    global comb_control

    set material [lindex $comb_control($id,shader) 0]

    if [is_good_shader $material] {
	# If not managing shader frame, then manage it.
	if { [llength [grid slaves $master] ] <= 1 } {
	    grid $comb_control($id,shader_frame) \
		    -row 1 \
		    -sticky "ew" \
		    -in $master \
		    -padx $comb_control($id,padx) \
		    -pady $comb_control($id,pady)
	}

	return
    }

    # We have a bad material type. If managing
    # the shader frame, then unmanage it.
    if { [llength [grid slaves $master] ] == 2 } {
	grid forget $comb_control($id,shader_frame)
	catch { destroy $comb_control($id,shader_gui) }
    }
}

#proc comb_select_material { id } {
#    global mged_gui
#    global comb_control
#
#    set top .$id.sel_material
#
#    if [winfo exists $top] {
#	raise $top
#	return
#    }
#
#    toplevel $top -screen $mged_gui($id,screen)
#
#    frame $top.gridF1
#    frame $top.gridF3
#    frame $top.gridF4 -relief groove -bd 2
#
#    listbox $top.materialLB -selectmode single -yscrollcommand "$top.materialS set"
#    scrollbar $top.materialS -relief flat -command "$top.materialLB yview"
#
#    label $top.materialL -text "Material List:" -anchor w
#    entry $top.materialE -width 12 -textvar comb_control($id,material_list)
#
#    button $top.resetB -relief raised -text "Reset"\
#	    -command "load_material $id"
#    button $top.dismissB -relief raised -text "Dismiss"\
#	    -command "catch { destroy $top }"
#
#    grid $top.materialLB $top.materialS -sticky "nsew" -in $top.gridF
#    grid rowconfigure $top.gridF1 0 -weight 1
#    grid columnconfigure $top.gridF1 0 -weight 1
#
#    grid $top.materialL x -sticky "ew" -in $top.gridF3
#    grid $top.materialE $top.resetB -sticky "nsew" -in $top.gridF3
#    grid columnconfigure $top.gridF3 0 -weight 1
#
#    grid $top.dismissB -in $top.gridF4 -pady $comb_control($id,pady)
#
#    grid $top.gridF1 -sticky "nsew" -padx $comb_control($id,padx) -pady $comb_control($id,pady)
#    grid $top.gridF3 -sticky "ew" -padx $comb_control($id,padx) -pady $comb_control($id,pady)
#    grid $top.gridF4 -sticky "ew"
#    grid rowconfigure $top 0 -weight 1
#    grid columnconfigure $top 0 -weight 1
#
#    wm title $top "Select Material"
#}
#
#proc load_material { id } {
#    global comb_control
#}
