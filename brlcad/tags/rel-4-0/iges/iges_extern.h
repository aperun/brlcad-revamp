
#define		CARDLEN		71 /* length of data portion in Global records */
#define		PARAMLEN	63 /* length of data portion in Parameter records */

extern char eor; /* IGES end of record delimeter */
extern char eof; /* IGES end of field delimeter */
extern char card[256]; /* input buffer, filled by readrec */
extern char crdate[9],crtime[9]; /* IGES file creation date and time */
extern fastf_t scale,inv_scale; /* IGES file scale factor and inverse */
extern fastf_t conv_factor; /* Conversion factor from IGES file units to mm */
extern mat_t *identity; /* identity matrix */
extern int units; /* IGES file units code */
extern int counter; /* keep track of where we are in the "card" buffer */
extern int pstart; /* record number where parameter section starts */
extern int dstart; /* record number where directory section starts */
extern int totentities; /* total number of entities in the IGES file */
extern int dirarraylen;	/* number of elements in the "dir" array */
extern int reclen; /* IGES file record length (in bytes) */
extern int currec; /* current record number in the "card" buffer */
extern int ntypes; /* Number of different types of IGES entities recognized by
			this code */
extern FILE *fd; /* file pointer for IGES file */
extern FILE *fdout; /*file pointer for BRLCAD output file */
extern struct iges_directory **dir; /* Directory array */
extern struct reglist *regroot; /* list of regions created from solids of revolution */
extern char *types[]; /* character strings of entity type names */
extern int typecount[][2]; /* Count of how many entities of each type actually
				appear in the IGES file */
extern char operator[]; /* characters representing operators: 'u', '+', and '-' */
