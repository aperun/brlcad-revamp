/*
 *			T E R M C A P . H 
 *
 * $Revision$
 *
 * $Log$
 * Revision 1.2  83/12/16  00:10:36  dpk
 * Added distinctive RCS header
 * 
 */

/* Termcap definitions */

extern char	*UP,	/* Scroll reverse, or up */
		*CS,	/* If on vt100 */
		*SO,	/* Start standout */
		*SE,	/* End standout */
		*CM,	/* The cursor motion string */
		*CL,	/* Clear screen */
		*CE,	/* Clear to end of line */
		*HO,	/* Home cursor */
		*AL,	/* Addline (insert line) */
		*DL,	/* Delete line */
		*VS,	/* Visual start */
		*VE,	/* Visual end */
		*KS,	/* Keypad mode start */
		*KE,	/* Keypad mode end */
		*TI,	/* Cursor addressing start */
		*TE,	/* Cursor addressing end */
		*IC,	/* Insert char */
		*DC,	/* Delete char */
		*IM,	/* Insert mode */
		*EI,	/* End insert mode */
		*LL,	/* Last line, first column */
		*M_IC,	/* Insert char with arg */
		*M_DC,	/* Delete char with arg */
		*M_AL,	/* Insert line with arg */
		*M_DL,	/* Delete line with arg */
		*SF,	/* Scroll forward */
		*SR,	/* Scroll reverse */
		*VB;	/* Visible bell */

extern int LI;		/* Number of lines */
extern int CO;		/* Number of columns */
extern int SG;		/* Number of magic cookies left by SO and SG */
extern int XS;		/* Wether standout is braindamaged */

extern int TABS;	/* Whether we are in tabs mode */
extern int UpLen;	/* Length of the UP string */
extern int HomeLen;	/* Length of Home string */
extern int LowerLen;	/* Length of lower string */

extern int BG;		/* Are we on a BITGRAPH */

extern char *BC;	/* Back space */
extern int ospeed;
