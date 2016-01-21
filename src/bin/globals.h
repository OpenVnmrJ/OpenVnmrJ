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
extern FILE * fid_in, * params_in, * text_in;
extern FILE * fid_out, * params_out, * text_out;
extern FILE * temp_out;
extern unsigned char * in_header;
extern unsigned char * in_data;
extern struct datafilehead in_fhead, out_fhead;
extern struct datablockhead in_bhead, out_bhead;
extern double realfactor, imagfactor;
