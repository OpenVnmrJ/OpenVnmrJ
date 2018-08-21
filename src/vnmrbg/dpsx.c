/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "vnmrsys.h"
#include "dpsdef.h"
#include "vfilesys.h"
#include "pvars.h"
#include "dps.h"
#include "group.h"
#include "symtab.h"
#include "tools.h"
#include "variables.h"

extern void unixPathToWin(char *path,char *buff,int maxlength);

static  int     rfchan[TOTALCH];
static  int     visibleCh[TOTALCH];
static  int     imageType = 0;
static  int     debug = 0;
static  int     items[TOTALCH];
static  int     lines[TOTALCH];
static  double  phases[TOTALCH];
static  double  powers[TOTALCH];
static  double  gradMax[TOTALCH];
static  double  times[TOTALCH];
static  double  chX[TOTALCH];
static  double  chY[TOTALCH];
static  double  chY2[TOTALCH];
static  double  gradMaxLevel;
static  double  seqTime;
static  char    tmpStr[MAXPATH];
static  char    tmpPath[MAXPATH];
static  char    seqName[MAXPATH];
static SRC_NODE *chanNode[TOTALCH];
static  FILE    *fds[TOTALCH];
// static  FILE    *phaseFds[TOTALCH];
static  SRC_NODE *start_node = NULL;

static char *channel_label[] = {"Tx", "RF", "Dec", "Dec2", "Dec3", "Dec4", "dec5",
                           "Acq", "Gx", "Gy", "Gz", "Acq", "Acq2" };
static char *image_labels[] = {"Tx", "RF", "Dec", "Dec2", "Dec3", "Dec4", "dec5",
                           "Acq", "Gro", "Gpe", "Gss", "Acq", "Acq2" };
// static int channel_columns[] = {4, 4, 4, 4, 4, 4, 4,
//                            3, 3, 3, 3, 3, 3};

#define ITEM     0
#define PATTERN  1
#define FILENAME 2
#define NAME     3
#define EVENT    4
#define INFO     5

#define JSHPUL     271
#define JSHGRAD    272
#define JGRPPULSE  273

#define JLOOP      301
#define JENDLOOP   302
#define JRDBYTE    303
#define JWRBYTE    304

static char *item_labels[] = {"#item", "#pattern", "#filename", "#itemname", 
             "#event", "#info" };


static void close_fds()
{
     int i;

     for (i = 0; i < TOTALCH; i++) {
         if (fds[i] != NULL) {
             fsync(fileno(fds[i]));
             fclose(fds[i]);
         }
         fds[i] = NULL;
         /**
         if (phaseFds[i] != NULL) {
             fsync(fileno(phaseFds[i]));
             fclose(phaseFds[i]);
         }
         phaseFds[i] = NULL;
         ***/
     }
}


static void write_info()
{
     int    k;
     char **labels;
     FILE *infoFd;
     
     if (imageType)
         labels = image_labels;
     else
         labels = channel_label;
     sprintf(tmpStr, "%s/.dpsViewInfo", curexpdir);
     unixPathToWin(tmpStr, tmpPath, MAXPATH);
     infoFd = fopen(tmpPath, "w");
     if (infoFd == NULL)
         return;
     fprintf(infoFd, "seqname  %s\n", seqName);
     fprintf(infoFd, "seqtime  %g\n", seqTime);
     fprintf(infoFd, "debug  %d\n", debug);
     for (k = 1; k < TOTALCH; k++) {
          if (rfchan[k] && visibleCh[k]) {
              fprintf(infoFd, "name %d  %s\n", k, labels[k]);
              fprintf(infoFd, "item_num %d %d \n",k, items[k]);
              fprintf(infoFd, "line_num %d %d \n",k, lines[k]);
              sprintf(tmpStr, "%s/.dpsChannel%d", curexpdir, k);
              unixPathToWin(tmpStr, tmpPath, MAXPATH);
              fprintf(infoFd, "file %d  %s\n", k, tmpPath);
          }
     }
     fsync(fileno(infoFd));
     fclose(infoFd);
}

void dpsx_init(int type, int d, char *name)
{
     int i;

     imageType = type;
     debug = d;
     for (i = 0; i < TOTALCH; i++) {
         rfchan[i] = 0;
         visibleCh[i] = 1;
         times[i] = 0.0;
         chX[i] = 0.0;
         chY[i] = 0.0;
         chY2[i] = 0.0;
         phases[i] = 0.0;
         gradMax[i] = 2.0;
         powers[i] = 2.0;
     }
     close_fds();
     strcpy(seqName, "dps");
     if (name != NULL)
         strcpy(seqName, name);
}


void dpsx_setValues(int type, int num, int *values)
{
     int k;
     if (num > TOTALCH)
         num = TOTALCH;
     switch (type) {
         case TODEV:
                  for (k = 0; k < num; k++)
                       rfchan[k] = values[k];
                  break;
         case DEVON:
                  for (k = 0; k < num; k++)
                       visibleCh[k] = values[k];
                  break;
     }
}

void dpsx_setDoubles(int type, int num, double *values)
{
     int k;
     if (num > TOTALCH)
         num = TOTALCH;
     switch (type) {
         case RF_POWERS:
                  for (k = 0; k < num; k++) {
                     //  powers[k] = values[k];
                  }
                  break;
         case GRAD_POWERS:
                  gradMaxLevel = 2.0;
                  if (num > 3)
                      num = 3;
                  for (k = 0; k < num; k++) {
                       gradMax[k] = values[k];
                       if (values[k] > 0.0)
                           powers[k+GRADDEV] = values[k];
                       if (values[k] > gradMaxLevel)
                           gradMaxLevel = values[k];
                  }
                  break;
     }
}

static void set_times()
{
     int dev, i, inParallel, timeInc;
     SRC_NODE *node, *brnode;
     double   t_time, t0, t1, ptime, p;

     if (start_node == NULL)
         return;
     for (i = 0; i < TOTALCH; i++) {
         rfchan[i] = 0;
         times[i] = 0.0;
         chanNode[i] = NULL;
     }
     node = start_node;
     t_time = 0.0;
     seqTime = 0.0;
     inParallel = 0;
     while (node != NULL) {
        timeInc = 1;
        dev = node->device;
        t1 = t_time;
        if (dev >= 0) {
            if (times[dev] > t1)
                t1 = times[dev];
        }
        if ((dev >= 0) && (node->flag & XMARK)) {
            if (!(node->flag & XWDH))
                timeInc = 0;
        }
        if (dev > 0)
            rfchan[dev] = 1;
        if (dev >= GRADDEV && node->type != DPESHGR2) {
            brnode = chanNode[dev];
            if (brnode != NULL && brnode->xetime < t_time)
                brnode->xetime = t1;
            chanNode[dev] = NULL;
        }
        switch (node->type) {
          case PARALLEL:
          case PARALLELSTART:
                  inParallel = 1; 
                  timeInc = 0;
                  break;
          case PARALLELEND:
                  inParallel = 0; 
                  timeInc = 0;
                  break;
          case PARALLELSYNC:
                  break;
          case XEND:
                  break;
          case RLLOOP:
                  break;
          case RLLOOPEND:
                  break;
          case KZLOOPEND:
                  timeInc = 0;
                  break;
          case GRAD:
          case RGRAD:
          case PEGRAD:
          case OBLGRAD:
          case DPESHGR:
                  if (node->power != 0.0)
                      chanNode[dev] = node;
                  break;
          case RDBYTE:
                  timeInc = 1;
                  break;
          case WRBYTE:
                  timeInc = 1;
                  node->ptime = 4.0e-6;  // 4 usec
                  break;
        
          default:
                  break;
        }
        node->xstime = t1;
        node->xetime = t1;
        if (timeInc) {
           ptime = 0.0;
           brnode = node;
           while (brnode != NULL) {
               t0 = brnode->ptime + node->rg1 + node->rg2;
               if (t0 > ptime)
                  ptime = t0;
               if (brnode->type != DELAY && brnode->ptime != 0.0) {
                  p = brnode->power;
                  if (p < 0.0)
                      p = 0.0 - p;
                  if (p > powers[dev])
                      powers[dev] = p;
               }
               brnode = brnode->bnode;
           }
           if (ptime > 0.0) {
               brnode = node;
               while (brnode != NULL) {
                  brnode->xstime = t1 + (ptime - brnode->ptime) / 2.0; 
                  brnode->xetime = brnode->xstime + brnode->ptime;
                  brnode = brnode->bnode;
               }
               t0 = t1 + ptime;
               brnode = node;
               while (brnode != NULL) {
                  times[brnode->device] = t0;
                  brnode = brnode->bnode;
               }
               if (node->wait)
                   t_time = t0;
               if (t0 > seqTime)
                   seqTime = t0;
           }
        }
        node = node->dnext;
     }
     if (t_time > seqTime)
        seqTime = t_time;
     for (i = 0; i < TOTALCH; i++) {
         brnode = chanNode[dev];
         if (brnode != NULL && brnode->xetime < t_time)
             brnode->xetime = t_time;
     }
     node = start_node;
     while (node != NULL) {
        brnode = node;
        while (brnode != NULL) {
            brnode->xstime = brnode->xstime * 1.0e+6;
            brnode->xetime = brnode->xetime * 1.0e+6;
            brnode = brnode->bnode;
        }
        node = node->dnext;
     }
     seqTime = seqTime * 1.0e+6;
}


static void print_item(int dev, int type, int code, char *data)
{
    FILE *fd;

    fd = fds[dev];
    if (fd == NULL)
         return;
 
    if (type == ITEM) {
        items[dev] = items[dev] + 1;
        fprintf(fd, "%s %d\n", item_labels[ITEM],code);
        type = NAME;
    }
    fprintf(fd, "%s %s\n", item_labels[type], data);
}

/**
static void print_info(int dev, int type, int code, char *data)
{
}
**/

static void print_size(int dev, int num)
{
    FILE *fd;

    fd = fds[dev];
    if (fd == NULL)
         return;
    lines[dev] = lines[dev] + num;
    fprintf(fd, "#size %d\n", num);
}

static void print_dummy(int dev) {
    FILE *fd;

    fd = fds[dev];
    if (fd == NULL)
         return;
    items[dev] = items[dev] + 1;
    fprintf(fd, "%s 0\n", item_labels[ITEM]);
    fprintf(fd, "%s %s\n", item_labels[NAME], "dummy");
}

static void patch_channel(int dev, double x)
{
     // FILE     *fd;

     if (chX[dev] > x)
         return;
    /***********
     fd = fds[dev];
     if (fd == NULL)
         return;
     print_dummy(dev);
     if (chY[dev] != 0.0) {
         print_size(dev, 2);
         fprintf(fd, "%.3f %g %g 0 0 1\n", x, chY[dev], chY[dev]); 
     }
     else
         print_size(dev, 1);
     fprintf(fd, "%.3f 0.0 0.0  0 0 1\n", x);
    ***********/
     chY[dev] = 0.0;
     chY2[dev] = 0.0;
     chX[dev] = x;
}

static void end_channels()
{
     int      k;
     FILE     *fd;

     for (k = 1; k < TOTALCH; k++) {
         fd = fds[k];
         if (fd != NULL) {
              print_dummy(k);
              print_size(k, 1);
              fprintf(fd, "%.3f %g %g  0 0 1\n", seqTime, chY[k], chY[k]);
         }
     }
}

static double adjust_height(SRC_NODE *node)
{
    double r, maxV;

    if (node->power == 0.0)
        return (0.0);
    if (node->device < GRADDEV)
        maxV = powers[node->device];
    else
        maxV = gradMaxLevel;
    r = fabs(node->power / maxV) * 0.7 + 0.3;
    if (r > 1.0)
        r = 1.0;
    if (node->device >= GRADDEV) {
        if (node->power < 0.0)
            r = -r;
    }
    return (r);
}

static void update_height(SRC_NODE *node)
{
     int      dev;
     double   v;
     FILE     *fd;

     while (node != NULL) {
         dev = node->device;
         fd = fds[dev];
         if (fd == NULL)
              return;
         print_dummy(dev);
         print_size(dev, 2);
         fprintf(fd, "%.3f %g %g 0 0 1\n", node->xstime, chY[dev], chY[dev]); 
         v = adjust_height(node);
         fprintf(fd, "%.3f %g %g 0 0 1\n", node->xstime, v, v);

         chY[dev] = v;
         chX[dev] = node->xstime;
         node = node->bnode;
     }
}

static void update_gradient(SRC_NODE *node)
{
     int      dev;
     FILE     *fd;

     dev = node->device;
     if (dev < GRADDEV)
         return;
     if (chY[dev] == 0.0)
         return;
     if (chX[dev] > node->xstime)
         return;
     fd = fds[dev];
     if (fd == NULL)
         return;
     print_item(dev, ITEM, GRAD, "dummy"); 
     print_size(dev, 2);
     fprintf(fd, "%.3f %g %g 0 0 1\n", node->xstime, chY[dev], chY[dev]); 
     fprintf(fd, "%.3f 0 0 0 0 1\n", node->xstime);
     chY[dev] = 0.0;
     chY2[dev] = 0.0;
     chX[dev] = node->xstime;
}

static void update_shaped_gradient(SRC_NODE *node, double v0, double v1)
{
     int      dev;
     FILE     *fd;

     dev = node->device;
     if (dev < GRADDEV)
         return;
     if (chX[dev] > node->xstime)
         return;
     fd = fds[dev];
     if (fd == NULL)
         return;
     print_item(dev, ITEM, GRAD, "dummy");
     print_size(dev, 2);
     fprintf(fd, "%.3f %g 0 %g 0 1\n", node->xstime, chY[dev], chY2[dev]);
     fprintf(fd, "%.3f %g 0 %g 0 1\n", node->xstime, v0, v1);
     chX[dev] = node->xstime;
     chY[dev] = v0;
     chY2[dev] = v1;
}

static void dummy_shapepulse(FILE *fd, SRC_NODE *node)
{
     int dev;
     COMMON_NODE *comnode;

     dev = node->device;
     comnode = (COMMON_NODE *) &node->node.common_node;
     if (dev < GRADDEV) {
         if (node->type == GRPPULSE)
             print_item(dev, ITEM, JGRPPULSE, node->fname);
         else
             print_item(dev, ITEM, JSHPUL, node->fname);
     }
     else
         print_item(dev, ITEM, JSHGRAD, node->fname);
     if (comnode->pattern != NULL)
         print_item(dev, PATTERN, 0, comnode->pattern);
     fprintf(fd, "#duration  %g\n", node->xetime - node->xstime);
     print_size(dev, 3);
     fprintf(fd, "%.3f 0.0 0.0 0 0 1\n", node->xstime);
     fprintf(fd, "%.3f 0.8 0.8 0 0 1\n", node->xstime);
     fprintf(fd, "%.3f 0.0 0.0 0 0 1\n", node->xetime);
     chX[dev] = node->xetime;
     chY[dev] = 0.0;
     chY2[dev] = 0.0;
}

static void gradient(SRC_NODE *node, int isPeGrad)
{
     int      dev, type, *val;
     double   v, *fval;
     char    **vlabel, **flabel;
     FILE     *fd;
     COMMON_NODE *comnode;

     vlabel = NULL;
     val = NULL;
     dev = node->device;
     if (dev < GRADDEV)
         return;
     if (chY[dev] == 0.0) {
         if (node->power == 0.0)
             return;
         if (node->xetime <= node->xstime)
             return;
     }
     fd = fds[dev];
     if (fd == NULL)
         return;
     type = GRAD;
     comnode = (COMMON_NODE *) &node->node.common_node;
     fval = comnode->fval;
     flabel = comnode->flabel;
     if (isPeGrad) {
        val = comnode->val;
        vlabel = comnode->vlabel;
        if (fval[1] != 0.0)
           type = PEGRAD;
        else
           isPeGrad = 0;
     }
     print_item(dev, ITEM, type, node->fname); 
     fprintf(fd, "#info Level:  %s = %g\n",flabel[0], node->power);
     if (isPeGrad) {
        if (flabel[1] != NULL)
           fprintf(fd, "#info Step: %s = %g\n", flabel[1],fval[1]);
        if (vlabel[0] != NULL)
           fprintf(fd, "#info Mult: %s = %d\n", vlabel[0], val[1]);
        print_size(dev, 5);
     }
     else
        print_size(dev, 3);
     v = adjust_height(node);
     fprintf(fd, "%.3f %g %g 0 0 1\n", node->xstime, chY[dev], chY[dev]);
     if (isPeGrad)
         fprintf(fd, "%.3f 0 0 0 0 1\n", node->xstime);
     fprintf(fd, "%.3f %g %g 0 0 1\n", node->xstime, v, v);
     fprintf(fd, "%.3f %g %g 0 0 1\n", node->xetime, v, v);
     chY[dev] = v;
     if (isPeGrad) {
         fprintf(fd, "%.3f 0 0 0 0 1\n", node->xetime);
         chY[dev] = 0.0;
     }
     chX[dev] = node->xetime;
}

static void pulse(SRC_NODE *node)
{
     int      dev;
     double   v;
     FILE     *fd;

     while (node != NULL) {
         if (node->ptime > 0.0 && node->power != 0.0) {
             dev = node->device;
             fd = fds[dev];
             if (fd == NULL)
                  return;
             patch_channel(dev, node->xstime);
             fprintf(fd, "#duration  %g\n", node->xetime - node->xstime);
             fprintf(fd, "#info Duration:  %g usec\n", node->xetime - node->xstime);
             if (dev < GRADDEV) {
                 if (node->type == GRPPULSE)
                     print_item(dev, ITEM, GRPPULSE, node->fname); 
                 else
                     print_item(dev, ITEM, PULSE, node->fname); 
                 fprintf(fd, "#info Level:     %g dB\n", node->power);
             }
             else {
                 print_item(dev, ITEM, GRAD, node->fname); 
                 fprintf(fd, "#info Power:     %g dB\n", node->power);
             }
             print_size(dev, 4);
             fprintf(fd, "%.3f 0.0 0.0  0 0 1\n", node->xstime);
             v = adjust_height(node);
             fprintf(fd, "%.3f %g %g 0 0 1\n", node->xstime, v, v);
             fprintf(fd, "%.3f %g %g 0 0 1\n", node->xetime, v, v);
             fprintf(fd, "%.3f 0.0 0.0  0 0 1\n", node->xetime);
             chY[dev] = 0.0;
             chX[dev] = node->xetime;
         }
         node = node->bnode;
     }
}

static void grad_shapepulse(SRC_NODE *node)
{
     int    dev, k, num, repeats;
     double t, t0, vx, tx, v0, v1;
     double *amp;
     SHAPE_NODE   *sh_node;
     FILE     *fd;

     dev = node->device;
     fd = fds[dev];
     if (fd == NULL)
         return;
     sh_node = node->shapeData;
     if (sh_node == NULL || sh_node->dataSize < 2) {
         dummy_shapepulse(fd, node);
         return;
     }
     t = node->xstime;
     if (node->updateLater <= 0)
         patch_channel(dev, t);

     vx = adjust_height(node);
     // v1 = 32767.0;
     v1 = fabs(sh_node->maxData);
     if (fabs(sh_node->minData) > v1)
         v1 = fabs(sh_node->minData);

     vx = vx / v1;
     amp = sh_node->amp;
     t0 = t;
     tx = (node->xetime - node->xstime) / (double) (sh_node->dataSize - 1); 
     v0 = *amp - 0.1;
     num = 0;
     repeats = 0;
     for (k = 0; k < sh_node->dataSize; k++) {
         if (v0 != *amp) {
            num++;
            if (repeats > 0) {
                num++;
                repeats = 0;
            }
            v0 = *amp;
         }
         else
            repeats++;
         amp++;
     }

     amp = sh_node->amp;
     v0 = *amp;
     v1 = v0 * vx;
     update_shaped_gradient(node, v1, v0);

     if (node->type == DPESHGR)
         print_item(dev, ITEM, DPESHGR, node->fname);
     else if (node->type == DPESHGR2)
         print_item(dev, ITEM, DPESHGR2, node->fname);
     else
         print_item(dev, ITEM, SHGRAD, node->fname);
     if (sh_node->name != NULL)
         print_item(dev, PATTERN, 0, sh_node->name);
     if (sh_node->filename != NULL)
         print_item(dev, FILENAME,0, sh_node->filename);
     fprintf(fd, "#duration  %g\n", node->xetime - node->xstime);

     // print_size(dev, sh_node->dataSize + 2);
     print_size(dev, num);
     // fprintf(fd, "%.3f  0.0  0.0  0 0 1\n", t);
     v0 = *amp - 0.1;
     repeats = 0;
     for (k = 0; k < sh_node->dataSize; k++) {
         if (v0 != *amp) {
             if (repeats > 0) {
                 v1 = v0 * vx;
                 fprintf(fd, "%.3f %g 0 %g 0 1\n", t - tx, v1, v0);
                 repeats = 0;
             }
             v0 = *amp;
             v1 = v0 * vx;
             fprintf(fd, "%.3f %g 0 %g 0 1\n", t, v1, v0);
         }
         else
            repeats++;
         t += tx;
         amp++;
     }
     t = t - tx;
     // fprintf(fd, "%.3f  0.0  0.0  0 0 1\n", t);
     chX[dev] = t;
     chY[dev] = v1;
     chY2[dev] = v0;
}

static void rf_shapepulse(SRC_NODE *node)
{
     int    dev, k;
     double t, vx, tx, v1, v2;
     double *amp, *phase;
     SHAPE_NODE   *sh_node;
     FILE     *fd;

     dev = node->device;
     fd = fds[dev];
     if (fd == NULL)
         return;
     sh_node = node->shapeData;

     if (sh_node == NULL || sh_node->dataSize < 2) {
         dummy_shapepulse(fd, node);
         return;
     }
     t = node->xstime;
     tx = (node->xetime - node->xstime) / (double) (sh_node->dataSize - 1);
     patch_channel(dev, t);
     if (node->type == GRPPULSE)
         print_item(dev, ITEM, GRPPULSE, node->fname);
     else
         print_item(dev, ITEM, SHPUL, node->fname);
     if (sh_node->name != NULL)
         print_item(dev, PATTERN, 0, sh_node->name);
     if (sh_node->filename != NULL)
         print_item(dev, FILENAME,0, sh_node->filename);
     fprintf(fd, "#duration  %g\n", node->xetime - node->xstime);
     vx = adjust_height(node);
     vx = vx / 1024.0;
     amp = sh_node->amp;
     phase = sh_node->phase;
     print_size(dev, sh_node->dataSize + 2);
     fprintf(fd, "%.3f 0.0 0.0 0 0 1\n", t);
     for (k = 0; k < sh_node->dataSize; k++) {
         v1 = *amp * vx;
         v2 = *phase * vx;
         fprintf(fd, "%.3f %g %g %g %g 1\n", t, v1, v2, *amp, *phase);
         t += tx;
         amp++;
         phase++;
     }
     t = t - tx;
     fprintf(fd, "%.3f 0.0 0.0 0 0 1\n", t);
     chX[dev] = t;
     chY[dev] = 0.0;
}


static void shapepulse(SRC_NODE *node)
{
     COMMON_NODE  *cmnode;

     while (node != NULL) {
         if (node->updateLater <= 0)
             update_gradient(node);
         if (node->ptime > 0.0 && node->power != 0.0 && node->visible) {
             cmnode = (COMMON_NODE *) &node->node.common_node;
             if (strlen(cmnode->pattern) == 1 && *cmnode->pattern == '?') {
                 pulse(node);
             }
             else {
                 if (node->device >= GRADDEV)
                     grad_shapepulse(node);
                 else
                     rf_shapepulse(node);
             }
         }
         node = node->bnode;
     }
}

static void acquire(SRC_NODE *node)
{
     int     dev;
     FILE    *fd;

     if (node->ptime <= 0.0)
         return;
     dev = ACQDEV;
     fd = fds[dev];
     if (fd == NULL)
         return;
     patch_channel(dev, node->xstime);
     print_item(dev, ITEM, ACQUIRE, node->fname);
     fprintf(fd, "#duration  %g\n", node->xetime - node->xstime);
     fprintf(fd, "#info Duration:  %g usec\n", node->xetime - node->xstime);
     print_size(dev, 4);
     fprintf(fd, "%.3f 0.0 0.0 0 0 1\n", node->xstime);
     fprintf(fd, "%.3f 0.8 0.8 0 0 1\n", node->xstime);
     fprintf(fd, "%.3f 0.8 0.8 0 0 1\n", node->xetime);
     fprintf(fd, "%.3f 0.0 0.0 0 0 1\n", node->xetime);
     chX[dev] = node->xetime;
     chY[dev] = 0.0;
}

static void print_loop(SRC_NODE *node, int isOpen)
{
     int     dev;
     FILE    *fd;
     COMMON_NODE  *comnode; 

     dev = node->device;
     fd = fds[dev];
     if (fd == NULL)
         return;
     comnode = (COMMON_NODE *) &node->node.common_node;
     if (isOpen) {
         print_item(dev, ITEM, JLOOP, node->fname);
         fprintf(fd, "#value  %d \n", comnode->val[3]);
         fprintf(fd, "#info Loops:  %d\n", comnode->val[3]);
     }
     else {
         print_item(dev, ITEM, JENDLOOP, node->fname);
     }

     print_size(dev, 1);
     fprintf(fd, "%.3f %g %g  0 0 1\n", node->xstime, chY[dev], chY[dev]); 
}

/**
static void print_event(SRC_NODE *node)
{
}
**/

static void read_MRIByte(SRC_NODE *node)
{
     int     dev;
     FILE    *fd;
     COMMON_NODE  *comnode; 

     // dev = node->device;
     dev = TODEV;
     fd = fds[dev];
     if (fd == NULL)
         return;
     comnode = (COMMON_NODE *) &node->node.common_node;
     print_item(dev, ITEM, JRDBYTE, node->fname);
     fprintf(fd, "#duration  %g\n", node->xetime - node->xstime);
     fprintf(fd, "#info Duration:  %g usec\n", node->xetime - node->xstime);
     fprintf(fd, "#info Bytes:     %s = %d\n", comnode->vlabel[0], comnode->val[1]);
     print_size(dev, 2);
     fprintf(fd, "%.3f 1 1  0 0 1\n", node->xstime);
     fprintf(fd, "%.3f 1 1  0 0 1\n", node->xetime);

}

static void write_MRIByte(SRC_NODE *node)
{
     int     dev;
     FILE    *fd;
     COMMON_NODE  *comnode; 

     dev = TODEV;
     fd = fds[dev];
     if (fd == NULL)
         return;
     comnode = (COMMON_NODE *) &node->node.common_node;
     print_item(dev, ITEM, JWRBYTE, node->fname);
     fprintf(fd, "#info Bytes: %s = %d\n", comnode->vlabel[0], comnode->val[1]);
     print_size(dev, 2);
     fprintf(fd, "%.3f 1 1  0 0 1\n", node->xstime);
     fprintf(fd, "%.3f 1 1  0 0 1\n", node->xetime);
}


static int open_fds()
{
     int k, retVal;
     char **labels;
     FILE *fd;
     
     if (imageType)
         labels = image_labels;
     else
         labels = channel_label;
     retVal = 1;
     rfchan[ACQDEV] = 1;
     for (k = 1; k < TOTALCH; k++) {
          fds[k] = NULL;
          items[k] = 0;
          lines[k] = 0;
          // phaseFds[k] = NULL;
          if (rfchan[k] && visibleCh[k]) {
              sprintf(tmpStr, "%s/.dpsChannel%d", curexpdir, k);
              unixPathToWin(tmpStr, tmpPath, MAXPATH);
              fd = fopen(tmpPath, "w");
              if (fd == NULL)
                  retVal = 0;
              fds[k] = fd;
              fprintf(fd, "#channel %d\n", k);
              fprintf(fd, "#name %s\n", labels[k]);
              fprintf(fd, "#seqtime %g\n", seqTime); 
              fprintf(fd, "#columns 6\n");
              fprintf(fd, "#debug  %d\n", debug);
              // fprintf(fd, "#columns %d\n", channel_columns[k]);
              print_dummy(k);
              print_size(k, 1); 
              fprintf(fd, "0.0  0.0  0.0 0 0 1\n");
          }
     }
     if (retVal == 0)
          close_fds();
     return (retVal);
}


static int outputSeq()
{
     int dev;
     SRC_NODE *node, *brnode;
     COMMON_NODE  *comnode; 
     FILE     *fd;

     if (start_node == NULL)
         return(0);

     if (open_fds() < 1)
         return (0);
     node = start_node;
     while (node != NULL) {
        dev = node->device;
        if (dev < 1)
             dev = 1;
        fd = fds[dev];
        if (fd == NULL) {
            node = node->dnext;
            continue;
        }
        comnode = (COMMON_NODE *) &node->node.common_node;
        switch (node->type) {
          case ACQUIRE: 
          case SAMPLE:   // sample
          case JFID: 
          case FIDNODE: 
                   acquire(node);
                   break;
          case DELAY:
          case INCDLY:  // incdelay
          case VDELAY:  // vdelay
          case VDLST:   // vdelay_list
                   break;
          case PULSE: 
          case SMPUL:   // simpulse, sim3pulse
                   pulse(node);
                   break;
          case SHPUL:
          case SHVPUL:
          case SMSHP:
          case APSHPUL: // apshaped_pulse, apshaped_decpulse,apshaped_dec2pulse
          case OFFSHP:  // shapedpulselist, genRFShapedPulseWithOffset
                   shapepulse(node);
                   break;
          case GRPPULSE: // GroupPulse, TGroupPulse
                   shapepulse(node);
                   break;
          case SHACQ:  // ShapedXmtNAcquire
          case SPACQ:  // SweepNOffsetAcquire, SweepNAcquire
                   acquire(node);
                   break;
          case HLOOP: // starthardloop
                   print_loop(node, 1);
                   break;
          case ENDHP: 
                   print_loop(node, 0);
                   break;
          case SLOOP:  // loop
          case GLOOP:  // gemini loop
          case MSLOOP: // msloop
          case PELOOP: // peloop
          case PELOOP2: // peloop2
          case NWLOOP: // nwloop
                   print_loop(node, 1);
                   break;
          case ENDSP:    // endloop
          case ENDSISLP:  // endpeloop, endmsloop, endpeloop2
          case NWLOOPEND: // endnwloop
                   print_loop(node, 0);
                   break;
          case STATUS: 
          case SETST: 
                   break;
          case GRAD: 
          case RGRAD: 
                   gradient(node, 0);
                   break;
          case MGPUL:  // magradpulse, vagradpulse, simgradpulse, oblique_gradpulse
                   pulse(node);
                   break;
          case MGRAD: // magradient, vagradient,simgradient
                   gradient(node, 0);
                   break;
          case OBLGRAD:  // obl_gradient,oblique_gradient,pe_oblshapedgradient
                   gradient(node, 0);
                   break;
          case PEGRAD:  // pe_gradient,pe2_gradient,phase_encode_gradient
                   gradient(node, 1);
                   break;
          case OBLSHGR: // obl_shapedgradient,obl_shaped3gradient,oblique_shapedgradient
          case SHGRAD:  // shapedgradient,shaped2Dgradient,mashapedgradient
                   shapepulse(node);
                   break;
          case PEOBLG:  // deprecated
                   break;
          case PESHGR:  // pe_shaped3gradient,pe3_shaped3gradient,pe2_shaped3gradient
          case SHVGRAD: // shapedvgradient
          case SHINCGRAD: // shapedincgradient,shaped_INC_gradient
          case PEOBLVG:
                   shapepulse(node);
                   break;
          case DPESHGR:  // pe3_shaped3gradient_dual 
                   shapepulse(node);
                   brnode = node->bnode;
                   if (brnode != NULL) {
                      brnode->xstime = node->xstime;
                      brnode->xetime = node->xetime;
                      brnode->ptime = node->ptime;
                      shapepulse(brnode);
                   }
                   break;
          case PRGON:  // obsprgon,decprgon,dec2prgon
                   break;
          case XPRGON:
                   break;
          case VPRGON:  // obsprgonOffset,decprgonOffset
                   break;
          case PRGMARK: 
          case VPRGMARK: 
                   break;
          case VGRAD:   // vgradient,Pvgradient,S_vgradient
                   gradient(node, 0);
                   break;
          case INCGRAD:  // incgradient,Sincgradient,Wincgradient
                   break;
          case ZGRAD: // zgradpulse
                   pulse(node);
                   break;
          case DEVON:  // xmtron,rfon,decon, rfoff,decoff,xmtroff
                   update_height(node);
                   break;
          case VDEVON: 
                   update_height(node);
                   break;
          case RFONOFF: 
                   update_height(node);
                   break;
          case PWRF: 
          case PWRM: 
          case RLPWRF: 
          case VRLPWRF: 
          case RLPWRM:
                   break;
          case DECLVL:  // declvlon, declvloff
                   break;
          case RLPWR: 
          case DECPWR: 
          case POWER: 
                   break;
          case OFFSET: // offset,ioffset,decoffset,obsoffset 
          case POFFSET: // poffset_list,position_offset,position_offset_list
                   break;
          case LOFFSET: 
                   break;
          case VFREQ:  // vfreq
          case VOFFSET: // voffset
                   break;
          case LKHLD:  // lk_hold
                   break;
          case LKSMP:  // lk_sample
                   break;
          case AMPBLK:  // decblankon,obsunblank,decunblank,blankingon
                   break;
          case SPINLK:  // spinlock, decspinlock,genspinlock
                   print_item(dev, ITEM, node->type, node->fname);
                   if (comnode->vlabel[0] != NULL)
                       print_item(dev, PATTERN, 0, comnode->vlabel[0]);
                   print_size(dev, 1);
                   // time, duration, cycles
                   fprintf(fd, "%.3f %g %g 0 0 1\n", node->xstime,comnode->fval[0],
                          (double)comnode->val[2] );
                   break;
          case XGATE: 
                   print_item(dev, ITEM, node->type, node->fname);
                   print_size(dev, 1);
                   fprintf(fd, "%.3f %g 0 0 0 1\n", node->xstime,comnode->fval[0]);
                   break;
          case ROTORP:   // rotorperiod
          case ROTORS:   // rotorsync
          case BGSHIM: 
                   break;
          case SHSELECT: 
          case GRADANG: 
                   break;
          case ROTATEANGLE: // rot_angle_list
          case EXECANGLE:   // exe_grad_rotation
                   break;
          case SETRCV:     // setreceiver
                   break;
          case SPHASE:    // xmtrphase, dcplrphase, dcplr2phase, dcplr3phase
                   break;
          case PSHIFT:   // phaseshift
                   break;
          case PHASE:    // txphase, decphase
                   break;
          case SPON: // sp1on, sp2on
                   break;
          case RCVRON:   // rcvron, recon, recoff, rcvroff
                   break;
          case PRGOFF:  // prg_dec_off,obsprgoff,decprgoff,dec2prgoff
          case XPRGOFF: 
                   break;
          case HDSHIMINIT:   // hdwshiminit
                   break;
          case ROTATE:  // rotate, rotate_angle
                   break;
          case DCPLON: 
                   break;
          case DCPLOFF: 
          case XDEVON: 
                   break;
          case XTUNE:   // set4Tune
                   break;
          case XMACQ:   // XmtNAcquire
                   acquire(node);
                   break;
          case ACQ1:    // startacq
                   break;
          case TRIGGER:  // triggerSelect
          case SETGATE:  // setMRIUserGates
                   break;
          case RDBYTE:   // readMRIUserByte
                   read_MRIByte(node);
                   break;
          case WRBYTE:   // writeMRIUserByte
                   write_MRIByte(node);
                   break;
          case SETANGLE:  // set_angle_list
                   break;
          case ACTIVERCVR: // setactivercvrs
                   break;
          case PARALLELSTART: 
                   break;
          case PARALLELEND: 
                   break;
          case XEND: 
                   break;
          case XDOCK:
                   break;
          case PARALLELSYNC: 
                   break;
          case RLLOOP:   // rlloop 
          case KZLOOP:   // kzloop
                   print_loop(node, 1);
                   break;
          case RLLOOPEND: // rlendloop
          case KZLOOPEND: // kzendloop
                   print_loop(node, 0);
                   break;
          default:
                   // Wprintf("dps draw: unknown code %d \n", node->type);
                   break;
        }

        node = node->dnext;
     }

     end_channels();
     return (1);
}


void dpsx(SRC_NODE *node)
{
     int  retVal;

     start_node = node;
     close_fds();
     if (start_node == NULL)
         return;
     set_times();
     retVal = outputSeq();
     close_fds();
     if (retVal) {
         write_info();
         sprintf(tmpStr, "%s/.dpsViewInfo", curexpdir);
         unixPathToWin(tmpStr, tmpPath, MAXPATH);
         writelineToVnmrJ("dps scopewindow ", tmpPath);
     }
}

