# This file was generated by fedex_python.  You probably don't want to edit
# it since your modifications will be lost if fedex_plus is used to
# regenerate it.
from SCL.SCLBase import *
from SCL.SimpleDataTypes import *
from SCL.ConstructedDataTypes import *
from SCL.AggregationDataTypes import *
from SCL.TypeChecker import check_type
from SCL.Expr import *

# ENUMERATION TYPE hair_color
if (not 'bald' in globals().keys()):
	bald = 'bald'
if (not 'red' in globals().keys()):
	red = 'red'
hair_color = ENUMERATION(
	'bald',
	'red',
	)

# ENUMERATION TYPE favorite_color
if (not 'clear' in globals().keys()):
	clear = 'clear'
if (not 'red' in globals().keys()):
	red = 'red'
favorite_color = ENUMERATION(
	'clear',
	'red',
	)
