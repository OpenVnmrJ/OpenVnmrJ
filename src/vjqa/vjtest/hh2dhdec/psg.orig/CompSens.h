/* CompSens.h - Compressive sensing psg module. Sets up non-uniform sampling. 
   Agilent Technologies, Eriks Kupce, Oxford, Jan. 2011 

   SPARSE - y/n - flag to activate random sampling.
   ni, ni2, ni3 - actually sampled number of points
   nimax, ni2max, ni3max - total number of points in the data matrix
                           that become ni, ni2 and ni3 after inflating
                           the data matrix with inflateND
   skey - integer number - optional seed for the sampling schedule
                           if skey < 0 automatic schedule making is suppressed.
*/

#include "vfilesys.h"

extern char  curexp[];    /* sampling schedule to be saved in curexp */
static char  SPARSE[MAXSTR]; 
static short xdim;
static double **CSsch;
static int   kk, nimax, ni2max, ni3max;

int  get_RS(fname, xni, xni2, xni3) // return 1 if integers and -1 if double
char  *fname;
int    xni, xni2, xni3;
{
  FILE  *src;
  char   cmd[MAXSTR];
  double d[4];
  int    s[4], i, j, k, m;

  k = 1; j = 0; m = -1; 
  if(xni > 1) k *= xni;      
  if(xni2 > 1) k *= xni2;   
  if(xni3 > 1) k *= xni3;   

  if((src = fopen(fname, "r")) == NULL)
  {
    printf("get_RS : problem opening %s \n", fname);
    psg_abort(1);
  }

  fgets(cmd, 512, src);
  xdim = sscanf(cmd, "%lf %lf %lf", &d[0], &d[1], &d[2]);  
  if(xdim < 1) 
  {
    printf("get_RS : failed to read the sampling schedule\n %s \n", fname);
    return 0;
  }

  CSsch = (double **) calloc(xdim, sizeof(double *));
  for(i=0; i<xdim; i++)
    CSsch[i] = (double *) calloc(k, sizeof(double ));
  
  for(j=0; j<xdim; j++)  CSsch[j][0] =  d[j]; 

  i=1;  
  while((fgets(cmd, 512, src)) && (i<k))
  { 
    j = sscanf(cmd, "%lf %lf %lf", &d[0], &d[1], &d[2]);
    if(j<xdim) break;  
    for(j=0; j<xdim; j++) 
    {
      CSsch[j][i] = d[j];  
      if(d[j] > 3.0) m=1;
    }
    i++; 
  }
  fclose(src);
  
  if(i<k)
  {
    printf("Number of scheduled SPARSE increments %d is less than expected (%d)\n", i, k);
    return 0;
  }

  if(xdim < 1) return 0;

  return m*i;
}


void set_RS(opt)  /* set CS schedule */
char *opt;        // reserved for weighted sampling modes
{
  FILE     *src;
  char      stype[MAXSTR], cmd[2*MAXSTR], fname[MAXSTR], path[MAXSTR];
  int       i, j, i1, i2, i3, jx, ni, ni2, ni3, skey;
  double    dkey;

  if (find("SPARSE") > 0) 
  {
    getstr("SPARSE", SPARSE);
    if(SPARSE[0] != 'y') 
      return; 
  }
  else
    return;

  ni=0; ni2=0; ni3=0; i1=0; i2=0; i3=0; sw1=1.0; sw2=1.0; sw3=1.0; 
  nimax=0; ni2max=0; ni3max=0; 
  jx = (int) (0.5 + getval("arrayelemts"));
  ni = (int) (0.5 + getval("ni"));        /* assuming ni is always present */
  if(find("ni2") > 0) ni2 = (int) (0.5 + getval("ni2"));
  if(find("ni3") > 0) ni3 = (int) (0.5 + getval("ni3"));
  if(ni<1) ni=1; if(ni2<1) ni2=1; if(ni3<1) ni3=1;

  if(ni>1)
  {
    if(find("nimax") > 0) nimax = (int) (0.5 + getval("nimax"));
    if(nimax > ni) i1=ni;
  }
  if(ni2>1) 
  {
    if(find("ni2max") > 0) ni2max = (int) (0.5 + getval("ni2max"));
    if(ni2max > ni2) i2=ni2;
  }
  if(ni3>1) 
  {
    if(find("ni3max") > 0) ni3max = (int) (0.5 + getval("ni3max"));
    if(ni3max > ni3) i3=ni3;
  }

  if(FIRST_FID)    /* make the schedule */
  {
    if((!i1) && (!i2) && (!i3))  
    {  
      printf("none of nimax, ni2max or ni3max is set, SPARSE flag ignored.");
      return;
    }
    getstr("stype",stype);
    sprintf(fname,"%s/sampling.sch", curexp);
    if((stype[0] =='a') || (stype[0] == 'y'))
    {
      skey=169; dkey=0.0;
      if(find("skey") > 0)
      {
        dkey = getval("skey");   /* seed for sampling schedule */
        if(dkey < 0.0)
          skey = (int) (dkey - 0.5);   /* seed for sampling schedule */
        else
          skey = (int) (0.5 + dkey);   /* seed for sampling schedule */
      } 
      if (appdirFind("mkCSsch","bin",path,"",R_OK))
        sprintf(cmd,"%s %s %d %d %d %d %d %d %d\n", 
              path, fname, ni, nimax, ni2, ni2max, ni3, ni3max, skey);
      else
        abort_message("Failed to find mkCSsc sampling schedule program");
      system(cmd);
    }
    if(!(kk=get_RS(fname, i1, i2, i3))) 
    {
      printf("getRS(): failed to read the sampling schedule\n");
      psg_abort(1);
    }
  }

  if(i1) 
  { 
    sw1 = getval("sw1");  
    if(sw1<0.1) 
    {
      printf("sw1 not found\n"); 
      psg_abort(0); 
    }
  }

  if(i2) 
  { 
    sw2 = getval("sw2");
    if(sw2<0.1) 
    {
      printf("sw2 not found\n"); 
      psg_abort(0); 
    }
  }
 
  if(i3) 
  { 
    sw3 = getval("sw3"); 
    if(sw3<0.1) 
    {
      printf("sw3 not found\n"); 
      psg_abort(0); 
    }
  }  

  i = (ix-1)/jx; 
  if((!i1) && (ni > 1))     // ni reg
  {
    i /= ni;   
    if((!i2) && (ni2 > 1))  // & ni2 reg
      i /= ni2;   
  }
  if((i1) && (!i2) && (i3)) // only ni2 reg  
  {
    j = i/(ni*ni2);
    i = i%ni + j*ni;
  }

  i = i%abs(kk); j=0; 
  if((i1) && (xdim > j)) { d2 = CSsch[j][i]; if(kk>0) d2/=sw1; j++; }
  if((i2) && (xdim > j)) { d3 = CSsch[j][i]; if(kk>0) d3/=sw2; j++; }
  if((i3) && (xdim > j)) { d4 = CSsch[j][i]; if(kk>0) d4/=sw3; j++; }
  
  return;
}

