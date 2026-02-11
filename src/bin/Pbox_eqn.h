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
/* Pbox_eqn.h - Pbox equation evaluator */


void parse_eqn(char *expr)
{
  int i=0, _ipr=0, ipr_=0;            

  while(expr[i]!='\0')
  {
    switch (expr[i])
    {
      case '{': expr[i] = '('; break; 
      case '}': expr[i] = ')'; break;
      case '[': expr[i] = '('; break;
      case ']': expr[i] = ')'; break;
    }
    if (strchr("!@#$&_~:',<>?\"\\=|`", expr[i]))
    {
      printf("Pbox wave expr : character %c is not allowed \n", expr[i]);
      exit(0);
    }
    if(expr[i]=='(') _ipr++;
    else if (expr[i]==')') ipr_++;
    i++;
  }
  if (_ipr != ipr_) 
  {
    printf("Pbox wave expr : unbalanced parentheses.\n");
    exit(0);
  }
  return;
}


int findeqn(fnm, str, val)    /* find equation */
FILE *fnm;
char *str, *val;
{
  char chr[MAXSTR], *c;
          
  fseek(fnm, 0, 0);
  while (fscanf(fnm, "%s", chr) != EOF)
  {
    while ((chr[0] == '#') || (chr[0] == '('))
    {
      fgets(chr, MAXSTR, fnm); fscanf(fnm, "%s", chr);
    }
    if (chr[0] == str[0])
    {
      if (strcmp(chr,str) == 0)
      {
        fscanf(fnm, "%s", chr);
        if (chr[0] == '=')
        {
          fgets(val, MAXSTR, fnm); 
          if( (c = strchr(val, ';')) )
          {
            *c = '\0';
            (void) parse_eqn(val);           
          }  
          else
            sscanf(val, "%s", val);
          return 1;
        }
      }
      else if ((cutstr2(val, chr, '=')) && (strcmp(val, str) == 0))
      {
        strcpy(val, chr);
        fgets(chr, MAXSTR, fnm); 
        if ( (c = strchr(chr, ';')) )
        {
          *c = '\0';
          strcat(val,chr);
          (void) parse_eqn(val);
        }
        return 1;
      }
    }
  }
  return (0);
}


int findeqnm(fnm, str, val, ip)   /* find modified eqn */
FILE *fnm;
char *val, *str;
int  ip;
{
  char chr[MAXSTR], *c;
  
  chr[0] = '\0';
  fseek(fnm, ip, 0); 
  while ((fscanf(fnm, "%s", chr) != EOF) && (chr[0] != '$'))
  {
    if (str[0] == chr[0]) 
    {
      if(strcmp(chr,str) == 0)
      {
        fscanf(fnm, "%s", chr);
        if (chr[0] == '=') 
        {
          fgets(val, MAXSTR, fnm); 
          if ( (c = strchr(val, ';')) )
          {
            *c = '\0';
            (void) parse_eqn(val);           
          }  
          else
            sscanf(val, "%s", val);
          return 1;
        }
      }
      else if ((cutstr2(val, chr, '=')) && (strcmp(val, str) == 0))
      {
        strcpy(val, chr);
        fgets(chr, MAXSTR, fnm); 
        if ( (c = strchr(chr, ';')) )
        {
          *c = '\0';
          strcat(val,chr);
          (void) parse_eqn(val);
        }
        return 1;
      }
    }
  }
  return (0);
}


/* Available functions : acos, asin, cos, atan, exp, floor, log,  
   sech, ceil, ln, sin, sinh, cosh, sqrt, tan, tanh, fabs  */

char set_func(sfun)
char *sfun;
{
  switch(sfun[0])
  {
    case 'a': 
      if (strcmp(sfun,"acos") == 0) return 'a';
      else if (strcmp(sfun,"asin") == 0) return 'b';
      else if (strcmp(sfun,"atan") == 0) return 'd';
    case 'c': 
      if (strcmp(sfun,"cos") == 0) return 'c';
      else if (strcmp(sfun,"cosh") == 0) return 'm';
      else if (strcmp(sfun,"ceil") == 0) return 'i';
    case 'e': 
      if (strcmp(sfun,"exp") == 0) return 'e';
    case 'f': 
      if (strcmp(sfun,"floor") == 0) return 'f';
      else if (strcmp(sfun,"fabs") == 0) return '|';
    case 'l': 
      if (strcmp(sfun,"ln") == 0) return 'l';
      else if (strcmp(sfun,"log") == 0) return 'g';
    case 's': 
      if (strcmp(sfun,"sin") == 0) return 's';
      else if (strcmp(sfun,"sinh") == 0) return 'n';
      else if (strcmp(sfun,"sech") == 0) return 'h';
      else if (strcmp(sfun,"sqrt") == 0) return 's';
    case 't': 
      if (strcmp(sfun,"tan") == 0) return 't';
      else if (strcmp(sfun,"tanh") == 0) return 'z';
    default:  return 0;
  }
  return 0;
}


int isdelim(delim)
char delim;
{
  switch(delim)
  {
    case '+': return ADD;
    case '-': return ADD;
    case '*': return MULT;
    case '/': return MULT;
    case '%': return MULT;
    case '^': return POW;
    case '(': return PRN;
    case ')': return PRN;
    case '=': return EQ;
    default:  return 0;
  }
}


int get_tokens(expr, tkn, sfnm, tc)
char *expr, *sfnm;
Var  *tkn;
int  *tc;
{
  int i=0, j=0, prn=0, imax=0;          
  char str[MAXSTR], val[MAXSTR];
  FILE *fil;

  if ((fil = fopen(sfnm, "r")) == NULL)
  {
    printf("Pbox_eqn : problems opening file %s...", sfnm);
    exit(0);
  }

  (*tc) = 0;
  tkn->flg = DBLE, tkn->mgn=0.0; tkn++;    
  while(expr[i]!='\0')
  {
    while(isspace(expr[i])) i++; 
    if ( (tkn->flg = isdelim(expr[i])) )
    {
      tkn->u = expr[i++];
      if(tkn->u == '(')
        tkn->flg += prn++;
      else if (tkn->u == ')')
        tkn->flg += --prn;
      if(tkn->flg > imax) 
        imax = tkn->flg;
    }
    else if(isalpha(expr[i]))
    {
      tkn->flg = DBLE;  
      str[j++]=expr[i++]; 
      while((isdelim(expr[i])==0) && (isspace(expr[i])==0))
        str[j++]=expr[i++];
      str[j] = '\0'; 
      while(isspace(expr[i])) i++;
      if (expr[i] == '(')
      {         
        if ( (tkn->u = set_func(str)) )
          tkn->flg = FUNC;
        else
        {
          printf("Pbox wave expr : %s() - unrecognized function. \n", val);
          exit(0);
        } 
      }
      else if (((str[0]=='t') || (str[0]=='T')) && (str[1]=='\0'))
      {
        tkn->flg = VAR;  /* time constant */
        (*tc)++;
      }
      else if (findpar(fil, str, val))
        tkn->mgn = stod(val); 
      else
      {
        printf("Pbox wave expr : parameter %s not found.\n", str);
        exit(0);
      }
    }
    else if ((isdigit(expr[i])) || (expr[i]=='.'))
    {
      tkn->flg = DBLE;  
      val[j++]=expr[i++]; 
      while((isdigit(expr[i])) || (expr[i]=='.'))
        val[j++]=expr[i++];
      val[j] = '\0';
      tkn->mgn = stod(val);
    }
    tkn++; j=0;
  }
  fclose(fil);  
  return imax;
}


int all_tokens(expr)		/* count all tokens */
char *expr;
{
  int i=0, toks=0;          
  
  while(expr[i]!='\0')
  {
    while(isspace(expr[i])) i++;
    if(isdelim(expr[i])) toks++, i++;
    else if(isalpha(expr[i]))
    {
      toks++, i++; 
      while((isdelim(expr[i])==0) && (isspace(expr[i])==0)) i++;
    }
    else if ((isdigit(expr[i])) || (expr[i]=='.'))
    {
      toks++; i++; 
      while((isdigit(expr[i])) || (expr[i]=='.')) i++;
    }
  }
  return toks;
}


double eval_exp(lval, rval, delim)
double  lval, rval;
char    delim;
{
  double in;
  
  switch(delim)
  {
    case '+': 
      return (lval+rval);
    case '-': 
      return (lval-rval);
    case '*': 
      return (lval*rval);
    case '/': 
      if(rval==0.0)
        pxerr("Pbox wave expr : division by zero !\n");
      else
        return (lval/rval);
    case '%': 
      if(rval==0.0)
        pxerr("Pbox wave expr : division by zero !\n");
      else 
        return fmod(lval,rval);
    case '^': 
      if (lval == 0.0) return 0.0;
      else if ((lval < 0.0) && (modf(rval, &in) != 0.0))
        pxerr("Pbox wave expr : math exception in pow()\n");
      else
        return pow(lval,rval);
    case 'a': 
    case 'b': 
      if ((rval < -1.0) || (rval > 1.0))
        pxerr("Pbox wave expr : math exception in acos() or asin()\n");
      else if (delim == 'a')
        return acos(rval);
      else
        return asin(rval);
    case 'c': 
      return cos(rval);
    case 'd': 
      return atan(rval);
    case 'e': 
      return exp(rval);
    case 'f': 
      return floor(rval);
    case 'g': 
    case 'l': 
      if (rval <= 0.0)
        pxerr("Pbox wave expr : math exception in ln() or log()\n");
      else if (delim == 'g')
        return log10(rval);
      else
        return log(rval);
    case 'h': 
      return (1.0/cosh(rval));
    case 'i': 
      return ceil(rval);
    case 'm': 
      return cosh(rval);
    case 'n': 
      return sinh(rval);
    case 'q': 
      if (rval < 0.0)
        pxerr("Pbox wave expr : math exception in sqrt() \n");
      else
        return sqrt(rval);
    case 's': 
      return sin(rval);
    case 't': 
      return tan(rval);
    case 'z': 
      return tanh(rval);
    case '|': 
      return fabs(rval);
    default:  
      printf("Pbox wave expr : unrecognized operator %c \n", delim);
      exit(0);
  }
  return 0.0;
}


void eval_exp1(lval, rval, delim, np)
double  lval, *rval[];
char    delim;
int     np;
{
  int    i;
  double in;
  
  switch(delim)
  {
    case '+': 
      for(i=0; i<np; i++)
        (*rval)[i] += lval;
      break;
    case '-': 
      for(i=0; i<np; i++)
        (*rval)[i] = lval - (*rval)[i];
      break;
    case '*': 
      for(i=0; i<np; i++)
        (*rval)[i] *= lval;
      break;
    case '/': 
      for(i=0; i<np; i++)
      {
        if((*rval)[i]==0.0)
          pxerr("Pbox wave expr : division by zero !\n");
        else
          (*rval)[i] = lval/(*rval)[i];
      }
      break;
    case '%': 
      for(i=0; i<np; i++)
      {
        if((*rval)[i]==0.0)
          pxerr("Pbox wave expr : division by zero !\n");
        else
          (*rval)[i] = fmod(lval,(*rval)[i]);
      }
      break;
    case '^': 
      if (lval == 0.0) 
      {
        for(i=0; i<np; i++)
          (*rval)[i] = 0.0;
      }
      else if (lval < 0.0)
      {
        for(i=0; i<np; i++)
        {
          if (modf((*rval)[i], &in) != 0.0)
            pxerr("Pbox wave expr : math exception in pow()\n");
          else
            (*rval)[i] = pow(lval,(*rval)[i]);
        }
      }
      else
      { 
        for(i=0; i<np; i++)
          (*rval)[i] = pow(lval,(*rval)[i]);
      }
      break;   
    case 'a': 
      for(i=0; i<np; i++)
      {
        if (((*rval)[i] < -1.0) || ((*rval)[i] > 1.0))
          pxerr("Pbox wave expr : math exception in acos()\n");
        else
          (*rval)[i] = acos((*rval)[i]);
      }
      break;
    case 'b': 
      for(i=0; i<np; i++)
      {
        if (((*rval)[i] < -1.0) || ((*rval)[i] > 1.0))
          pxerr("Pbox wave expr : math exception in asin()\n");
        else
          (*rval)[i] = asin((*rval)[i]);
      }
      break;
    case 'c': 
      for(i=0; i<np; i++)
        (*rval)[i] = cos((*rval)[i]);
      break;
    case 'd': 
      for(i=0; i<np; i++)
        (*rval)[i] = atan((*rval)[i]);
      break;
    case 'e': 
      for(i=0; i<np; i++)
        (*rval)[i] = exp((*rval)[i]);
      break;
    case 'f': 
      for(i=0; i<np; i++)
        (*rval)[i] = floor((*rval)[i]);
      break;
    case 'g': 
      for(i=0; i<np; i++)
      {
        if ((*rval)[i] <= 0.0)
          pxerr("Pbox wave expr : math exception in log()\n");
        else
          (*rval)[i] = log10((*rval)[i]);
      }
      break;
    case 'l': 
      for(i=0; i<np; i++)
      {
        if ((*rval)[i] <= 0.0)
          pxerr("Pbox wave expr : math exception in log()\n");
        else
          (*rval)[i] = log((*rval)[i]);
      }
      break;
    case 'h': 
      for(i=0; i<np; i++)
        (*rval)[i] = 1.0/cosh((*rval)[i]);
      break;
    case 'i': 
      for(i=0; i<np; i++)
        (*rval)[i] = ceil((*rval)[i]);
      break;
    case 'm': 
      for(i=0; i<np; i++)
        (*rval)[i] = cosh((*rval)[i]);
      break;
    case 'n': 
      for(i=0; i<np; i++)
        (*rval)[i] = sinh((*rval)[i]);
      break;
    case 'q': 
      for(i=0; i<np; i++)
      {
        if ((*rval)[i] < 0.0)
          pxerr("Pbox wave expr : math exception in sqrt()\n");
        else
          (*rval)[i] = sqrt((*rval)[i]);
      }
      break;
    case 's': 
      for(i=0; i<np; i++)
        (*rval)[i] = sin((*rval)[i]);
      break;
    case 't': 
      for(i=0; i<np; i++)
        (*rval)[i] = tan((*rval)[i]);
      break;
    case 'z': 
      for(i=0; i<np; i++)
        (*rval)[i] = tanh((*rval)[i]);
      break;
    case '|': 
      for(i=0; i<np; i++)
        (*rval)[i] = fabs((*rval)[i]);
      break;
    default:  
      printf("Pbox wave expr : unrecognized operator %c \n", delim);
      exit(0);
  }
}


double eval_exp2(lval, rval, delim, np)
double  *lval[], rval;
char    delim;
int     np;
{
  int    i;
  double in;

  switch(delim)
  {
    case '+': 
      for(i=0; i<np; i++)
        (*lval)[i] += rval;
      break;
    case '-': 
      for(i=0; i<np; i++)
        (*lval)[i] -= rval;
      break;
    case '*': 
      for(i=0; i<np; i++)
        (*lval)[i] *= rval;
      break;  
    case '/': 
      if(rval==0.0)
        pxerr("Pbox wave expr : division by zero !\n");
      for(i=0; i<np; i++)
        (*lval)[i] /= rval;
      break;
    case '%': 
      if(rval==0.0)
        pxerr("Pbox wave expr : division by zero !\n");
      for(i=0; i<np; i++)
        (*lval)[i] = fmod((*lval)[i],rval);
      break;
    case '^': 
      for(i=0; i<np; i++)
      {
        if ((*lval)[i] != 0.0)
        { 
          if (((*lval)[i] < 0.0) && (modf(rval, &in) != 0.0))
            pxerr("Pbox wave expr : math exception in pow()\n");
          else
            (*lval)[i] = pow((*lval)[i],rval);
        }
      }
      break;
    default:  
      printf("Pbox wave expr : unrecognized operator %c \n", delim);
      exit(0);
  }
  return 0.0;
}


void eval_exp3(lval, rval, delim, np)
double  *lval[], *rval[];
char    delim;
int     np;
{
  int    i;
  double in;
  
  switch(delim)
  {
    case '+': 
      for(i=0; i<np; i++)
        (*rval)[i] += (*lval)[i];
      break;
    case '-': 
      for(i=0; i<np; i++)
        (*rval)[i] = (*lval)[i] - (*rval)[i];
      break;
    case '*': 
      for(i=0; i<np; i++)
        (*rval)[i] *= (*lval)[i];
      break;
    case '/': 
      for(i=0; i<np; i++)
      {
        if((*rval)[i]==0.0)
          pxerr("Pbox wave expr : division by zero !\n");
        else
          (*rval)[i] = (*lval)[i]/(*rval)[i];
      }
      break;
    case '%': 
      for(i=0; i<np; i++)
      {
        if((*rval)[i]==0.0)
          pxerr("Pbox wave expr : division by zero !\n");
        else
          (*rval)[i] = fmod((*lval)[i],(*rval)[i]);
      }
      break;
    case '^': 
      for(i=0; i<np; i++)
      {
        if ((*lval)[i] != 0.0)
        {
          if (((*lval)[i] < 0.0) && (modf((*rval)[i], &in) != 0.0))
            pxerr("Pbox wave expr : math exception in pow()\n");
          else
            (*rval)[i] = pow((*lval)[i],(*rval)[i]);
        }
      }
      break;   
    default:  
      printf("Pbox wave expr : unrecognized operator %c \n", delim);
      exit(0);
  }
}


int ee(eqn, fnm, np, ar)
char    *eqn, *fnm;
int      np;
double  *ar[];
{
  int    i, j, k, ip, jp, toks, tkmax, ii=0, ic[2], tc, ti, tj;
  char   expr[MAXSTR];
  double **a;
  Var    *tok;
  
  strcpy(expr, eqn);
  toks = all_tokens(expr);          
  if (toks)
    tok = (Var *) calloc(++toks, sizeof(Var));
  else
  {
    printf("Pbox wave expr : missing expression. \n");
    exit(0);
  }
  tkmax = get_tokens(expr, tok, fnm, &tc); 

  if (tc>0) 
  {
    a = (double **) calloc(tc, sizeof(double *));
    for (i = 0; i < tc; i++)
      a[i] = arry(np);
    for (i = 0; i < tc; i++)
    {
      for (j = 0; j < np; j++)
        a[i][j] = (double) j/(np - 1);
    }

  }
  else
  {
    for (i = 0; i < tc; i++)
      Amp[i] = 1.0;
    return(1);
  }
  
  for(i=0, j=0; i<toks; i++)
  {
    if(tok[i].flg==VAR)
      tok[i].mgn = (double) j++;
  }
  
  for(j=tkmax; j>0; j--)
  { 
    if (j>FUNC)
    {
      for(i=0; i<toks; i++)
      {
        if(tok[i].flg==j) 
          ic[ii++]=i;
        if(ii==2)
        {
          tok[ic[0]].flg=0, tok[ic[1]].flg=0;
          ii=0; ic[0]++;
          for(jp=FUNC; jp>0; jp--)
          {
            for(ip=ic[0]; ip<ic[1]; ip++)
            {
              if(tok[ip].flg==jp)
              {
                k=ip+1;
                while(tok[k].flg == 0) k++;
                if(tok[ip-1].flg<VAR)
                { 
                  if(tok[k].flg==DBLE)
                    tok[k].mgn = eval_exp(tok[ip-1].mgn, tok[k].mgn, tok[ip].u);
                  else if (tok[k].flg==VAR)
                  {
                    tj = (int) (tok[k].mgn+0.1); 
                    (void) eval_exp1(tok[ip-1].mgn, &a[tj], tok[ip].u, np);
                  }
                }
                else if ((tok[ip-1].flg==VAR) && (tok[k].flg==DBLE))
                {
                  tj = (int) (tok[ip-1].mgn+0.1); 
                  tok[k].mgn =  eval_exp2(&a[tj], tok[k].mgn, tok[ip].u, np);
                  tok[k].flg = VAR, tok[k].mgn = (double) tj;
                }
                else if ((tok[ip-1].flg==VAR) && (tok[k].flg==VAR))
                {
                  tj = (int) (tok[ip-1].mgn+0.1); 
                  ti = (int) (tok[k].mgn+0.1);
                  (void) eval_exp3(&a[tj], &a[ti], tok[ip].u, np);
                }                
                if(tok[ip].flg != FUNC) tok[ip-1].flg=0;
                tok[ip].flg=0;
              }
            }
          }
          tok[ic[1]]=tok[ic[1]-1];
          tok[ic[1]-1].flg=0;
        }
      }
    }
    else
    {  
      for(i=0; i<toks; i++)
      {
        if(tok[i].flg==j)
        {
          k=i+1;
          while(tok[k].flg == 0) k++;
          if(tok[i-1].flg<VAR) 
          {
            if (tok[k].flg==DBLE)
              tok[k].mgn = eval_exp(tok[i-1].mgn, tok[k].mgn, tok[i].u);
            else if (tok[k].flg==VAR)
            {
              tj = (int) (tok[k].mgn+0.1); 
              (void) eval_exp1(tok[i-1].mgn, &a[tj], tok[i].u, np);
            }
          }
          else if ((tok[i-1].flg==VAR) && (tok[k].flg==DBLE))
          {
            tj = (int) (tok[i-1].mgn+0.1); 
            (void) eval_exp2(&a[tj], tok[k].mgn, tok[i].u, np);
            tok[k].flg = VAR, tok[k].mgn = (double) tj;
          }
          else if ((tok[i-1].flg==VAR) && (tok[k].flg==VAR))
          {
            tj = (int) (tok[i-1].mgn+0.1);
            ti = (int) (tok[k].mgn+0.1); 
            (void) eval_exp3(&a[tj], &a[ti], tok[i].u, np);
          }
          if(tok[i].flg != FUNC) tok[i-1].flg=0;
          tok[i].flg=0;
        }
      }
    }
  }

  k = (int) (tok[toks-1].mgn+0.1); 
  if(k>tc) pxerr("Pbox wave expr : count error.\n");
  for(i=0; i<np; i++)
    (*ar)[i] = a[k][i];

  free(tok);  
  return (1);
}


