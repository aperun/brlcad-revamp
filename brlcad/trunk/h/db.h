/*
 *			D B . H
 *
 *		GED Database Format
 *
 *	U. S. Army Ballistic Research Laboratory
 *
 * This header file describes the object file structure.
 */

#define NAMESIZE		16
#define NAMEMOVE(from,to)	(void)strncpy(to, from, NAMESIZE)
extern char *strncpy();

/*
 *		FILE FORMAT
 *
 * All records are of fixed length, and are composed of one of several formats:
 *	An ID record
 *	A SOLID description
 *	A COMBINATION description (1 header, 1 member)
 *	A COMBINATION extension (2 members)
 *	An ARS `A' (header) record
 *	An ARS `B' (data) record
 *	A free record
 *	A Polygon header record
 *	A Polygon data record
 *
 * The records are stored as binary records corresponding to PDP-11 and
 * VAX C structs, so padding must be supplied explicitly for alignment.
 */
union record  {
	char	u_id;		/* To differentiate record types */
#define ID_IDENT	'I'
#define ID_SOLID	'S'
#define ID_COMB	'C'
#define ID_MEMB	'M'
#define ID_ARS_A	'A'
#define ID_ARS_B	'B'
#define ID_FREE		'F'	/* Free record -- ignore */
#define ID_P_HEAD	'P'	/* Polygon header */
#define ID_P_DATA	'Q'	/* Polygon data record */
	char	u_size[128];	/* Total record size */
	struct polyhead  {
		char	p_id;		/* = POLY_HEAD */
		char	p_pad1;
		char	p_name[NAMESIZE];
	} p;
	struct polydata  {
		char	q_id;		/* = POLY_DATA */
		char	q_count;	/* # of vertices <= 5 */
		float	q_verts[5][3];	/* Actual vertices for this polygon */
		float	q_norms[5][3];	/* Normals at each vertex */
	} q;
	struct ident  {
		char	i_id;		/* I */
		char	i_units;	/* units */
#define ID_NO_UNIT	0		/* unspecified */
#define ID_MM_UNIT	1		/* milimeters (preferred) */
#define ID_CM_UNIT	2		/* centimeters */
#define ID_M_UNIT	3		/* meters */
#define ID_IN_UNIT	4		/* inches (deprecated) */
#define ID_FT_UNIT	5		/* feet (deprecated) */
		char	i_version[6];	/* Version code of Database format */
#define ID_VERSION	"v4"		/* Current Version */
		char	i_title[72];	/* Title or description */
	} i;
	struct solidrec  {
		char	s_id;		/* = SOLID */
		char	s_type;		/* GED primitive type */
/* also TOR 	16	/* toroid */
#define GENTGC	18	/* supergeneralized TGC; internal form */
#define GENELL	19	/* ready for drawing ELL:  V,A,B,C */
#define GENARB8	20	/* generalized ARB8:  V, and 7 other vectors */
#define HALFSPACE 21	/* half-space */
		char	s_name[NAMESIZE];	/* unique name */
		short	s_cgtype;		/* COMGEOM solid type */
#define RPP	1	/* axis-aligned rectangular parallelopiped */
#define BOX	2	/* arbitrary rectangular parallelopiped */
#define RAW	3	/* right-angle wedge */
#define ARB4	4	/* tetrahedron */
#define ARB5	5	/* pyramid */
#define ARB6	6	/* extruded triangle */
#define ARB7	7	/* weird 7-vertex shape */
#define ARB8	8	/* hexahedron */
#define ELL	9	/* ellipsoid */
#define ELL1	10	/* ? another ellipsoid ? */
#define SPH	11	/* sphere */
#define RCC	12	/* right circular cylinder */
#define REC	13	/* right elliptic sylinder */
#define TRC	14	/* truncated regular cone */
#define TEC	15	/* truncated elliptic cone */
#define TOR	16	/* toroid */
#define TGC	17	/* truncated general cone */
		float	s_values[24];		/* parameters */
#define s_tgc_V	s_values[0]
#define s_tgc_H	s_values[3]
#define s_tgc_A s_values[6]
#define s_tgc_B s_values[9]
#define s_tgc_C s_values[12]
#define s_tgc_D s_values[15]

#define s_tor_V	s_values[0]
#define s_tor_H	s_values[3]
#define s_tor_A	s_values[6]
#define s_tor_B	s_values[9]
#define s_tor_C	s_values[12]
#define s_tor_D	s_values[15]
#define s_tor_E	s_values[18]
#define s_tor_F	s_values[21]

#define s_ell_V s_values[0]
#define s_ell_A s_values[3]
#define s_ell_B s_values[6]
#define s_ell_C s_values[9]
	}  s;
	struct combination  {
		char	c_id;		/* C */
		char	c_flags;		/* `R' if region, else ` ' */
		char	c_name[NAMESIZE];	/* unique name */
		short	c_regionid;		/* region ID code */
		short	c_aircode;		/* air space code */
		short	c_length;		/* # of members */
		short	c_num;			/* COMGEOM region # */
		short	c_material;		/* material code */
		short	c_los;			/* equivalent LOS estimate */
	}  c;
	struct member  {
		char	m_id;		/* M */
		char	m_relation;		/* boolean operation */
#define INTERSECT	'+'
#define SUBTRACT	'-'
#define UNION		'u'
		char	m_brname[NAMESIZE];	/* name of this branch */
		char	m_instname[NAMESIZE];	/* name of referred-to obj. */
		short	m_pad1;
		float	m_mat[16];		/* homogeneous trans matrix */
		short	m_num;			/* COMGEOM solid # ref */
		short	m_pad2;
	}  M;
	struct ars_rec  {
		char	a_id;		/* A */
		char	a_type;			/* primitive type */
#define	ARS	21	/* arbitrary triangular-surfaced polyhedron */
		char	a_name[NAMESIZE];	/* unique name */
		short	a_m;			/* # of curves */
		short	a_n;			/* # of points per curve */
		short	a_curlen;		/* # of granules per curve */
		short	a_totlen;		/* # of granules for ARS */
		short	a_pad;
		float	a_xmax;			/* max x coordinate */
		float	a_xmin;			/* min x coordinate */
		float	a_ymax;			/* max y coordinate */
		float	a_ymin;			/* min y coordinate */
		float	a_zmax;			/* max z coordinate */
		float	a_zmin;			/* min z coordinate */
	}  a;
	struct ars_ext  {			/* one "granule" */
		char	b_id;		/* B */
		char	b_type;			/* primitive type */
#define ARSCONT 22	/* extension record type for ARS solid */
		short	b_n;			/* current curve # */
		short	b_ngranule;		/* curr. granule for curve */
		short	b_pad;
		float	b_values[8*3];		/* vectors */
	}  b;
};
