# Routines to check for cylic references

check_externs "db bu_get_value_by_keyword"

#
#	This routine returns a list of all the objects directly referenced
#	in the tree (as returned by "db get comb tree"). If the refs
#	argument is non-empty, then the returned list includes $refs
proc get_refs { refs tree } {
	set op [lindex $tree 0]
	if { [string compare $op "l"] == 0 } {
		# this is a leaf
		set leaf [lindex $tree 1]

		# if the leaf is not aleady in the list append it
		if { [lsearch -exact $refs $leaf] == -1 } {
			lappend refs $leaf
		}
	} else {
		# this is a binary node (+, -, or u)
		# recurse on each subtree
		set subtree [lindex $tree 1]
		set refs [get_refs $refs $subtree]
		set subtree [lindex $tree 2]
		set refs [get_refs $refs $subtree]
		
	}

	# return the final list
	return $refs
}

#	comb is a combination name
#	ancestors is a list of ancestors of comb
#	this routine descends "comb" looking for cyclic references
#	returns "1" if it finds a cycle, "0" otherwise
proc check_cycle { ancestors comb } {
	foreach anc $ancestors {
		if { [string compare $anc $comb] == 0 } {
			return 1
		}
	}

	# get the combination from disk
	set comb_all [concat type [db get $comb]]

	# if this is not really a combination, there cannot be a cycle
	if { [bu_get_value_by_keyword type $comb_all] != "comb" } {
		return 0
	}

	# add this combination to the list of ancestors
	lappend ancestors $comb

	# recurse for each combination referred to by this combination
	foreach desc [get_refs "" [bu_get_value_by_keyword tree $comb_all]] {
		if { [string compare [lindex [db get $desc] 0] "comb"] == 0 } {
			if { [check_cycle $ancestors $desc] == 1 } {
				return 1
			}
		}
	}

	# if we get here, no cycles were found
	return 0
}
