/* Generated by re2c */
// multiple scanners


enum YYCONDTYPE {
	yycr1,
	yycr2,
};


void scan(unsigned char* in)
{

{
	unsigned char yych;
	static void *yyctable[2] = {
		&&yyc_r1,
		&&yyc_r2,
	};
	goto *yyctable[YYGETCONDITION()];
/* *********************************** */
yyc_r1:

	if (limit1 <= cursor1) fill1(1);
	yych = *cursor1;
	if (yych <= '2') {
		if (yych <= '0') goto yy2;
		if (yych <= '1') goto yy5;
		goto yy3;
	} else {
		if (yych <= '`') goto yy2;
		if (yych <= 'a') goto yy7;
		if (yych <= 'b') goto yy9;
	}
yy2:
yy3:
	++cursor1;
	{ return "2"; }
yy5:
	++cursor1;
	{ return "1"; }
yy7:
	++cursor1;
	{ return "a"; }
yy9:
	++cursor1;
	{ return "b"; }
/* *********************************** */
yyc_r2:
	if (limit1 <= cursor1) fill1(1);
	yych = *cursor1;
	if (yych <= '2') {
		if (yych <= '0') goto yy13;
		if (yych <= '1') goto yy16;
		goto yy14;
	} else {
		if (yych == 'b') goto yy18;
	}
yy13:
yy14:
	++cursor1;
	{ return "2"; }
yy16:
	++cursor1;
	{ return "1"; }
yy18:
	++cursor1;
	{ return "b"; }
}

}

void scan(unsigned short* in)
{

{
	unsigned short yych;
	static void *yyctable[2] = {
		&&yyc_r1,
		&&yyc_r2,
	};
	goto *yyctable[YYGETCONDITION()];
/* *********************************** */
yyc_r1:

	if (limit2 <= cursor2) fill2(1);
	yych = *cursor2;
	if (yych <= '2') {
		if (yych <= '0') goto yy2;
		if (yych <= '1') goto yy5;
		goto yy3;
	} else {
		if (yych <= '`') goto yy2;
		if (yych <= 'a') goto yy7;
		if (yych <= 'b') goto yy9;
	}
yy2:
yy3:
	++cursor2;
	{ return "2"; }
yy5:
	++cursor2;
	{ return "1"; }
yy7:
	++cursor2;
	{ return "a"; }
yy9:
	++cursor2;
	{ return "b"; }
/* *********************************** */
yyc_r2:
	if (limit2 <= cursor2) fill2(1);
	yych = *cursor2;
	if (yych <= '2') {
		if (yych <= '0') goto yy13;
		if (yych <= '1') goto yy16;
		goto yy14;
	} else {
		if (yych == 'b') goto yy18;
	}
yy13:
yy14:
	++cursor2;
	{ return "2"; }
yy16:
	++cursor2;
	{ return "1"; }
yy18:
	++cursor2;
	{ return "b"; }
}

}

void scan(unsigned int* in)
{

{
	unsigned int yych;
	static void *yyctable[2] = {
		&&yyc_r1,
		&&yyc_r2,
	};
	goto *yyctable[YYGETCONDITION()];
/* *********************************** */
yyc_r1:

	if (limit3 <= cursor3) fill3(1);
	yych = *cursor3;
	if (yych <= '2') {
		if (yych <= '0') goto yy2;
		if (yych <= '1') goto yy5;
		goto yy3;
	} else {
		if (yych <= '`') goto yy2;
		if (yych <= 'a') goto yy7;
		if (yych <= 'b') goto yy9;
	}
yy2:
yy3:
	++cursor3;
	{ return "2"; }
yy5:
	++cursor3;
	{ return "1"; }
yy7:
	++cursor3;
	{ return "a"; }
yy9:
	++cursor3;
	{ return "b"; }
/* *********************************** */
yyc_r2:
	if (limit3 <= cursor3) fill3(1);
	yych = *cursor3;
	if (yych <= '2') {
		if (yych <= '0') goto yy13;
		if (yych <= '1') goto yy16;
		goto yy14;
	} else {
		if (yych == 'b') goto yy18;
	}
yy13:
yy14:
	++cursor3;
	{ return "2"; }
yy16:
	++cursor3;
	{ return "1"; }
yy18:
	++cursor3;
	{ return "b"; }
}

}

