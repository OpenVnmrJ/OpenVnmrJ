/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * 
 *
 */

import java.awt.*;
import java.awt.event.*;
import java.util.*;

class PlotArrowDraw implements PlotDefines
{
   public PlotArrowDraw()
   {
	upxs = new int[4];
	upys = new int[4];
	dnxs = new int[4];
	dnys = new int[4];
	rxs = new int[4];
	rys = new int[4];
	lxs = new int[4];
	lys = new int[4];
	rdnxs = new int[4];
	rdnys = new int[4];
	rupxs = new int[4];
	rupys = new int[4];
	lupxs = new int[4];
	lupys = new int[4];
	ldnxs = new int[4];
	ldnys = new int[4];
	xs = new int[4];
	ys = new int[4];
   }

   public void draw(Graphics gc, int type, int x, int y)
   {
	switch (type) {
	 case ARROWUP:
		draw_up(gc, x, y);
		break;
	 case ARROWDN:
		draw_dn(gc, x, y);
		break;
	 case ARROWL:
		draw_left(gc, x, y);
		break;
	 case ARROWR:
		draw_right(gc, x, y);
		break;
	 case ARROWRDN:
		draw_rdn(gc, x, y);
		break;
	 case ARROWLDN:
		draw_ldn(gc, x, y);
		break;
	 case ARROWLUP:
		draw_lup(gc, x, y);
		break;
	 case ARROWRUP:
		draw_rup(gc, x, y);
		break;
	}
   }

   private void drawWLine(Graphics gc, int w, int x1, int y1, int x2, int y2)
   {
	int     dx, dy;
        int     ax, ay;
        double  angle;

        dx = x2 - x1;
        dy = y1 - y2;
        if ((dx == 0) && (dy == 0))
             return;
        if (dx == 0)
        {
             angle = 1.0;
             ax = w;
             ay = 0;
        }
        else if (dy == 0)
        {
             angle = 0.0;
             ax = 0;
             ay = w;
        }
        else
        {
             if (dx < 0)
                ax = 0 - dx;
             else
                ax = dx;
             if (dy < 0)
                ay = 0 - dy;
             else
                ay = dy;
             angle = 1.0 - (double) ax / (double)(ax + ay);
             ax = (int) (w * angle);
             ay = w - ax;
             if (dx < 0)
                ax = 0 - ax;
             if (dy < 0)
                ay = 0 - ay;
        }
        xpnts[0] = x1 + ax / 2;
        xpnts[1] = x2 + ax / 2;
        xpnts[2] = x2 + ax / 2 - ax;
        xpnts[3] = x1 + ax / 2 - ax;
        ypnts[0] = y1 + ay / 2;
        ypnts[1] = y2 + ay / 2;
        ypnts[2] = y2 + ay / 2 - ay;
        ypnts[3] = y1 + ay / 2 - ay;
        gc.fillPolygon(xpnts, ypnts, 4);
   }

   public void draw2(Graphics gc, int type, int x, int y, int x2, int y2, int lw)
   {
	int	dx, dy;
	double  angle, angle2, len;
	int     ax, ay, aw;

	dx = x2 - x;
	dy = y - y2;
	if ((dx == 0) && (dy == 0))
	     return;
	aw = gwidth + lw;
	angle = 0.0;
	len = Math.sqrt((double)(dx * dx) + (double)(dy * dy));
	if (dx == 0)
	{
	     if (y2 < y)
		angle = 0.5 * pi;
	     else
		angle = -0.5 * pi;
	}
	else if (dy == 0)
	{
	     if (x < x2)
		angle = 0.0;
	     else
		angle = pi;
	}
	else
	{
	     angle = Math.acos((double)dx / len);
	     if (dy < 0)
		angle = -angle;
	}
	angle2 = angle - pi;
	xs[0] = x2;  ys[0] = y2;
	xs[1] = x2 + (int)(Math.cos(angle2 + 0.314) * aw);
	ys[1] = y2 - (int)(Math.sin(angle2 + 0.314) * aw);
	xs[2] = x2 + (int)(Math.cos(angle2 - 0.314) * aw);
	ys[2] = y2 - (int)(Math.sin(angle2 - 0.314) * aw);
	gc.fillPolygon(xs, ys, 3);
	xs[0] = x2 + (int)(Math.cos(angle2) * gwidth);
	ys[0] = y2 - (int)(Math.sin(angle2) * gwidth);
	if (lw > 1)
		drawWLine(gc, lw, x, y, xs[0], ys[0]);
	else
		gc.drawLine(x, y, xs[0], ys[0]);
   }

   private void set_points (int  sx[], int sy[], int dx, int dy)
   {
	xs[0] = sx[0] + dx;  ys[0] = dy + sy[0];
	xs[1] = sx[1] + dx;  ys[1] = dy + sy[1];
	xs[2] = sx[2] + dx;  ys[2] = dy + sy[2];
   }

   public void draw_up(Graphics gc, int x, int y)
   {
	y = y - height;
	set_points (upxs, upys, x, y);
	gc.fillPolygon(xs, ys, 3);
	gc.drawLine(xs[0], ys[1], xs[0], y + height);
   }

   public void draw_dn(Graphics gc, int x, int y)
   {
	y = y - height;
	set_points (dnxs, dnys, x, y);
	gc.fillPolygon(xs, ys, 3);
	gc.drawLine(xs[0], y, xs[0], ys[1]);
   }

   public void draw_right(Graphics gc, int x, int y)
   {
	y = y - height / 2;
	set_points (rxs, rys, x, y);
	gc.fillPolygon(xs, ys, 3);
	gc.drawLine(x, ys[2] ,xs[1] , ys[2]);
   }

   public void draw_left(Graphics gc, int x, int y)
   {
	y = y - height / 2;
	set_points (lxs, lys, x, y);
	gc.fillPolygon(xs, ys, 3);
	gc.drawLine(xs[1], ys[0] , x+width , ys[0]);
   }

   public void draw_rdn(Graphics gc, int x, int y)
   {
	y = y - rh;
	set_points (rdnxs, rdnys, x, y);
	gc.fillPolygon(xs, ys, 3);
	gc.drawLine(x, y, x + rw - 2 , y + rh - 2);
   }

   public void draw_ldn(Graphics gc, int x, int y)
   {
	y = y - rh;
	set_points (ldnxs, ldnys, x, y);
	gc.fillPolygon(xs, ys, 3);
	gc.drawLine(x+2, y + rh-2, x + rw , y);
   }

   public void draw_lup(Graphics gc, int x, int y)
   {
	y = y - rh;
	set_points (lupxs, lupys, x, y);
	gc.fillPolygon(xs, ys, 3);
	gc.drawLine(x+2, y+2, x + rw , y + rh);
   }

   public void draw_rup(Graphics gc, int x, int y)
   {
	y = y - rh;
	set_points (rupxs, rupys, x, y);
	gc.fillPolygon(xs, ys, 3);
	gc.drawLine(x, y + rh, x + rw-2 , y+2);
   }

   public void setSize(int w, int h)
   {
	if (width == w && height == h)
	    return;
	width = w;
	height = h;
	gwidth = (int)(w * 0.8);
	rh = (int) (h * 0.7);
	rw = (int) (w * 0.7);
	w = width / 2;
	upxs[0] = w / 2;  upys[0] = 0;
	upxs[1] = 0;  upys[1] = (int) (h * 0.6);
	upxs[2] = w;  upys[2] = (int) (h * 0.6);
	
	dnxs[0] = w / 2;  dnys[0] = h;
	dnxs[1] = 0;  dnys[1] = (int) (h * 0.4);
	dnxs[2] = w;  dnys[2] = (int) (h * 0.4);

	h = height / 2;
	w = width;
	rxs[0] = (int) (w * 0.4);  rys[0] = 0;
	rxs[1] = (int) (w * 0.4);  rys[1] = h;
	rxs[2] = w;  rys[2] = h / 2;

	lxs[0] = 0;  lys[0] = h / 2;
	lxs[1] = (int) (w * 0.6);  lys[1] = 0;
	lxs[2] = (int) (w * 0.6);  lys[2] = h;

	h = (int) (height * 0.7);
	w = (int) (width * 0.7);
	rdnxs[0] = w;  rdnys[0] = h;
	rdnxs[1] = (int)(w * 0.3);  rdnys[1] = (int) (h * 0.7);
	rdnxs[2] = (int) (w * 0.7);  rdnys[2] = (int) (h * 0.3);

	ldnxs[0] = 0;  ldnys[0] = h;
	ldnxs[1] = (int) (w * 0.7);  ldnys[1] = (int) (h * 0.7);
	ldnxs[2] = (int) (w * 0.3);  ldnys[2] = (int) (h * 0.3);

	lupxs[0] = 0;  lupys[0] = 0;
	lupxs[1] = (int) (w * 0.3);  lupys[1] = (int) (h * 0.7);
	lupxs[2] = (int) (w * 0.7);  lupys[2] = (int) (h * 0.3);

	rupxs[0] = w;  rupys[0] = 0;
	rupxs[1] = (int) (w * 0.3);  rupys[1] = (int) (h * 0.3);
	rupxs[2] = (int) (w * 0.7);  rupys[2] = (int) (h * 0.7);
   }

   private int	width = 0;
   private int	height = 0;
   private int	gwidth = 0;
   private int	rh = 0;
   private int	rw = 0;
   private int[] xs, ys;
   private int[] upxs, upys;
   private int[] dnxs, dnys;
   private int[] rxs, rys;
   private int[] lxs, lys;
   private int[] rupxs, rupys;
   private int[] lupxs, lupys;
   private int[] rdnxs, rdnys;
   private int[] ldnxs, ldnys;
   private int[] xpnts = new int[4];
   private int[] ypnts = new int[4];
   static final double pi = 3.1416;
   static final double degree0 = 2.6182; /* 150 degree */
   static final double deg60 = 1.047;  /* 60 degree */
   static final double deg45 = 0.7854;  /* 45 degree */

}

