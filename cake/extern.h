/*
**	External definitions for Cake.
**
**	$Header$
*/

/* library functions */
#if !defined(__bsdi__)
extern	char	*malloc();
#endif
#ifdef	ATT
extern	char	*strchr();
extern	char	*strrchr();
#else
extern	char	*index();
extern	char	*rindex();
#endif

/* memory management */
extern	Cast	newmem();
extern		oldmem();

/* make routines */
extern	Node	*make_node();
extern	Entry	*make_dep();
extern	Test	*make_test_mm();
extern	Test	*make_test_m();
extern	Test	*make_test_c();
extern	Test	*make_test_s();
extern	Test	*make_test_l();
extern	Test	*make_test_b();
extern	Test	*make_test_u();
extern	Pat	*make_pat();
extern	Act	*make_act();

extern	Act	*prep_act();
extern	Act	*prep_script();

/* table handling routines */
extern		init_sym();
extern	char	*new_name();

/* printout routines */
extern	char	scratchbuf[];
extern		print_pat();
extern		print_test();
extern		print_entry();
extern		print_node();

/* flags */
extern	int	Gflag;
extern	int	Lflag;
extern	int	Rflag;
extern	int	Xflag;
extern	int	bflag;
extern	int	cflag;
extern	int	dflag;
extern	int	gflag;
extern	int	iflag;
extern	int	kflag;
extern	int	nflag;
extern	int	qflag;
extern	int	rflag;
extern	int	sflag;
extern	int	tflag;
extern	int	vflag;
extern	int	wflag;
extern	int	xflag;
extern	int	zflag;

extern	int	cakedebug;
extern	int	entrydebug;
extern	int	patdebug;
extern	int	lexdebug;

/* misc global vars */
extern	char	*cakefile;
extern	int	maxprocs;

/* misc functions */
extern	char	*get_newname();
extern		put_trail();
extern		get_trail();

extern	int	carry_out();
extern	int	action();
extern	int	cake_wait();
extern	char	*expand_cmds();

/* Moved out of individual modules */
extern		get_utime();
extern	time_t	cake_gettime();
extern	Node	*chase();
extern	Node	*chase_node();
extern	bool	match();
extern	bool	eval();
extern	Entry	*ground_entry();
extern	Test	*deref_test();
extern		deref_entry();
extern	List	*entries;
extern	char	*list_names();
extern	List	*break_pat();
extern	bool	hasvars();
extern		do_when();
extern	char	*ground();
extern	bool	has_meta();
extern	char	*strip_backslash();
extern	char	*shell_path[2];
extern	char	*shell_cmd[2];
extern	char	*shell_opt[2];
extern	List	*find_process();
extern	int	parse_args();
extern	Node	*chase();
extern	char	*getenv();
extern	char	*dir_setup();
extern	FILE	*cake_popen();
extern	FILE	*yyin;
