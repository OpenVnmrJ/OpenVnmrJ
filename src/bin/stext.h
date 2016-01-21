/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
/* Definition of the data required in the top of the stext file as a
   structure for convenient handling.  The structure has been ordered for
   efficient use of storage, not the order found in the file.
*/

struct stext_essparms {
	short datatype;
	short dim1;
	short dim2;
	short dim3;
	short bytes;
	short order;
	short acqtyp;
	short abstyp;
	char * system;
	long np;
	double sw;
	double sw1;
	double sfrq;
	double dfrq;
};

#define STEXT_datatype_c1fid	1	/* complex 1-d fid */
#define STEXT_datatype_p1spc	2	/* phased 1-d spectrum */
#define STEXT_datatype_c2fid	3	/* complex 2-d fid */
#define STEXT_datatype_p2spc	4	/* phased 2-d spectrum */
#define STEXT_order_mcsparc	1	/* big-endian byte order:  680x0, SPARC */
#define STEXT_order_iapxvax	0	/* little-endian:  80x86, Vax */
#define STEXT_acqtyp_simul	0	/* simultaneous sampling */
#define STEXT_acqtyp_seqen	1	/* seqential sampling */
#define STEXT_abstyp_absval	0	/* absolute value 2-d data */
#define STEXT_abstyp_hyper	1	/* hypercomplex 2-d data */
#define STEXT_abstyp_tppi	2	/* tppi 2-d data */

/* The next two were "18y" and "18n" in previous versions, the 18 being a
   VXR-4000-style string parameter qualifier.  Currently VNMR swrite() does
   not produce the 18, so we drop it here.
 */
#define STEXT_parqual_on	"\"y\""	/* string? enabled */
#define STEXT_parqual_off	"\"n\""	/* string? disabled */

/* prototypes for functions used by modules dealing with stext files */
#ifdef __STDC__

void stext_essential(struct stext_essparms * esp);
void stext_text(char * string);
void stext_text_continued(char * string);
void stext_params_begin(void);
void stext_params_long_long(char * name, void * value);
void stext_params_long_float(char * name, void * value);
void stext_params_double_float(char * name, void * value);
void stext_params_double_double(char * name, void * value);
void stext_params_chars(char * name, void * value, int length);
void stext_params_end(void);

#else /* __STDC__ */

void stext_essential();
void stext_text();
void stext_text_continued();
void stext_params_begin();
void stext_params_long_long();
void stext_params_long_float();
void stext_params_double_float();
void stext_params_double_double();
void stext_params_chars();
void stext_params_end();

#endif /* __STDC__ */
