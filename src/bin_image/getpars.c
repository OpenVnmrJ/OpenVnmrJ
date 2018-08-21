/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*---------------------------------------------------------------------------*/
/* This is free software: you can redistribute it and/or modify              */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This is distributed in the hope that it will be useful,                   */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* If not, see <http://www.gnu.org/licenses/>.                               */
/*---------------------------------------------------------------------------*/
/**/

extern char procpar[MAXSTR];

double getval(char *param) {
  FILE  *fp;
  char   name[MAXSTR];
  int    type,i,active,itmp,n;
  float  ftmp;

  if ((fp = fopen(procpar,"r")) == NULL) {
    fprintf(stdout,"Unable to open procpar file %s\n",procpar);
    exit(1);
  }

  /* Look for parameter name */
  do {
    /* MUST check for an error here (an infinite loop can otherwise result) */
    if (fscanf(fp, "%s", name) != 1) {
      fprintf(stdout,"Can't find parameter %s\n",param);
      fclose(fp);
      return(0);
    }
  } while (strcmp(name, param) != 0);


  /* Read all the flag variables, looking for type and value of 'active' */
  if (fscanf(fp, "%*f %d %*f %*f %*f %*f %*f %*f %d %*f",&type,&active) != 2) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",name);
    fclose(fp);
    exit(0);
  }
      
  if (active == 0) {
    fclose(fp);
    return(0);
  }

  /* The next line has the number of values */
  if (fscanf(fp, "%d", &n) != 1) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",param);
    fclose(fp);
    exit(0);
  }


  switch(type) {
    case 0:
      for (i = 0;i < n;i++) {
        if (fscanf(fp, "%d", &itmp) != 1) {
	  fprintf(stdout,"Problem getting values for for parameter %s\n",param);
          fclose(fp);
	  exit(0);
	}
        fclose(fp);
        return((double)itmp);
      }
      break;
    case 1:
      for (i = 0;i < n;i++)  {
        if (fscanf(fp, "%f", &ftmp) != 1) {
	  fprintf(stdout,"Problem getting values for for parameter %s\n",param);
          fclose(fp);
	  exit(0);
	}
        fclose(fp);
        return(ftmp);
      }
      break;
    case 2:
      fprintf(stdout,"Parameter type is a string, use getstr\n");
      fclose(fp);
      exit(0);
      break;
    default:
      break;
  }

  fclose(fp);
  return(0.0);  // Should never get to here
}


int getarray(char *param, double *vals) {
  FILE *fp;
  char  name[MAXSTR];
  int   type,i,active,itmp;
  float ftmp;
  int   nvals;

  if ((fp = fopen(procpar,"r")) == NULL) {
    fprintf(stdout,"Unable to open procpar file %s\n",procpar);
    exit(1);
  }

  /* Look for parameter name */
  do {
    /* MUST check for an error here (an infinite loop can otherwise result) */
    if (fscanf(fp, "%s", name) != 1) {
      fprintf(stdout,"Can't find parameter %s\n",param);
      fclose(fp);
      return(0);
    }
  } while (strcmp(name, param) != 0);


  /* Read all the flag variables, looking for type and value of 'active' */
  if (fscanf(fp, "%*f %d %*f %*f %*f %*f %*f %*f %d %*f",&type,&active) != 2) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",name);
    fclose(fp);
    exit(0);
  }
      
  if (active == 0) {
    fclose(fp);
    return(0);
  }

  /* The next line has the number of values */
  if (fscanf(fp, "%d", &nvals) != 1) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",param);
    fclose(fp);
    exit(0);
  }

  switch(type) {
    case 0:
      for (i = 0;i < nvals;i++) {
        if (fscanf(fp, "%d", &itmp) != 1) {
	  fprintf(stdout,"Problem getting values for for parameter %s\n",param);
          fclose(fp);
	  exit(0);
	}
        vals[i] = (double) itmp;
      }
      break;
    case 1:
      for (i = 0;i < nvals;i++)  {
        if (fscanf(fp, "%f", &ftmp) != 1) {
	  fprintf(stdout,"Problem getting values for for parameter %s\n",param);
          fclose(fp);
	  exit(0);
	}
        vals[i] = (double)ftmp;
      }
      break;
    case 2:
      fprintf(stdout,"Parameter type is a string, use getstr\n");
      fclose(fp);
      exit(0);
      break;
    default:
      break;
  }

  fclose(fp);
  return(nvals);

}





void getstr(char *param, char *str) {
  FILE *fp;
  char  name[MAXSTR], str1[MAXSTR];
  int   type,i,active,n;

  if ((fp = fopen(procpar,"r")) == NULL) {
    fprintf(stdout,"Unable to open procpar file %s\n",procpar);
    exit(1);
  }

  /* Look for parameter name */
  do {
    /* MUST check for an error here (an infinite loop can otherwise result) */
    if (fscanf(fp, "%s", name) != 1) {
      fprintf(stdout,"Can't find parameter %s\n",param);
      fclose(fp);
      exit(0);
    }
  } while (strcmp(name, param) != 0);


  /* Read all the flag variables, looking for type and value of 'active' */
  if (fscanf(fp, "%*f %d %*f %*f %*f %*f %*f %*f %d %*f",&type,&active) != 2) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",name);
    fclose(fp);
    exit(0);
  }
      

  /* The next line has the number of values */
  if (fscanf(fp, "%d", &n) != 1) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",param);
    fclose(fp);
    exit(0);
  }

  if (n > 1) {
    printf("Can not read arrayed strings, sorry\n");
    fclose(fp);
    exit(0);
  }
  
  
  switch(type) {
    case 0:
      fprintf(stdout,"Parameter type is a number, use getval\n");
      fclose(fp);
      exit(0);
      break;
    case 1:
      fprintf(stdout,"Parameter type is a number, use getval\n");
      fclose(fp);
      exit(0);
      break;
    case 2:
        if (fscanf(fp, "%s", str1) != 1) {
          fprintf(stdout,"Problem getting parameter %s\n",param);
          fclose(fp);
          exit(0);
	}
	for (i=1; i<strlen(str1)-1; i++)
	  str[i-1] = str1[i];
	str[i] = '\0';
      break;
    default:
      break;
  }


  fclose(fp);
}


int getstrarray(char *param, char **strarray) {
  FILE *fp;
  char  name[MAXSTR], str1[MAXSTR];
  int   type,i,active,n;
  int   nvals;

  if ((fp = fopen(procpar,"r")) == NULL) {
    fprintf(stdout,"Unable to open procpar file %s\n",procpar);
    exit(1);
  }

  /* Look for parameter name */
  do {
    /* MUST check for an error here (an infinite loop can otherwise result) */
    if (fscanf(fp, "%s", name) != 1) {
      fprintf(stdout,"Can't find parameter %s\n",param);
      fclose(fp);
      return(0);
    }
  } while (strcmp(name, param) != 0);


  /* Read all the flag variables, looking for type and value of 'active' */
  if (fscanf(fp, "%*f %d %*f %*f %*f %*f %*f %*f %d %*f",&type,&active) != 2) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",name);
    fclose(fp);
    exit(0);
  }
      
  if (active == 0) {
    fclose(fp);
    return(0);
  }

  /* The next line has the number of values */
  if (fscanf(fp, "%d", &nvals) != 1) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",param);
    fclose(fp);
    exit(0);
  }

/*   if (sizeof(strarray) < nvals) {
    printf("getarray: size of array (%d) is too small, %d elements needed\n", (int)sizeof(strarray),nvals);
    fclose(fp);
    exit(0);
  }
 */

  switch(type) {
    case 0:
      fprintf(stdout,"Parameter type is a number, use getval\n");
      fclose(fp);
      exit(0);
      break;
    case 1:
      fprintf(stdout,"Parameter type is a number, use getval\n");
      fclose(fp);
      exit(0);
      break;
    case 2:
      for (n = 0;n < nvals;n++) {
        if (fscanf(fp, "%s", str1) != 1) {
          fprintf(stdout,"Problem getting parameter %s\n",param);
          fclose(fp);
          exit(0);
	}
	for (i=1; i<strlen(str1)-1; i++)
	  strarray[n][i-1] = str1[i];
	strarray[n][i] = '\0';
      }
      break;
    default:
      break;
  }



  fclose(fp);
  return(nvals);

}




int getstatus(char *param) {
  FILE  *fp;
  char   name[MAXSTR];
  int    type,active,n;

  if ((fp = fopen(procpar,"r")) == NULL) {
    fprintf(stdout,"Unable to open procpar file %s\n",procpar);
    exit(1);
  }

  /* Look for parameter name */
  do {
    if (fscanf(fp, "%s", name) != 1) {
      fprintf(stdout,"Can't find parameter %s\n",param);
      fclose(fp);
      return(0);
    }
  } while (strcmp(name, param) != 0);

  /* Read all the flag variables, looking for type and value of 'active' */
  if (fscanf(fp, "%*f %d %*f %*f %*f %*f %*f %*f %d %*f",&type,&active) != 2) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",name);
    fclose(fp);
    exit(0);
  }
      
  if (active == 0) {
    fclose(fp);
    return(0);
  }


  /* The next line has the number of values */
  if (fscanf(fp, "%d", &n) != 1) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",param);
    fclose(fp);
    exit(0);
  }

  fclose(fp);
  return(n);
  
}

int gettype(char *param) {
  /* Get parameter type:
       0 = integer
       1 = real
       2 = string
  */
  FILE  *fp;
  char   name[MAXSTR];
  int    type,active;

  if ((fp = fopen(procpar,"r")) == NULL) {
    fprintf(stdout,"Unable to open procpar file %s\n",procpar);
    exit(1);
  }

  /* Look for parameter name */
  do {
    /* MUST check for an error here (an infinite loop can otherwise result) */
    if (fscanf(fp, "%s", name) != 1) {
      fprintf(stdout,"Can't find parameter %s\n",param);
      fclose(fp);
      return(0);
    }
  } while (strcmp(name, param) != 0);


  /* Read all the flag variables, looking for type and value of 'active' */
  if (fscanf(fp, "%*f %d %*f %*f %*f %*f %*f %*f %d %*f",&type,&active) != 2) {
    fprintf(stdout,"Problem with attributes for parameter %s\n",name);
    fclose(fp);
    exit(0);
  }

  return(type);
  
}
