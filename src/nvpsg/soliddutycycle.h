/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

extern int option_check(const char *option);

// =============================
// Structure for DUTY
// =============================

typedef struct {
   double duty;         // percentage duty cycle (%)
   double dutyon;       // total possible time on
   double dutyoff;      // total timeoff
   int    c1;           // conditional for time off (1) 
   double t1;           // time off for conditional (1)
   int    c2;           // conditional for time off (2)
   double t2;           // time off for conditional (2)
   int    c3;           // conditional for time off (3)
   double t3;           // time off for conditional (3)
   int    c4;           // conditional for time off (4)
   double t4;           // time off for conditional (4)
   int    c5;           // conditional for time off (5)
   double t5;           // time off for conditional (5)
   int    c6;           // conditional for time off (6)
   double t6;           // time off for conditional (6)
   int    c7;           // conditional for time off (7)
   double t7;           // time off for conditional (7)
   int    c8;           // conditional for time off (8)
   double t8;           // time off for conditional (8)
} DUTY;

DUTY init_dutycycle()
{

   DUTY d; 
     
   d.c1 = -1;
   d.c2 = -1;
   d.c3 = -1;
   d.c4 = -1;
   d.c5 = -1;
   d.c6 = -1;
   d.c7 = -1;
   d.c8 = -1;
     
   d.t1 = 0.0;
   d.t2 = 0.0;
   d.t3 = 0.0;
   d.t4 = 0.0;
   d.t5 = 0.0;
   d.t6 = 0.0;
   d.t7 = 0.0;
   d.t8 = 0.0;

   d.duty = 105.0;
   d.dutyon = 0.0;
   d.dutyoff = 0.0;
   return d;
}

DUTY update_dutycycle(DUTY d)
{

   d.duty = d.dutyoff + d.dutyon; 
   if (d.c1 > -1) d.duty = d.duty + d.t1;
   if (d.c2 > -1) d.duty = d.duty + d.t2;
   if (d.c3 > -1) d.duty = d.duty + d.t3;
   if (d.c4 > -1) d.duty = d.duty + d.t4;
   if (d.c5 > -1) d.duty = d.duty + d.t5;
   if (d.c6 > -1) d.duty = d.duty + d.t6;
   if (d.c7 > -1) d.duty = d.duty + d.t7;
   if (d.c8 > -1) d.duty = d.duty + d.t8;

   if (d.c1 > 0) d.dutyon = d.dutyon + d.t1;
   if (d.c2 > 0) d.dutyon = d.dutyon + d.t2;
   if (d.c3 > 0) d.dutyon = d.dutyon + d.t3;
   if (d.c4 > 0) d.dutyon = d.dutyon + d.t4;
   if (d.c5 > 0) d.dutyon = d.dutyon + d.t5;
   if (d.c6 > 0) d.dutyon = d.dutyon + d.t6;
   if (d.c7 > 0) d.dutyon = d.dutyon + d.t7;
   if (d.c8 > 0) d.dutyon = d.dutyon + d.t8;

   if (d.duty == 0.0) {
       printf("Warning: Total time Is 0.0, Exiting update_dutycycle()\n");
       return d;
   }
   d.duty = 100.0*d.dutyon/d.duty;
   return d; 
}

double abort_dutycycle(DUTY d, double percent)
{
   printf("Duty Cycle is %.1f%%\n", d.duty); 
   if (d.duty > 100.0) {
      abort_message("Abort: The duty cycle has not been determined\n");
   }
   if (d.duty > percent) {
     if (option_check("nosafe"))
      warn_message("Warning: The duty cycle of %.1f%% is greater than %.1f%%.\n", d.duty, percent); 
     else  
      abort_message("Abort: The duty cycle of %.1f%% is greater than %.1f%%.\n", d.duty, percent);
   }
   return d.duty; 
}


