/* Generated by re2c */
#line 1 "config4d.f.re"
#define	NULL		((char*) 0)
#define	YYCTYPE		char
#define	YYCURSOR	p
#define	YYLIMIT		p
#define	YYMARKER	q
#define	YYFILL(n)

char *scan(char *p)
{
	char *q;

#line 15 "<stdout>"

	switch (YYGETSTATE()) {
	default: goto yy0;
	case 0: goto yyFillLabel0;
	case 1: goto yyFillLabel1;
	}
yyNext:
start:
yy0:
	YYSETSTATE(0);
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
yyFillLabel0:
	yych = *YYCURSOR;
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy2;
	default:	goto yy4;
	}
yy2:
	++YYCURSOR;
	yych = *YYCURSOR;
	goto yy7;
yy3:
#line 15 "config4d.f.re"
	{ return YYCURSOR; }
#line 49 "<stdout>"
yy4:
	++YYCURSOR;
#line 16 "config4d.f.re"
	{ return NULL; }
#line 54 "<stdout>"
yy6:
	++YYCURSOR;
	YYSETSTATE(1);
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
yyFillLabel1:
	yych = *YYCURSOR;
yy7:
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy6;
	default:	goto yy3;
	}
#line 17 "config4d.f.re"

}
