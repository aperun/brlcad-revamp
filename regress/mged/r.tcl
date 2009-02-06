puts "*** Testing 'r' command ***"

if {![info exists make_primitives_list]} {  
   source regression_resources.tcl
}

in_sph r 1
in_sph r 2
in_sph r 3
comb r_comb1.c u r_sph1.s u r_sph2.s u r_sph3.s
comb r_comb2.c u r_sph1.s + r_sph2.s + r_sph3.s
r r1.r u r_sph1.s u r_sph2.s u r_sph3.s
r r2.r u r_sph1.s u r_comb1.c
r r3.r u r_comb1.c u r_comb2.c

puts "*** 'r' testing completed ***\n"
