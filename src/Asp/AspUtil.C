/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <math.h>
#include "AspUtil.h"
#include "graphics.h"

extern "C" {
#include "variables.h"          // For T_REAL, T_STRING
#include "group.h" 
   extern void *allocateWithId(size_t n, const char *id);
   extern void release(void *p);
   extern void scfix1(float *frompntr, int fromincr, float mult, short *topntr, int toincr, int npnts);
   void aip_drawString(const char *str, int x, int y, int clear, int color);
   void aip_drawVString(const char *str, int x, int y, int color);
}

int AspUtil::getParSize(string parName) {
   const char *cname = parName.c_str();
   vInfo  info;
   if (!(P_getVarInfo(CURRENT,cname,&info)) ) return info.size; 
   else if (!(P_getVarInfo(GLOBAL,cname,&info)) ) return info.size; 
   else return 0; 
}

bool AspUtil::setReal(string varname, int index, double value, bool notify)
{
    const char *cname = varname.c_str();
    if (P_setreal(CURRENT, cname, value, index)) { // Try CURRENT
        if (P_setreal(GLOBAL, cname, value, index)) { // Try GLOBAL
            if (P_creatvar(CURRENT, cname, T_REAL)) { // Create in CURRENT
                return false;   // Can this happen?
            }
            P_setprot(CURRENT, cname, 0x8000); // Set no-share attribute
            P_setdgroup(CURRENT, cname, D_PROCESSING); // Processing parm
            if (P_setreal(CURRENT, cname, value, index)) {
                return false;   // Can this happen?
            }
        }
    }
    if (notify) {
	char str[64];
	sprintf(str,"1 %s",cname);
	writelineToVnmrJ("pnew", str);
    }
    return true;
}

bool AspUtil::setReal(string varname, double value, bool notify)
{
    const char *cname = varname.c_str();
    if (P_setreal(CURRENT, cname, value, 1)) { // Try CURRENT
        if (P_setreal(GLOBAL, cname, value, 1)) { // Try GLOBAL
            if (P_creatvar(CURRENT, cname, T_REAL)) { // Create in CURRENT
                return false;   // Can this happen?
            }
            P_setprot(CURRENT, cname, 0x8000); // Set no-share attribute
            P_setdgroup(CURRENT, cname, D_PROCESSING); // Processing parm
            if (P_setreal(CURRENT, cname, value, 1)) {
                return false;   // Can this happen?
            }
        }
    }
    if (notify) {
	char str[64];
	sprintf(str,"1 %s",cname);
	writelineToVnmrJ("pnew", str);
    }
    return true;
}

bool AspUtil::setString(string varname, string value, bool notify)
{
    const char *cname = varname.c_str();
    const char *cval = value.c_str();
    if (P_setstring(CURRENT, cname, cval, 1)) { // Try CURRENT
        if (P_setstring(GLOBAL, cname, cval, 1)) { // Try GLOBAL
            if (P_creatvar(CURRENT, cname, T_STRING)) { // Create in CURRENT
                return false;   // Can this happen?
            }
            P_setprot(CURRENT, cname, 0x8000); // Set no-share attribute
            P_setdgroup(CURRENT, cname, D_PROCESSING); // Processing parm
            if (P_setstring(CURRENT, cname, cval, 1)) {
                return false;   // Can this happen?
            }
        }
    }
    if (notify) {
	char str[64];
	sprintf(str,"1 %s",cname);
	writelineToVnmrJ("pnew", str);
    }
    return true;
}

double AspUtil::getReal(string varname, double defaultVal)
{
    const char *cname = varname.c_str();
    double value;
    if (P_getreal(CURRENT, cname, &value, 1)) { // Try CURRENT
        if (P_getreal(GLOBAL, cname, &value, 1)) { // Try GLOBAL
            // Doesnt exist; create it with default value
            setReal(varname, defaultVal, false);
            return defaultVal;
        }
    }
    return value;
}

double AspUtil::getReal(string varname, int index, double defaultVal)
{
    const char *cname = varname.c_str();
    double value;

    if (index <= 0) {
        index = 1;
    }
    if (P_getreal(CURRENT, cname, &value, index)) { // Try CURRENT
        if (P_getreal(GLOBAL, cname, &value, index)) { // Try GLOBAL
            // Doesnt exist
            return defaultVal;
        }
    }
    return value;
}

string AspUtil::getString(string varname)
{
    const char *cname = varname.c_str();
    char cval[2048];
    if (P_getstring(CURRENT, cname, cval, 1, sizeof(cval))) {
        if (P_getstring(GLOBAL, cname, cval, 1, sizeof(cval))) {
            // Doesn't exist; return empty string
            // (Don't create variable)
            return "";
        }
    }
    string rtn(cval);
    return rtn;
}

string AspUtil::getString(string varname, string defaultVal)
{
    const char *cname = varname.c_str();
    char cval[2048];
    if (P_getstring(CURRENT, cname, cval, 1, sizeof(cval))) {
        if (P_getstring(GLOBAL, cname, cval, 1, sizeof(cval))) {
            // Doesnt exist; create it with default value
            //setString(varname, defaultVal, false);
            return defaultVal;
        }
    }
    string rtn(cval);
    return rtn;
}

bool AspUtil::isActive(string varname)
{
    const char *cname = varname.c_str();
    int stat;
    if ((stat=P_getactive(CURRENT, cname)) == ACT_ON ||
        (stat < 0 && P_getactive(GLOBAL, cname) == ACT_ON))
    {
        return true;
    }
    return false;
}

bool AspUtil::setActive(string varname, bool value)
{
    const char *cname = varname.c_str();
    int ival = value ? ACT_ON : ACT_OFF;
    if (P_setactive(CURRENT, cname, ival)) { // Try CURRENT
        if (P_setactive(GLOBAL, cname, ival)) { // Try GLOBAL
            return false;
        }
    }
    return true;
}

int AspUtil::selectLine(int x, int y, int x1, int y1, int x2, int y2, bool handle) {
   double a2=(x1-x)*(x1-x)+(y1-y)*(y1-y);
   double b2=(x2-x)*(x2-x)+(y2-y)*(y2-y);
   double c2=(x1-x2)*(x1-x2)+(y1-y2)*(y1-y2);
   double c = sqrt(c2);
   double d = (a2-b2+c2)/(2*c);
   double h2 = a2-d*d;

   int select;
   if(d < -2 || d > (c+2)) select=0;
   else if(handle && fabs(a2) < 10) select = HANDLE1;
   else if(handle && fabs(b2) < 10) select = HANDLE2;
   else if(fabs(h2) < 4) select = LINE1;
   else select=0;
//Winfoprintf("h2 %d %d %d %d %d %d %d %f %f %f %f %f",x,y,x1,y1,x2,y2,select,a2,b2,c2,d,h2); 

   return select;
}

int AspUtil::select(int x, int y, int px, int py, int pw, int ph, int rank, bool handle) {
   return select(x, y, (double)px, (double)py, (double)pw, (double)ph, rank, handle);
}

// handles start from upper left corner and clockwise
int AspUtil::select(int x, int y, double px, double py, double pw, double ph, int rank, bool handle) {

	int mouseOver = NOSELECT;
    //    int h=HANDLE_SIZE;
    double h = (pw<ph) ? pw:ph;
    h = (h<HANDLE_SIZE) ? h:HANDLE_SIZE;

      double xmax=px+pw;
      double ymax=py+ph;
      if(rank == 2 && x>=px && x<=xmax && y>=py && y<=ymax) {
          mouseOver=BOXSELECT;
      } else if(rank == 1 && x>=px && x<=xmax) {
          mouseOver=BOXSELECT;
      }
      if(mouseOver != NOSELECT && handle) {

        if(pw==0 && ph==0 && rank==2) mouseOver=HANDLE3;
	else if(pw==0 && ph==0) mouseOver=LINE2;

        else if(x>(px-h) && x<(px+h) && y>(py-h) && y<(py+h)) {
          if(rank == 2) mouseOver=HANDLE1;
          else if(rank == 1) mouseOver=LINE1;
        } else if(x>(xmax-h) && x<(xmax+h) && y>(py-h) && y<(py+h)) {
          if(rank ==2 ) mouseOver=HANDLE2;
          else if(rank == 1) mouseOver=LINE2;
        } else if(x>(xmax-h) && x<(xmax+h) && y>(ymax-h) && y<(ymax+h)) {
          if(rank == 2) mouseOver=HANDLE3;
          else if(rank == 1) mouseOver=LINE2;
        } else if(x>(px-h) && x<(px+h) && y>(ymax-h) && y<(ymax+h)) {
          if(rank == 2) mouseOver=HANDLE4;
          else if(rank == 1) mouseOver=LINE1;
        } else if(x>(px-h) && x<(px+h) && y>=py && y<=ymax) {
          mouseOver=LINE1;
        } else if(x>(xmax-h) && x<(xmax+h) && y>=py && y<=ymax) {
          mouseOver=LINE2;
        } else if(rank == 2 && y>(py-h) && y<(py+h) && x>=px && x<=xmax) {
          mouseOver=LINE3;
        } else if(rank == 2 && y>(ymax-h) && y<(ymax+h) && x>=px && x<=xmax) {
          mouseOver=LINE4;
        } 
      }
      return mouseOver;
}

void AspUtil::drawHandle(int mouseOver, double px, double py, double pw, double ph, int color, int thickness) {

    double h = (pw<ph) ? pw:ph;
    h = (h<HANDLE_SIZE) ? h:HANDLE_SIZE;

    Dpoint_t handle[3];
    if(mouseOver==HANDLE1) {
        handle[0].x=px;
        handle[0].y=py+h;
        handle[1].x=px;
        handle[1].y=py;
        handle[2].x=px+h;
        handle[2].y=py;
        GraphicsWin::drawPolyline(handle,3,color);
    } else if(mouseOver==HANDLE2) {
        handle[0].x=px+pw;
        handle[0].y=py+h;
        handle[1].x=px+pw;
        handle[1].y=py;
        handle[2].x=px+pw-h;
        handle[2].y=py;
        GraphicsWin::drawPolyline(handle,3,color);
    } else if(mouseOver==HANDLE3) {
        handle[0].x=px+pw;
        handle[0].y=py+ph-h;
        handle[1].x=px+pw;
        handle[1].y=py+ph;
        handle[2].x=px+pw-h;
        handle[2].y=py+ph;
        GraphicsWin::drawPolyline(handle,3,color);
    } else if(mouseOver==HANDLE4) {
        handle[0].x=px;
        handle[0].y=py+ph-h;
        handle[1].x=px;
        handle[1].y=py+ph;
        handle[2].x=px+h;
        handle[2].y=py+ph;
        GraphicsWin::drawPolyline(handle,3,color);
    }
}

void AspUtil::drawBorderLine(int mouseOver, double px, double py, double pw, double ph, int color, int thickness) {

    Dpoint_t handle1,handle2;
    if(mouseOver==LINE1) {
        handle1.x=handle2.x=px;
        handle1.y=py;
        handle2.y=py+ph;
        AspUtil::drawLine(handle1,handle2,color);
    } else if(mouseOver==LINE2) {
        handle1.x=handle2.x=px+pw;
        handle1.y=py;
        handle2.y=py+ph;
        AspUtil::drawLine(handle1,handle2,color);
    } else if(mouseOver==LINE3) {
        handle1.x=px;
        handle2.x=px+pw;
        handle1.y=handle2.y=py;
        AspUtil::drawLine(handle1,handle2,color);
    } else if(mouseOver==LINE4) {
        handle1.x=px;
        handle2.x=px+pw;
        handle1.y=handle2.y=py+ph;
        AspUtil::drawLine(handle1,handle2,color);
    }
}

void AspUtil::drawBox(int px, int py, int pw, int ph, int color, int thickness) {
   drawBox((double)px,(double)py,(double)pw,(double)ph,color,thickness);
}

void AspUtil::drawBox(double px, double py, double pw, double ph, int color, int thickness) {
   Dpoint box[5];
   box[0].x=box[3].x=box[4].x=px;
   box[0].y=box[1].y=box[4].y=py;
   box[1].x=box[2].x=px+pw;
   box[2].y=box[3].y=py+ph;

   GraphicsWin::drawPolyline(box, 5, color);
}

void AspUtil::drawLine(Dpoint_t p1, Dpoint_t p2, int color, int thickness) {
/*
   Dpoint line[2];
   line[0].x=p1.x;
   line[0].y=p1.y;
   line[1].x=p2.x;
   line[1].y=p2.y;
*/
   GraphicsWin::drawLine((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, color);
}

void AspUtil::clearFields(int x, int y, int w, int h) {
   GraphicsWin::clearRect(x,y,w,h);
}

// TODO: font and style
void AspUtil::writeFields(char *str, int x, int y, int w, int h) {
    char fontName[64];    
    getOptName(TEXT,fontName);
    set_graphics_font(fontName);
    aip_drawString(str, x, y, 0, PARAM_COLOR);
    set_graphics_font("Default");
}

int AspUtil::drawYbars(float *phasfl, int np, int skip, double vx, double vy, double vw, double vh, int dcolor, double scale, double yoff, int vertical) {

//calc_ybars(float *phasfl, int skip, double scale, int df, int dn, int n, int off) {
//displayspec(int df, int dn, int vertical, int *newspec, int *oldspec, int *erase, int max, int min, int dcolor)

   struct ybar *out;
   short *bufpnt;
   int ybarsize;

   int df,dn,df2,dn2,off;

   if(vertical) {
       df2 = (int)vx;
       dn2 = (int)vw;
       df = mnumypnts - (int)(vy+vh);
       dn = (int)vh;
       off=(int)yoff+df;
       if (df + dn > mnumypnts) return(1);
   } else {
       df = (int)vx;
       dn = (int)vw;
       df2 = mnumypnts - (int)(vy+vh);
       dn2 = (int)vh;
       off=(int)yoff+df2;
       if (df + dn > mnumxpnts) return(1);
   }

  /* this condition typically occurs if the graphics screen is re-sized */
  if (df + dn > mnumxpnts)
    return(1);

  ybarsize = mnumxpnts;
  if (mnumypnts>ybarsize)
    ybarsize = mnumypnts;
  if ((bufpnt = (short *) allocateWithId(sizeof(short)*np,"newProc"))==0)
    { Werrprintf("cannot allocate buffer space");
      return 1;
    }
  if ((out = (struct ybar*)allocateWithId(sizeof(struct ybar)*ybarsize,"newProc"))==0)
    { Werrprintf("cannot allocate ybar buffer space");
          return 1;
    }
  scfix1(phasfl,skip,(float) scale,bufpnt,1,np);
  if (dn > np)
    expand(bufpnt,np,out+df,dn,off);
  else
    compress(bufpnt,np,out+df,dn,off);
  release(bufpnt);

  if(dcolor >=0) color(dcolor);
//Winfoprintf("##%d df,dn,max,min %d %d %d %d",vertical,df,dn,df2+dn2,df2));
  ybars(df, df+dn-1, out, vertical, df2+dn2,df2);

  release(out);
  set_anno_color("");
  return 0;
}

void AspUtil::drawString(const char *str, int x, int y, int color, const char *fontName, int rotate) {
    if(strlen(fontName)>0) set_graphics_font(fontName);
    if(rotate>0) {
	aip_drawVString(str, x, y, color);
    } else aip_drawString(str, x, y, 0, color);
    set_graphics_font("Default");
}

void AspUtil::drawMark(int x, int y, int color, int thickness) {
    Gpoint mpnt[4];
    // Note that g_fill_polygon() works asymetrically, which is why
    //  we need to subtract one less than the "MARK_SIZE", but add the
    //  whole "MAKK_SIZE".
    int size = thickness+MARK_SIZE;
    mpnt[0].x = x - size+ 1;
    mpnt[0].y = y - size+ 1;
    mpnt[1].x = x + size;
    mpnt[1].y = y - size+ 1;
    mpnt[2].x = x + size;
    mpnt[2].y = y + size;
    mpnt[3].x = x - size+ 1;
    mpnt[3].y = y + size;
    GraphicsWin::fillPolygon(mpnt, 4, color);
}

void AspUtil::drawMark(int x, int y, int w, int h, int color, int thickness) {
   int x1 = x - w/2;
   int x2 = x + w/2;  
   int y1 = y - h/2;
   int y2 = y + h/2;  
   GraphicsWin::drawLine(x1,y1,x2,y2, color);
   GraphicsWin::drawLine(x2,y1,x1,y2, color);
}

void AspUtil::drawOval(int px, int py, int pw, int ph, int color, int thickness) {
   GraphicsWin::drawOval(px,py,pw,ph,color);
}

void AspUtil::drawArrow(Dpoint_t p1, Dpoint_t p2, int color, bool tail, bool twoEnds, int xsize, int ysize, int thickness) {

   double dx=p1.x-p2.x;
   double dy=p1.y-p2.y;
   double r = sqrt(dx*dx+dy*dy);
   double xs = xsize+(thickness);
   double ys = ysize+(thickness)/2;
   if(!tail) xs = r; // a full length triangle

   Gpoint poly[3];
   Dpoint_t points[2];
   poly[0].x=(int)p2.x;
   poly[0].y=(int)p2.y;

   if(p2.x==p1.x) {
     poly[1].x=(int)(p2.x+ys);
     poly[2].x=(int)(p2.x-ys);
     if(p2.y>p1.y) {
       poly[1].y=(int)(p2.y-xs);
       poly[2].y=(int)(p2.y-xs);
     } else {
       poly[1].y=(int)(p2.y+xs);
       poly[2].y=(int)(p2.y+xs);
     }
   } else if(p2.y==p1.y) {
     poly[1].y=(int)(p2.y+ys);
     poly[2].y=(int)(p2.y-ys);
     if(p2.x>p1.x) {
       poly[1].x=(int)(p2.x-xs);
       poly[2].x=(int)(p2.x-xs);
     } else {
       poly[1].x=(int)(p2.x+xs);
       poly[2].x=(int)(p2.x+xs);
     }
   } else {
     double sinpsi = -dy/r;
     double cospsi = dx/r;
     poly[1].x=(int)(p2.x + xs*cospsi - ys*sinpsi);
     poly[2].x=(int)(p2.x + xs*cospsi + ys*sinpsi);
     poly[1].y=(int)(p2.y - xs*sinpsi - ys*cospsi);
     poly[2].y=(int)(p2.y - xs*sinpsi + ys*cospsi);
   }

   points[0].x = 0.5*(double)(poly[1].x+poly[2].x);
   points[0].y = 0.5*(double)(poly[1].y+poly[2].y);

   GraphicsWin::fillPolygon(poly,3,color);

   if(!twoEnds) {
     if(tail) drawLine(points[0],p2,color,thickness);
     return;
   }

   poly[0].x=(int)p1.x;
   poly[0].y=(int)p1.y;

   if(p2.x==p1.x) {
     poly[1].x=(int)(p1.x+ys);
     poly[2].x=(int)(p1.x-ys);
     if(p1.y>p2.y) {
       poly[1].y=(int)(p1.y-xs);
       poly[2].y=(int)(p1.y-xs);
     } else {
       poly[1].y=(int)(p1.y+xs);
       poly[2].y=(int)(p1.y+xs);
     }
   } else if(p2.y==p1.y) {
     poly[1].y=(int)(p1.y+ys);
     poly[2].y=(int)(p1.y-ys);
     if(p1.x>p2.x) {
       poly[1].x=(int)(p1.x-xs);
       poly[2].x=(int)(p1.x-xs);
     } else {
       poly[1].x=(int)(p1.x+xs);
       poly[2].x=(int)(p1.x+xs);
     }
   } else {
     double sinpsi = dy/r;
     double cospsi = -dx/r;
     poly[1].x=(int)(p1.x + xs*cospsi - ys*sinpsi);
     poly[2].x=(int)(p1.x + xs*cospsi + ys*sinpsi);
     poly[1].y=(int)(p1.y - xs*sinpsi - ys*cospsi);
     poly[2].y=(int)(p1.y - xs*sinpsi + ys*cospsi);
   }

   GraphicsWin::fillPolygon(poly,3,color);

   points[1].x = 0.5*(double)(poly[1].x+poly[2].x);
   points[1].y = 0.5*(double)(poly[1].y+poly[2].y);
   
   if(tail) drawLine(points[0],points[1],color,thickness);
}

int AspUtil::getColor(char *name) {
   int color=-1;
   if(isdigit(name[0])) color=atoi(name);
   else if(!colorindex(name, &color)) color=-1;
   return color;
}

// intput numbers start from 1
void AspUtil::getFirstLastStep(char *str, int maxInd, int &first, int &last, int &step) {
     first=1, last=maxInd, step=1;
     char *strptr = str;
     char *tokptr; 
     if((tokptr = (char*) strtok(strptr, ":")) != (char *) 0) {
        strcpy(str,tokptr);
        strptr = (char *) 0;
        if((tokptr = (char*) strtok(strptr, ":")) != (char *) 0) step = atoi(tokptr);
     } //  else strcpy(str,str);
     strptr = str;
     if((tokptr = (char*) strtok(strptr, "-")) != (char *) 0) {
        first = atoi(tokptr);
        strptr = (char *) 0;
        if((tokptr = (char*) strtok(strptr, "-")) != (char *) 0) {
           last = atoi(tokptr);
        }
     }

     if(first<1) first=1;
     if(last<first) last=first;
     if(last>maxInd) last=maxInd;
     if(last==first) step=1;
     else if(step > (last-first)) step = last-first;

     // internally, index starts from zero
     first--;
     last--;
}

// read from persistence/Graphics.
void AspUtil::getDisplayOption(string name, string &value)
{
    char path[MAXSTR2];
    struct stat fstat;
    FILE *fp;

    char  buf[MAXSTR], words[MAXWORDNUM][MAXSTR], *tokptr;
    int nw=0;

    bool found=false;

    // find the line in persistence/Graphics
    sprintf(path,"%s/persistence/Graphics",userdir);
    if(stat(path, &fstat) == 0 && (fp = fopen(path, "r")) ) {
      while (fgets(buf,sizeof(buf),fp)) {
        if(strstr(buf,name.c_str()) != buf) continue; 
        found=true;
	break;
      }
    }

    // find the line in system templates/themes/Graphics/Default
    if(!found) {
        sprintf(path,"%s/templates/themes/Graphics/Default",systemdir);
    	if(stat(path, &fstat) == 0  && (fp = fopen(path, "r")) ) {
      	  while (fgets(buf,sizeof(buf),fp)) {
            if(strstr(buf,name.c_str()) != buf) continue; 
            found=true;
	    break;
	  }
	}
   }

   if(!found) return;

      nw=0;
      tokptr = strtok(buf, ", \n");
      while(tokptr != NULL) {
        if(strlen(tokptr) > 0) {
          strcpy(words[nw], tokptr);
          nw++;
        }
        tokptr = strtok(NULL, ", \n");
      }
      if(nw<2) return;

      if(nw == 4) { // RGB color
        unsigned red = atoi(words[1]);
        unsigned green = atoi(words[2]);
        unsigned blue = atoi(words[3]);
	char hexColor[16];
        snprintf(hexColor, sizeof hexColor, "0x%02x%02x%02x",red,green,blue); 
        value = string(hexColor);
      } else value = string(words[1]);
      found=true;

   fclose(fp);
}

int AspUtil::selectPolygon(int x, int y, Dpoint_t *poly, int np) {
    int px,py,pw,ph;
    int i;
    double pi2, a, a1, a2, sum;

    if(np == 0) return 0;

    pi2 = 2.0*M_PI;
    double eps = 0.1*M_PI;

    if(np == 1) {
        px = (int)poly[0].x;
        py = (int)poly[0].y;
        pw = MARKSIZE;
        ph = MARKSIZE;
	return select(x, y, px, py, pw, ph);
    } else if(np == 2) {
	return selectLine(x,y,(int)poly[0].x,(int)poly[0].y,(int)poly[1].x,(int)poly[1].y);
    }

    double w = poly[np-1].x - x;
    double h = poly[np-1].y - y;
    a2 = atan2(h,w);
    if(h < 0) a2 += pi2;

    sum = 0.0;
    for(i=0; i<np; i++) {

        a1 = a2;
        w = poly[i].x - x;
        h = poly[i].y - y;
        a2 = atan2(h,w);
        if(h < 0) a2 += pi2;
        /* get smallest angle */
        a = fabs(a2 - a1);
        if(a > M_PI) a = pi2 - a;
        sum += a;
    }
   
    if(fabs(sum - pi2) < eps) return 1;
    else return 0;
}

string AspUtil::subParamInStr(string label) {
   char str[MAXSTR],tmpStr[MAXSTR];
   size_t p1=string::npos;
   size_t p2=string::npos;

   // substitute pararameters in $$
   string thisLabel=string(label);
   p1=thisLabel.find("$",0);
   while(p1 != string::npos) {
      p2=thisLabel.find("$",p1+1);
      if(p2 != string::npos) {
         strcpy(tmpStr,thisLabel.substr(p1+1,p2-p1-1).c_str());
         string strValue = AspUtil::getString(string(tmpStr),"");
         if(strValue=="") {
            double value = AspUtil::getReal(string(tmpStr),0.0);
            sprintf(str,"%.3f",value);
	    strValue = string(str);
	 }
         thisLabel = thisLabel.substr(0,p1) + strValue + thisLabel.substr(p2+1);
         p1=thisLabel.find("$",p1+strValue.size());
      } else p1 = string::npos;
   }

   return thisLabel;
}
