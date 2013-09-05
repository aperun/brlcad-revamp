#ifndef EXPPP_H
#define EXPPP_H
extern SCL_EXPPP_EXPORT int exppp_nesting_indent;    /* default nesting indent */
extern SCL_EXPPP_EXPORT int exppp_continuation_indent;   /* default nesting indent for */
/* continuation lines */
extern SCL_EXPPP_EXPORT int exppp_linelength;        /* leave some slop for closing */
/* parens.  \n is not included in */
/* this count either */
extern SCL_EXPPP_EXPORT bool exppp_rmpp;          /* if true, create rmpp */
extern SCL_EXPPP_EXPORT bool exppp_alphabetize;       /* if true, alphabetize */
extern SCL_EXPPP_EXPORT bool exppp_terse;         /* don't describe action to stdout */
extern SCL_EXPPP_EXPORT bool exppp_reference_info;    /* if true, add commentary */
/* about where things came from */
extern SCL_EXPPP_EXPORT bool exppp_preserve_comments; /* if true, preserve comments where */
/* possible */
extern SCL_EXPPP_EXPORT char * exppp_output_filename; /* force output filename */
extern SCL_EXPPP_EXPORT bool exppp_output_filename_reset; /* if true, force output filename */

SCL_EXPPP_EXPORT void EXPRESSout( Express e );

SCL_EXPPP_EXPORT void ENTITYout( Entity e );
SCL_EXPPP_EXPORT void EXPRout( Expression expr );
SCL_EXPPP_EXPORT void FUNCout( Function f );
SCL_EXPPP_EXPORT void PROCout( Procedure p );
SCL_EXPPP_EXPORT void RULEout( Rule r );
SCL_EXPPP_EXPORT char * SCHEMAout( Schema s );
SCL_EXPPP_EXPORT void SCHEMAref_out( Schema s );
SCL_EXPPP_EXPORT void STMTout( Statement s );
SCL_EXPPP_EXPORT void TYPEout( Type t );
SCL_EXPPP_EXPORT void TYPEhead_out( Type t );
SCL_EXPPP_EXPORT void TYPEbody_out( Type t );
SCL_EXPPP_EXPORT void WHEREout( Linked_List w );

SCL_EXPPP_EXPORT char * REFto_string( Dictionary refdict, Linked_List reflist, char * type, int level );
SCL_EXPPP_EXPORT char * ENTITYto_string( Entity e );
SCL_EXPPP_EXPORT char * SUBTYPEto_string( Expression e );
SCL_EXPPP_EXPORT char * EXPRto_string( Expression expr );
SCL_EXPPP_EXPORT char * FUNCto_string( Function f );
SCL_EXPPP_EXPORT char * PROCto_string( Procedure p );
SCL_EXPPP_EXPORT char * RULEto_string( Rule r );
SCL_EXPPP_EXPORT char * SCHEMAref_to_string( Schema s );
SCL_EXPPP_EXPORT char * STMTto_string( Statement s );
SCL_EXPPP_EXPORT char * TYPEto_string( Type t );
SCL_EXPPP_EXPORT char * TYPEhead_to_string( Type t );
SCL_EXPPP_EXPORT char * TYPEbody_to_string( Type t );
SCL_EXPPP_EXPORT char * WHEREto_string( Linked_List w );

SCL_EXPPP_EXPORT int REFto_buffer( Dictionary refdict, Linked_List reflist, char * type, int level, char * buffer, int length );
SCL_EXPPP_EXPORT int ENTITYto_buffer( Entity e, char * buffer, int length );
SCL_EXPPP_EXPORT int EXPRto_buffer( Expression e, char * buffer, int length );
SCL_EXPPP_EXPORT int FUNCto_buffer( Function e, char * buffer, int length );
SCL_EXPPP_EXPORT int PROCto_buffer( Procedure e, char * buffer, int length );
SCL_EXPPP_EXPORT int RULEto_buffer( Rule e, char * buffer, int length );
SCL_EXPPP_EXPORT int SCHEMAref_to_buffer( Schema s, char * buffer, int length );
SCL_EXPPP_EXPORT int STMTto_buffer( Statement s, char * buffer, int length );
SCL_EXPPP_EXPORT int TYPEto_buffer( Type t, char * buffer, int length );
SCL_EXPPP_EXPORT int TYPEhead_to_buffer( Type t, char * buffer, int length );
SCL_EXPPP_EXPORT int TYPEbody_to_buffer( Type t, char * buffer, int length );
SCL_EXPPP_EXPORT int WHEREto_buffer( Linked_List w, char * buffer, int length );
#endif
