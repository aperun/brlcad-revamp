# dialog.tcl --
#
# This file defines the procedure tk_dialog, which creates a dialog
# box containing a bitmap, a message, and one or more buttons.
#
# RCS: @(#) $Id$
#
# Copyright (c) 1992-1993 The Regents of the University of California.
# Copyright (c) 1994-1997 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#
# ::tk_dialog:
#
# This procedure displays a dialog box, waits for a button in the dialog
# to be invoked, then returns the index of the selected button.  If the
# dialog somehow gets destroyed, -1 is returned.
#
# Arguments:
# w -		Window to use for dialog top-level.
# title -	Title to display in dialog's decorative frame.
# text -	Message to display in dialog.
# bitmap -	Bitmap to display in dialog (empty string means none).
# default -	Index of button that is to display the default ring
#		(-1 means none).
# args -	One or more strings to display in buttons across the
#		bottom of the dialog box.

proc ::tk_dialog {w title text bitmap default args} {
    global tcl_platform
    variable ::tk::Priv

    # Check that $default was properly given
    if {[string is int $default]} {
	if {$default >= [llength $args]} {
	    return -code error "default button index greater than number of\
		    buttons specified for tk_dialog"
	}
    } elseif {[string equal {} $default]} {
	set default -1
    } else {
	set default [lsearch -exact $args $default]
    }

    # 1. Create the top-level window and divide it into top
    # and bottom parts.

    catch {destroy $w}
    toplevel $w -class Dialog
    wm title $w $title
    wm iconname $w Dialog
    wm protocol $w WM_DELETE_WINDOW { }

    # Dialog boxes should be transient with respect to their parent,
    # so that they will always stay on top of their parent window.  However,
    # some window managers will create the window as withdrawn if the parent
    # window is withdrawn or iconified.  Combined with the grab we put on the
    # window, this can hang the entire application.  Therefore we only make
    # the dialog transient if the parent is viewable.
    #
    if {[winfo viewable [winfo toplevel [winfo parent $w]]] } {
	wm transient $w [winfo toplevel [winfo parent $w]]
    }    

    if {[string equal $tcl_platform(platform) "macintosh"]
	    || [string equal [tk windowingsystem] "aqua"]} {
	::tk::unsupported::MacWindowStyle style $w dBoxProc
    }

    frame $w.bot
    frame $w.top
    if {[string equal [tk windowingsystem] "x11"]} {
	$w.bot configure -relief raised -bd 1
	$w.top configure -relief raised -bd 1
    }
    pack $w.bot -side bottom -fill both
    pack $w.top -side top -fill both -expand 1

    # 2. Fill the top part with bitmap and message (use the option
    # database for -wraplength and -font so that they can be
    # overridden by the caller).

    option add *Dialog.msg.wrapLength 3i widgetDefault
    if {[string equal $tcl_platform(platform) "macintosh"]
	    || [string equal [tk windowingsystem] "aqua"]} {
	option add *Dialog.msg.font system widgetDefault
    } else {
	option add *Dialog.msg.font {Times 12} widgetDefault
    }

    label $w.msg -justify left -text $text
    pack $w.msg -in $w.top -side right -expand 1 -fill both -padx 3m -pady 3m
    if {[string compare $bitmap ""]} {
	if {([string equal $tcl_platform(platform) "macintosh"]
	     || [string equal [tk windowingsystem] "aqua"]) &&\
		[string equal $bitmap "error"]} {
	    set bitmap "stop"
	}
	label $w.bitmap -bitmap $bitmap
	pack $w.bitmap -in $w.top -side left -padx 3m -pady 3m
    }

    # 3. Create a row of buttons at the bottom of the dialog.

    set i 0
    foreach but $args {
	button $w.button$i -text $but -command [list set ::tk::Priv(button) $i]
	if {$i == $default} {
	    $w.button$i configure -default active
	} else {
	    $w.button$i configure -default normal
	}
	grid $w.button$i -in $w.bot -column $i -row 0 -sticky ew \
		-padx 10 -pady 4
	grid columnconfigure $w.bot $i
	# We boost the size of some Mac buttons for l&f
	if {[string equal $tcl_platform(platform) "macintosh"]
	    || [string equal [tk windowingsystem] "aqua"]} {
	    set tmp [string tolower $but]
	    if {[string equal $tmp "ok"] || [string equal $tmp "cancel"]} {
		grid columnconfigure $w.bot $i -minsize [expr {59 + 20}]
	    }
	}
	incr i
    }

    # 4. Create a binding for <Return> on the dialog if there is a
    # default button.

    if {$default >= 0} {
	bind $w <Return> "
	[list $w.button$default] configure -state active -relief sunken
	update idletasks
	after 100
	set ::tk::Priv(button) $default
	"
    }

    # 5. Create a <Destroy> binding for the window that sets the
    # button variable to -1;  this is needed in case something happens
    # that destroys the window, such as its parent window being destroyed.

    bind $w <Destroy> {set ::tk::Priv(button) -1}

    # 6. Withdraw the window, then update all the geometry information
    # so we know how big it wants to be, then center the window in the
    # display and de-iconify it.

    wm withdraw $w
    update idletasks
    set x [expr {[winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 \
	    - [winfo vrootx [winfo parent $w]]}]
    set y [expr {[winfo screenheight $w]/2 - [winfo reqheight $w]/2 \
	    - [winfo vrooty [winfo parent $w]]}]
    wm geom $w +$x+$y
    wm deiconify $w

    # 7. Set a grab and claim the focus too.

    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {[string compare $oldGrab ""]} {
	set grabStatus [grab status $oldGrab]
    }
    grab $w
    if {$default >= 0} {
	focus $w.button$default
    } else {
	focus $w
    }

    # 8. Wait for the user to respond, then restore the focus and
    # return the index of the selected button.  Restore the focus
    # before deleting the window, since otherwise the window manager
    # may take the focus away so we can't redirect it.  Finally,
    # restore any grab that was in effect.

    vwait ::tk::Priv(button)
    catch {focus $oldFocus}
    catch {
	# It's possible that the window has already been destroyed,
	# hence this "catch".  Delete the Destroy handler so that
	# Priv(button) doesn't get reset by it.

	bind $w <Destroy> {}
	destroy $w
    }
    if {[string compare $oldGrab ""]} {
      if {[string compare $grabStatus "global"]} {
	    grab $oldGrab
      } else {
          grab -global $oldGrab
	}
    }
    return $Priv(button)
}
