# This file was generated by fedex_python.  You probably don't want to edit
# it since your modifications will be lost if fedex_plus is used to
# regenerate it.
import sys

from SCL.SCLBase import *
from SCL.SimpleDataTypes import *
from SCL.ConstructedDataTypes import *
from SCL.AggregationDataTypes import *
from SCL.TypeChecker import check_type
from SCL.Builtin import *
from SCL.Rules import *

schema_name = 'test_single_inheritance_multi_level'

schema_scope = sys.modules[__name__]

# Defined datatype length_measure
class length_measure(REAL):
	def __init__(self,*kargs):
		pass

# Defined datatype label
class label(STRING):
	def __init__(self,*kargs):
		pass

# Defined datatype point
class point(REAL):
	def __init__(self,*kargs):
		pass


####################
 # ENTITY shape #
####################
class shape(BaseEntityClass):
	'''Entity shape definition.

	:param item_name
	:type item_name:label

	:param number_of_sides
	:type number_of_sides:INTEGER
	'''
	def __init__( self , item_name,number_of_sides, ):
		self.item_name = item_name
		self.number_of_sides = number_of_sides

	@apply
	def item_name():
		def fget( self ):
			return self._item_name
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument item_name is mantatory and can not be set to None')
			if not check_type(value,label):
				self._item_name = label(value)
			else:
				self._item_name = value
		return property(**locals())

	@apply
	def number_of_sides():
		def fget( self ):
			return self._number_of_sides
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument number_of_sides is mantatory and can not be set to None')
			if not check_type(value,INTEGER):
				self._number_of_sides = INTEGER(value)
			else:
				self._number_of_sides = value
		return property(**locals())

####################
 # ENTITY subshape #
####################
class subshape(shape):
	'''Entity subshape definition.
	'''
	def __init__( self , inherited0__item_name , inherited1__number_of_sides ,  ):
		shape.__init__(self , inherited0__item_name , inherited1__number_of_sides , )

####################
 # ENTITY rectangle #
####################
class rectangle(subshape):
	'''Entity rectangle definition.

	:param height
	:type height:length_measure

	:param width
	:type width:length_measure
	'''
	def __init__( self , inherited0__item_name , inherited1__number_of_sides , height,width, ):
		subshape.__init__(self , inherited0__item_name , inherited1__number_of_sides , )
		self.height = height
		self.width = width

	@apply
	def height():
		def fget( self ):
			return self._height
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument height is mantatory and can not be set to None')
			if not check_type(value,length_measure):
				self._height = length_measure(value)
			else:
				self._height = value
		return property(**locals())

	@apply
	def width():
		def fget( self ):
			return self._width
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument width is mantatory and can not be set to None')
			if not check_type(value,length_measure):
				self._width = length_measure(value)
			else:
				self._width = value
		return property(**locals())
