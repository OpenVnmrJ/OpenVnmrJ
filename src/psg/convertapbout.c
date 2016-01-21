/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  These define's are shared with convert.c  */
#include <stdio.h>

#define STD_APBUS_DELAY  32			/* 400 nanosecs */
#define PFG_APBUS_DELAY  72			/* 900 nanosecs */


#define APCCNTMSK	0x0fff
#define APCONTNUE	0x1000

#define APADDRMSK	0x0fff 
#define APBYTF		0x1000

#define APINCFLG	0x2000


/*  These defines are unique to this file.  */

#define APSELECT 0xA000         /* select apchip register */
#define APWRITE  0xB000         /* write to apchip reister */
#define APPREINCWR 0x9000       /* preincrement register and write */
#define APCTLMASK 0xf000
#define APDATAMASK 0x00ff

typedef short  codeint;

int gen_apbcout(codeint *codeptr, codeint *addr, codeint len)
{
	int	 dlindex, index, aindex;
	codeint  datalen,curaddr=0;
	codeint  *tmpcodeptr, *tmpdelayptr;

	tmpcodeptr = codeptr;
	tmpdelayptr = codeptr;		/* save delay addr */
	*codeptr++ = STD_APBUS_DELAY;  /* delay */


	index = 0;
	while (len > 0)
	{
	    if (((*addr & APCTLMASK) == APSELECT) && ((*(addr+1) & APCTLMASK)
			== APWRITE))
	    {
		*codeptr++ = 0;		/* save space for length */
		index++;
		dlindex = index;
		curaddr = (*addr++ & APADDRMSK);
		len--;
		/* delay time for pfg is 1 usec */
		if (curaddr >= 0x0c50 && curaddr <= 0x0c5f)
		    *tmpdelayptr=PFG_APBUS_DELAY;
		if (curaddr >= 0x0940 && curaddr <= 0x095f)
		    *tmpdelayptr=PFG_APBUS_DELAY;
		*codeptr++ = curaddr | APBYTF;		/* save addr */
		index++;
		aindex = index;
		datalen = 0;
		if ((*(addr+1) & APCTLMASK) == APWRITE)
		{
		   while ((*addr & APCTLMASK) == APWRITE)
		   {
			if (datalen%2 == 0) {
		   	   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   	   index++;
			}
			else {
		   	   *(codeptr-1) = *(codeptr-1) | 
					(*addr++ & APDATAMASK);
			}
		   	len--;
			datalen++;
		   }
		}
		else if ((*(addr+1) & APCTLMASK) == APPREINCWR)
		{
		   tmpcodeptr[aindex] = tmpcodeptr[aindex] | APINCFLG;
		   *codeptr++ = *addr++ << 8;
		   len--;
		   index++;
		   datalen++;
		   while ((*addr & APCTLMASK) == APPREINCWR)
		   {
			if (datalen%2 == 0) {
		   	   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   	   index++;
			}
			else {
		   	   *(codeptr-1) = *(codeptr-1) | 
						(*addr++ & APDATAMASK);
			}
		   	len--;
			datalen++;
			curaddr++;
		   }
		}
		else
		{
		   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   len--;
		   index++;
		   datalen++;
		}
		
		tmpcodeptr[dlindex] =  datalen;
	    }
	    else if (((*addr & APCTLMASK) == APSELECT) && ((*(addr+1) & 
			APCTLMASK) == APPREINCWR))
	    {
		*codeptr++ = 0;		/* save space for length */
		index++;
		dlindex = index;
		curaddr = (*addr++ & APADDRMSK);
		len--;
		/* delay time for pfg is 1 usec */
		if (curaddr >= 0x0c50 && curaddr <= 0x0c5f)
		    *tmpdelayptr=PFG_APBUS_DELAY;
		if (curaddr >= 0x0940 && curaddr <= 0x095f)
		    *tmpdelayptr=PFG_APBUS_DELAY;
		*codeptr++ = (curaddr+1) | APBYTF;	/* save addr */
		index++;
		aindex = index;
		datalen = 0;
		if ((*(addr+1) & APCTLMASK) == APWRITE)
		{
		   curaddr++;
		   while ((*addr & APCTLMASK) == APWRITE)
		   {
			if (datalen%2 == 0) {
		   	   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   	   index++;
			}
			else {
		   	   *(codeptr-1) = *(codeptr-1) | 
						(*addr++ & APDATAMASK);
			}
		   	len--;
			datalen++;
		   }
		}
		else if ((*(addr+1) & APCTLMASK) == APPREINCWR)
		{
		   curaddr++;
		   tmpcodeptr[aindex] = tmpcodeptr[aindex] | APINCFLG;
		   *codeptr++ = *addr++ << 8;
		   len--;
		   index++;
		   datalen++;
		   while ((*addr & APCTLMASK) == APPREINCWR)
		   {
			if (datalen%2 == 0) {
		   	   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   	   index++;
			}
			else {
		   	   *(codeptr-1) = *(codeptr-1) | 
						(*addr++ & APDATAMASK);
			}
		   	len--;
			datalen++;
			curaddr++;
		   }
		}
		else
		{
		   curaddr++;
		   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   len--;
		   index++;
		   datalen++;
		}
		
		tmpcodeptr[dlindex] =  datalen;
	    }
	    else if ((*addr & APCTLMASK) == APPREINCWR)
	    {
		*codeptr++ = 0;		/* save space for length */
		index++;
		dlindex = index;
		/* delay time for pfg is 1 usec */
		if (curaddr >= 0x0c50 && curaddr <= 0x0c5f)
		    *tmpdelayptr=PFG_APBUS_DELAY;
		if (curaddr >= 0x0940 && curaddr <= 0x095f)
		    *tmpdelayptr=PFG_APBUS_DELAY;
		*codeptr++ = (curaddr+1) | APBYTF;	/* save addr */
		index++;
		aindex = index;
		datalen = 0;
		if ((*(addr+1) & APCTLMASK) == APWRITE)
		{
		   curaddr++;
		   *codeptr++ = *addr++ << 8;
		   len--;
		   index++;
		   datalen++;
		   while ((*addr & APCTLMASK) == APWRITE)
		   {
			if (datalen%2 == 0) {
		   	   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   	   index++;
			}
			else {
		   	   *(codeptr-1) = *(codeptr-1) | 
					(*addr++ & APDATAMASK);
			}
		   	len--;
			datalen++;
		   }
		}
		else if ((*(addr+1) & APCTLMASK) == APPREINCWR)
		{
		   curaddr++;
		   tmpcodeptr[aindex] = tmpcodeptr[aindex] | APINCFLG;
		   *codeptr++ = *addr++ << 8;
		   len--;
		   index++;
		   datalen++;
		   while ((*addr & APCTLMASK) == APPREINCWR)
		   {
			if (datalen%2 == 0) {
		   	   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   	   index++;
			}
			else {
		   	   *(codeptr-1) = *(codeptr-1) | 
						(*addr++ & APDATAMASK);
			}
		   	len--;
			datalen++;
			curaddr++;
		   }
		}
		else
		{
		   curaddr++;
		   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   len--;
		   index++;
		   datalen++;
		}
		
		tmpcodeptr[dlindex] =  datalen;
	    }
	    else if ((*addr & APCTLMASK) == APWRITE)
	    {
		*codeptr++ = 0;		/* save space for length */
		index++;
		dlindex = index;
		/* delay time for pfg is 1 usec */
		if (curaddr >= 0x0c50 && curaddr <= 0x0c5f)
		    *tmpdelayptr=PFG_APBUS_DELAY;
		if (curaddr >= 0x0940 && curaddr <= 0x095f)
		    *tmpdelayptr=PFG_APBUS_DELAY;
		*codeptr++ = (curaddr) | APBYTF;	/* save addr */
		index++;
		aindex = index;
		datalen = 0;
		if ((*(addr+1) & APCTLMASK) == APWRITE)
		{
		   *codeptr++ = *addr++ << 8;
		   len--;
		   index++;
		   datalen++;
		   while ((*addr & APCTLMASK) == APWRITE)
		   {
			if (datalen%2 == 0) {
		   	   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   	   index++;
			}
			else {
		   	   *(codeptr-1) = *(codeptr-1) | 
						(*addr++ & APDATAMASK);
			}
		   	len--;
			datalen++;
		   }
		}
		else if ((*(addr+1) & APCTLMASK) == APPREINCWR)
		{
		   tmpcodeptr[aindex] = tmpcodeptr[aindex] | APINCFLG;
		   *codeptr++ = *addr++ << 8;
		   len--;
		   index++;
		   datalen++;
		   while ((*addr & APCTLMASK) == APPREINCWR)
		   {
			if (datalen%2 == 0) {
		   	   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   	   index++;
			}
			else {
		   	   *(codeptr-1) = *(codeptr-1) | 
						(*addr++ & APDATAMASK);
			}
		   	len--;
			datalen++;
			curaddr++;
		   }
		}
		else
		{
		   *codeptr++ = (*addr++ & APDATAMASK) << 8;
		   len--;
		   index++;
		   datalen++;
		}
		
		tmpcodeptr[dlindex] =  datalen;
	    }
	    else
	    {
	        printf("convert: apbout - unidentified code: 0x%x\n",*addr);
		return( -1 );
	    }
	    if (len > 0)
		tmpcodeptr[dlindex] =  tmpcodeptr[dlindex] | APCONTNUE;
	}
	/* tmpcodeptr[lindex] =  index; */

/*	for (j=0; j<index; j++)
 *	{
 *	    IXPRINT2("actual(gen_apbcout) %d = 0x%x\n",j,tmpcodeptr[lindex+j]);
 *	}
 *	IXPRINT1("gen_apbcout codeptr = 0x%x\n",codeptr);
 */

	return(index+1);
}


