/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------
| asm.h 
| this file contains the structure for the sample information
| as created by the enter program. It is use by the ASM software.
+----------------------------------------------------------------*/
#define MAX_ENTRIES 24
#define MAX_PROMPT_LEN 11
#define MAX_TEXT_LEN 128

typedef struct _entry_line_ {
			char eprompt[MAX_PROMPT_LEN], etext[MAX_TEXT_LEN];
		    } ENTRY_LINE;



typedef struct sample_info
	{ 
	  ENTRY_LINE 	prompt_entry[MAX_ENTRIES]; /* Sample#, Macro ,Solvent,text,etc. */
	} SAMPLE_INFO ;

/* ASM defines used by multiple programs */
#define AUTOMODE_BIT 0x8000/* bit set in ExpFlags in psg to indicate automode*/
/* bit set in ExpFlags in Acqproc to indicate resume was send to Autoproc*/
#define RESUME_SENT_BIT 0x4000

/* bit set in ExpFlags in Acqproc to indicate a Resume Acquisition (RA) */
#define RESUME_ACQ_BIT 0x1000
#define VPMODE_BIT 0x400/* bit set in ExpFlags in psg to indicate vpmode (view port mode) */
