/* Generated by re2c */
#line 1 "bug1682718.re"
char *scan(char *p)
{

#line 7 "<stdout>"
	{
		unsigned char yych;

		yych = (unsigned char)*p;
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
		++p;
		yych = (unsigned char)*p;
		goto yy7;
yy3:
#line 9 "bug1682718.re"
		{return p;}
#line 32 "<stdout>"
yy4:
		++p;
#line 10 "bug1682718.re"
		{return (char*)0;}
#line 37 "<stdout>"
yy6:
		++p;
		yych = (unsigned char)*p;
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
	}
#line 11 "bug1682718.re"

}

