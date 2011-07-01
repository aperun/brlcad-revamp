#ifndef  S_HEADER_SCHEMA_INIT_CC
#define  S_HEADER_SCHEMA_INIT_CC

/*
* NIST STEP Editor Class Library
* cleditor/s_HEADER_SCHEMA.inline.cc
* April 1997
* David Sauder
* K. C. Morris

* Development of this software was funded by the United States Government,
* and is not subject to copyright.
*/

// This file was generated by fedex_plus.  You probably don't want to edit
// it since your modifications will be lost if fedex_plus is used to
// regenerate it.

/* $Id: s_HEADER_SCHEMA.init.cc,v 3.0.1.5 1997/11/05 22:11:43 sauderd DP3.1 $ */

#include <s_HEADER_SCHEMA.h> 
#include <ExpDict.h>
#include <STEPattribute.h>
#include <Registry.h>
void 
HeaderSchemaInit (Registry & reg)
{
	SchemaDescriptor *  z_HEADER_SCHEMA = new SchemaDescriptor("Header_Schema");
	reg.AddSchema (*z_HEADER_SCHEMA);
	HEADER_SCHEMAt_SCHEMA_NAME = new TypeDescriptor (
		  "Schema_Name",	// Name
		  STRING_TYPE,	// FundamentalType
		  z_HEADER_SCHEMA, // Originating Schema
		  "STRING");	// Description
	HEADER_SCHEMAe_FILE_IDENTIFICATION = new EntityDescriptor("File_Identification",z_HEADER_SCHEMA,
LFalse, LFalse, 
(Creator) create_s_N279_file_identification);
	HEADER_SCHEMAe_IMP_LEVEL = new EntityDescriptor("Imp_Level",z_HEADER_SCHEMA,
LFalse, LFalse, 
(Creator) create_s_N279_imp_level);
	HEADER_SCHEMAe_FILE_NAME = new EntityDescriptor("File_Name",z_HEADER_SCHEMA,
LFalse, LFalse, 
(Creator) create_p21DIS_File_name);
	HEADER_SCHEMAe_FILE_DESCRIPTION = new EntityDescriptor("File_Description",z_HEADER_SCHEMA,
LFalse, LFalse, 
(Creator) create_p21DIS_File_description);
	HEADER_SCHEMAe_S_CLASSIFICATION = new EntityDescriptor("S_Classification",z_HEADER_SCHEMA,
LFalse, LFalse, 
(Creator) create_s_N279_classification);
	HEADER_SCHEMAe_S_FILE_DESCRIPTION = new EntityDescriptor("S_File_Description",z_HEADER_SCHEMA,
LFalse, LFalse, 
(Creator) create_s_N279_file_description);
	HEADER_SCHEMAe_MAXSIG = new EntityDescriptor("Maxsig",z_HEADER_SCHEMA,
LFalse, LFalse, 
(Creator) create_p21DIS_Maxsig);
	HEADER_SCHEMAe_CLASSIFICATION = new EntityDescriptor("Classification",z_HEADER_SCHEMA,
LFalse, LFalse, 
(Creator) create_p21DIS_Classification);
	HEADER_SCHEMAe_FILE_SCHEMA = new EntityDescriptor("File_Schema",z_HEADER_SCHEMA,
LFalse, LFalse, 
(Creator) create_p21DIS_File_schema);
	HEADER_SCHEMAt_SCHEMA_NAME->ReferentType(t_sdaiSTRING);
	reg.AddType (*HEADER_SCHEMAt_SCHEMA_NAME);
	reg.AddEntity (*HEADER_SCHEMAe_FILE_IDENTIFICATION);
	a_0FILE_NAME = new AttrDescriptor("file_name",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_IDENTIFICATION);
	HEADER_SCHEMAe_FILE_IDENTIFICATION->AddExplicitAttr(a_0FILE_NAME);
	a_1DATE = new AttrDescriptor("date",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_IDENTIFICATION);
	HEADER_SCHEMAe_FILE_IDENTIFICATION->AddExplicitAttr(a_1DATE);
	TypeDescriptor * t_0 = new TypeDescriptor;
	t_0->FundamentalType(AGGREGATE_TYPE);
	t_0->Description("LIST of STRING");
	t_0->OriginatingSchema(z_HEADER_SCHEMA);
	t_0->ReferentType(t_sdaiSTRING);
	a_2AUTHOR = new AttrDescriptor("author",t_0,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_IDENTIFICATION);
	HEADER_SCHEMAe_FILE_IDENTIFICATION->AddExplicitAttr(a_2AUTHOR);
	TypeDescriptor * t_1 = new TypeDescriptor;
	t_1->FundamentalType(AGGREGATE_TYPE);
	t_1->Description("LIST of STRING");
	t_1->OriginatingSchema(z_HEADER_SCHEMA);
	t_1->ReferentType(t_sdaiSTRING);
	a_3ORGANIZATION = new AttrDescriptor("organization",t_1,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_IDENTIFICATION);
	HEADER_SCHEMAe_FILE_IDENTIFICATION->AddExplicitAttr(a_3ORGANIZATION);
	a_4PREPROCESSOR_VERSION = new AttrDescriptor("preprocessor_version",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_IDENTIFICATION);
	HEADER_SCHEMAe_FILE_IDENTIFICATION->AddExplicitAttr(a_4PREPROCESSOR_VERSION);
	a_5ORIGINATING_SYSTEM = new AttrDescriptor("originating_system",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_IDENTIFICATION);
	HEADER_SCHEMAe_FILE_IDENTIFICATION->AddExplicitAttr(a_5ORIGINATING_SYSTEM);
	reg.AddEntity (*HEADER_SCHEMAe_IMP_LEVEL);
	a_6IMPLEMENTATION_LEVEL = new AttrDescriptor("implementation_level",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_IMP_LEVEL);
	HEADER_SCHEMAe_IMP_LEVEL->AddExplicitAttr(a_6IMPLEMENTATION_LEVEL);
	reg.AddEntity (*HEADER_SCHEMAe_FILE_NAME);
	a_7NAME = new AttrDescriptor("name",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_NAME);
	HEADER_SCHEMAe_FILE_NAME->AddExplicitAttr(a_7NAME);
	a_8TIME_STAMP = new AttrDescriptor("time_stamp",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_NAME);
	HEADER_SCHEMAe_FILE_NAME->AddExplicitAttr(a_8TIME_STAMP);
	TypeDescriptor * t_2 = new TypeDescriptor;
	t_2->FundamentalType(AGGREGATE_TYPE);
	t_2->Description("LIST [1:?] of STRING (256)");
	t_2->OriginatingSchema(z_HEADER_SCHEMA);
	t_2->ReferentType(t_sdaiSTRING);
	a_9AUTHOR = new AttrDescriptor("author",t_2,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_NAME);
	HEADER_SCHEMAe_FILE_NAME->AddExplicitAttr(a_9AUTHOR);
	TypeDescriptor * t_3 = new TypeDescriptor;
	t_3->FundamentalType(AGGREGATE_TYPE);
	t_3->Description("LIST [1:?] of STRING (256)");
	t_3->OriginatingSchema(z_HEADER_SCHEMA);
	t_3->ReferentType(t_sdaiSTRING);
	a_10ORGANIZATION = new AttrDescriptor("organization",t_3,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_NAME);
	HEADER_SCHEMAe_FILE_NAME->AddExplicitAttr(a_10ORGANIZATION);
/*
	a_11STEP_VERSION = new AttrDescriptor("step_version",t_sdaiSTRING,LFalse,LFalse,AttrType_Explicit,*HEADER_SCHEMAe_FILE_NAME);
	HEADER_SCHEMAe_FILE_NAME->AddExplicitAttr(a_11STEP_VERSION);
*/
	a_12PREPROCESSOR_VERSION = new AttrDescriptor("preprocessor_version",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_NAME);
	HEADER_SCHEMAe_FILE_NAME->AddExplicitAttr(a_12PREPROCESSOR_VERSION);
	a_13ORIGINATING_SYSTEM = new AttrDescriptor("originating_system",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_NAME);
	HEADER_SCHEMAe_FILE_NAME->AddExplicitAttr(a_13ORIGINATING_SYSTEM);
	a_14AUTHORISATION = new AttrDescriptor("authorisation",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_NAME);
	HEADER_SCHEMAe_FILE_NAME->AddExplicitAttr(a_14AUTHORISATION);
	reg.AddEntity (*HEADER_SCHEMAe_FILE_DESCRIPTION);
	TypeDescriptor * t_4 = new TypeDescriptor;
	t_4->FundamentalType(AGGREGATE_TYPE);
	t_4->Description("LIST [1:?] of STRING (256)");
	t_4->OriginatingSchema(z_HEADER_SCHEMA);
	t_4->ReferentType(t_sdaiSTRING);
	a_15DESCRIPTION = new AttrDescriptor("description",t_4,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_DESCRIPTION);
	HEADER_SCHEMAe_FILE_DESCRIPTION->AddExplicitAttr(a_15DESCRIPTION);
	a_16IMPLEMENTATION_LEVEL = new AttrDescriptor("implementation_level",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_DESCRIPTION);
	HEADER_SCHEMAe_FILE_DESCRIPTION->AddExplicitAttr(a_16IMPLEMENTATION_LEVEL);
	reg.AddEntity (*HEADER_SCHEMAe_S_CLASSIFICATION);
	a_17SECURITY_CLASSIFICATION = new AttrDescriptor("security_classification",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_S_CLASSIFICATION);
	HEADER_SCHEMAe_S_CLASSIFICATION->AddExplicitAttr(a_17SECURITY_CLASSIFICATION);
	reg.AddEntity (*HEADER_SCHEMAe_S_FILE_DESCRIPTION);
	a_18DESCRIPTION = new AttrDescriptor("description",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_S_FILE_DESCRIPTION);
	HEADER_SCHEMAe_S_FILE_DESCRIPTION->AddExplicitAttr(a_18DESCRIPTION);
	reg.AddEntity (*HEADER_SCHEMAe_MAXSIG);
	a_19MAXIMUM_SIGNIFICANT_DIGIT = new AttrDescriptor("maximum_significant_digit",t_sdaiINTEGER,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_MAXSIG);
	HEADER_SCHEMAe_MAXSIG->AddExplicitAttr(a_19MAXIMUM_SIGNIFICANT_DIGIT);
	reg.AddEntity (*HEADER_SCHEMAe_CLASSIFICATION);
	a_20SECURITY_CLASSIFICATION = new AttrDescriptor("security_classification",t_sdaiSTRING,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_CLASSIFICATION);
	HEADER_SCHEMAe_CLASSIFICATION->AddExplicitAttr(a_20SECURITY_CLASSIFICATION);
	reg.AddEntity (*HEADER_SCHEMAe_FILE_SCHEMA);
	TypeDescriptor * t_5 = new TypeDescriptor;
	t_5->FundamentalType(AGGREGATE_TYPE);
	t_5->Description("LIST [1:?] of Schema_Name");
	t_5->OriginatingSchema(z_HEADER_SCHEMA);
	t_5->ReferentType(t_sdaiSTRING);
	a_21SCHEMA_IDENTIFIERS = new AttrDescriptor("schema_identifiers",t_5,
LFalse,LFalse,
AttrType_Explicit,*HEADER_SCHEMAe_FILE_SCHEMA);
	HEADER_SCHEMAe_FILE_SCHEMA->AddExplicitAttr(a_21SCHEMA_IDENTIFIERS);

}
#endif
