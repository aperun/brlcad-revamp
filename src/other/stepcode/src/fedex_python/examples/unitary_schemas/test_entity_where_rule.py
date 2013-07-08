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

schema_name = 'test_entity_where_rule'

schema_scope = sys.modules[__name__]

# Defined datatype label
class label(STRING):
	def __init__(self,*kargs):
		pass


####################
 # ENTITY unit_vector #
####################
class unit_vector(BaseEntityClass):
	'''Entity unit_vector definition.

	:param a
	:type a:REAL

	:param b
	:type b:REAL

	:param c
	:type c:REAL
	'''
	def __init__( self , a,b,c, ):
		self.a = a
		self.b = b
		self.c = c

	@apply
	def a():
		def fget( self ):
			return self._a
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument a is mantatory and can not be set to None')
			if not check_type(value,REAL):
				self._a = REAL(value)
			else:
				self._a = value
		return property(**locals())

	@apply
	def b():
		def fget( self ):
			return self._b
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument b is mantatory and can not be set to None')
			if not check_type(value,REAL):
				self._b = REAL(value)
			else:
				self._b = value
		return property(**locals())

	@apply
	def c():
		def fget( self ):
			return self._c
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument c is mantatory and can not be set to None')
			if not check_type(value,REAL):
				self._c = REAL(value)
			else:
				self._c = value
		return property(**locals())
	def length_1(self):
		eval_length_1_wr = ((((self.a  **  2)  +  (self.b  **  2))  +  (self.c  **  2))  ==  1)
		if not eval_length_1_wr:
			raise AssertionError('Rule length_1 violated')
		else:
			return eval_length_1_wr


####################
 # ENTITY address #
####################
class address(BaseEntityClass):
	'''Entity address definition.

	:param internal_location
	:type internal_location:label

	:param street_number
	:type street_number:label

	:param street
	:type street:label

	:param postal_box
	:type postal_box:label

	:param town
	:type town:label

	:param region
	:type region:label

	:param postal_code
	:type postal_code:label

	:param country
	:type country:label

	:param facsimile_number
	:type facsimile_number:label

	:param telephone_number
	:type telephone_number:label

	:param electronic_mail_address
	:type electronic_mail_address:label

	:param telex_number
	:type telex_number:label
	'''
	def __init__( self , internal_location,street_number,street,postal_box,town,region,postal_code,country,facsimile_number,telephone_number,electronic_mail_address,telex_number, ):
		self.internal_location = internal_location
		self.street_number = street_number
		self.street = street
		self.postal_box = postal_box
		self.town = town
		self.region = region
		self.postal_code = postal_code
		self.country = country
		self.facsimile_number = facsimile_number
		self.telephone_number = telephone_number
		self.electronic_mail_address = electronic_mail_address
		self.telex_number = telex_number

	@apply
	def internal_location():
		def fget( self ):
			return self._internal_location
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._internal_location = label(value)
				else:
					self._internal_location = value
			else:
				self._internal_location = value
		return property(**locals())

	@apply
	def street_number():
		def fget( self ):
			return self._street_number
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._street_number = label(value)
				else:
					self._street_number = value
			else:
				self._street_number = value
		return property(**locals())

	@apply
	def street():
		def fget( self ):
			return self._street
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._street = label(value)
				else:
					self._street = value
			else:
				self._street = value
		return property(**locals())

	@apply
	def postal_box():
		def fget( self ):
			return self._postal_box
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._postal_box = label(value)
				else:
					self._postal_box = value
			else:
				self._postal_box = value
		return property(**locals())

	@apply
	def town():
		def fget( self ):
			return self._town
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._town = label(value)
				else:
					self._town = value
			else:
				self._town = value
		return property(**locals())

	@apply
	def region():
		def fget( self ):
			return self._region
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._region = label(value)
				else:
					self._region = value
			else:
				self._region = value
		return property(**locals())

	@apply
	def postal_code():
		def fget( self ):
			return self._postal_code
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._postal_code = label(value)
				else:
					self._postal_code = value
			else:
				self._postal_code = value
		return property(**locals())

	@apply
	def country():
		def fget( self ):
			return self._country
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._country = label(value)
				else:
					self._country = value
			else:
				self._country = value
		return property(**locals())

	@apply
	def facsimile_number():
		def fget( self ):
			return self._facsimile_number
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._facsimile_number = label(value)
				else:
					self._facsimile_number = value
			else:
				self._facsimile_number = value
		return property(**locals())

	@apply
	def telephone_number():
		def fget( self ):
			return self._telephone_number
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._telephone_number = label(value)
				else:
					self._telephone_number = value
			else:
				self._telephone_number = value
		return property(**locals())

	@apply
	def electronic_mail_address():
		def fget( self ):
			return self._electronic_mail_address
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._electronic_mail_address = label(value)
				else:
					self._electronic_mail_address = value
			else:
				self._electronic_mail_address = value
		return property(**locals())

	@apply
	def telex_number():
		def fget( self ):
			return self._telex_number
		def fset( self, value ):
			if value != None: # OPTIONAL attribute
				if not check_type(value,label):
					self._telex_number = label(value)
				else:
					self._telex_number = value
			else:
				self._telex_number = value
		return property(**locals())
	def wr1(self):
		eval_wr1_wr = (((((((((((EXISTS(self.internal_location)  or  EXISTS(self.street_number))  or  EXISTS(self.street))  or  EXISTS(self.postal_box))  or  EXISTS(self.town))  or  EXISTS(self.region))  or  EXISTS(self.postal_code))  or  EXISTS(self.country))  or  EXISTS(self.facsimile_number))  or  EXISTS(self.telephone_number))  or  EXISTS(self.electronic_mail_address))  or  EXISTS(self.telex_number))
		if not eval_wr1_wr:
			raise AssertionError('Rule wr1 violated')
		else:
			return eval_wr1_wr

