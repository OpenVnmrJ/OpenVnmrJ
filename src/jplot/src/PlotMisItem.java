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
import java.io.*; 

public class PlotMisItem implements PlotDefines, PlotObj, EditItemIF
{
   public PlotMisItem(Object p, int item_type, int item_x, 
		int item_y, int item_w, int item_h, int pw, int ph, double r)
   {
	p_tool = (PlotEditTool) p;
	mygc = PlotUtil.getGC();;
	ratio = r;
	p_width = pw;
	p_height = ph;
	org_x = (double) item_x / pw;
	org_y = (double) item_y / ph;
	org_width = (double) item_w / pw;
	org_height = (double) item_h / ph;
	x = item_x;
	y = item_y;
	x2 = item_x;
	y2 = item_y;
	width = item_w;
	height = item_h;
	switch (item_type) {
	  case LINE:
		     x2 = item_w;
		     y2 = item_h;
		     break;
	  case ARROWRR:
		     x2 = item_w;
		     y2 = item_h;
		     item_type = GARROW;
		     break;
	  case ARROWLL:
		     x2 = x;
		     y2 = y;
		     x = item_w;
		     y = item_h;
		     org_x = (double) x / pw;
		     org_y = (double) y / ph;
		     org_width = (double) x2 / pw;
		     org_height = (double) y2 / ph;
		     item_type = GARROW;
		     break;
	  case GARROW:
		     x2 = item_w;
		     y2 = item_h;
		     break;
	}
	type = item_type;
   }

   public PlotMisItem(Object p, int stype, double sx, 
		double sy, double sw, double sh, int pw, int ph, double r)
   {
	p_tool = (PlotEditTool) p;
	mygc = PlotUtil.getGC();;
	ratio = r;
	org_x = sx;
	org_y = sy;
	p_width = pw;
	p_height = ph;
	org_width = sw;
	org_height = sh;
	x = (int) (sx * pw);
	y = (int) (sy * ph);
	x2 = x;
	y2 = y;
	width = (int) (sw * pw);
	height = (int) (sh * ph);
	switch (stype) {
	  case LINE:
		     x2 = width;
		     y2 = height;
		     break;
	  case ARROWRR:
		     x2 = width;
		     y2 = height;
		     stype = GARROW;
		     break;
	  case ARROWLL:
		     x2 = x;
		     y2 = y;
		     x = width;
		     y = height;
		     org_x = (double) x / pw;
		     org_y = (double) y / ph;
		     org_width = (double) x2 / pw;
		     org_height = (double) y2 / ph;
		     stype = GARROW;
		     break;
	  case GARROW:
		     x2 = width;
		     y2 = height;
		     break;
	}
	type = stype;
   }

   private void drawWLine(int x1, int y1, int x2, int y2)
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
             ax = lineRwidth;
             ay = 0;
        }
        else if (dy == 0)
        {
             angle = 0.0;
             ax = 0;
             ay = lineRwidth;
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
             ax = (int) (lineRwidth * angle);
             ay = lineRwidth - ax;
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
        mygc.fillPolygon(xpnts, ypnts, 4);
   }

   private void drawWRect(int x, int y, int w, int h)
   {
        int     n, k, x1, y1, x2, y2;

        k = lineRwidth - lineRwidth / 2;
        x1 = x - k;
        y1 = y - k;
        x2 = x + w - k;
        y2 = y + h - k;
        if (x1 < 0) x1 = 0;
        if (y1 < 0) y1 = 0;
        if ((w < lineRwidth) || (h < lineRwidth))
        {
             if (w < lineRwidth)
                w = lineRwidth;
             if (h < lineRwidth)
                h = lineRwidth;
             mygc.fillRect(x1, y1, w, h);
             return;
        }
        mygc.fillRect(x1, y1, x2 - x1 + lineRwidth, lineRwidth);
        y1 += lineRwidth;
        h = y2 - y1;
        if (h > 0)
        {
           mygc.fillRect(x1, y1, lineRwidth, h);
           mygc.fillRect(x2, y1, lineRwidth, h);
        }
        w = w - lineRwidth;
        mygc.fillRect(x1, y2, x2 - x1 + lineRwidth, lineRwidth);
   }


   public void drawItem(Graphics gc)
   {
/*
	gc.setPaintMode();
*/
	if (is_hilited)
	    gc.setColor(hg);
	else
	    gc.setColor(fg);
	if (type  >= ARROWUP && type <= ARROWLUP)
	{
	    p_tool.drawArrow(gc, type, x, y);
	    return;
	}
	switch (type) {
	 case TEXT:
	    	   if (font == null)
			setFontInfo();
	    	   gc.setFont(font);
	    	   gc.drawString(text, x, y);
		   return;
	 case LINE:
		   if (lineRwidth < 2)
		      gc.drawLine(x, y, x2, y2);
		   else {
		      mygc = gc;
		      drawWLine(x, y, x2, y2);
		   }
		   return;
	 case SQUARE:
		   if (lineRwidth < 2)
		      gc.drawRect(x, y, width, height);
		   else {
		      mygc = gc;
		      drawWRect(x, y, width, height);
		   }
		   return;
	 case GARROW:
		   p_tool.drawArrow2(gc, type, x, y, x2, y2, lineRwidth);
		   return;
	}
   }

   public void drawItem()
   {
	mygc = PlotUtil.getGC();;
	drawItem(mygc);
   }

   public void move(int dx, int dy, int step)
   {
	if (step == 2)
	{
	    x = mx;
	    y = my;
	    x2 = mx2;
	    y2 = my2;
	    org_x = (double) x / p_width;
	    org_y = (double) y / p_height;
	    if ((type == LINE) || (type == GARROW))
	    {
	        org_width = (double) x2 / p_width;
	        org_height = (double) y2 / p_height;
		width = x2;
		height = y2;
	    }
	    drawItem();
	    return;
	}
	if ((step == 0) || (step == 3))
	{
	    mx = x;
	    my = y;
	    mx2 = x2;
	    my2 = y2;
	    if (step == 0)
		return;
	}
	if (type  >= ARROWUP && type <= ARROWLUP)
	    p_tool.drawArrow(type, mx, my);
	else
	{
	    switch (type) {
	     case  TEXT:
	    		mygc.setFont(font);
			mygc.drawString(text, mx, my);
			break;
	     case  LINE:
		   	if (lineRwidth < 2)
			    mygc.drawLine(mx, my, mx2, my2);
		   	else
		      	    drawWLine(mx, my, mx2, my2);
			break;
	     case  SQUARE:
		        if (lineRwidth < 2)
	    		      mygc.drawRect(mx, my, width, height);
		   	else
		             drawWRect(mx, my, width, height);
			break;
	     case  GARROW:
	    		p_tool.drawArrow2(type, mx, my, mx2, my2, lineRwidth);
			break;
	   }
	}
        
	if (step == 0)
	    return;
	mx = x + dx;
	my = y + dy;
	mx2 = x2 + dx;
	my2 = y2 + dy;
	if (mx < 2 )
	{   dx = 2 - x;
	    mx = x + dx;
	    mx2 = x2 + dx;
	}
	if (my < 3 )
	{   dy = 3 - y;
	    my = y + dy;
	    my2 = y2 + dy;
	}
	if (type  >= ARROWUP && type <= ARROWLUP)
	    p_tool.drawArrow(type, mx, my);
	else
	{
	    switch (type) {
	     case  TEXT:
			mygc.drawString(text, mx, my);
			break;
	     case  LINE:
		   	if (lineRwidth < 2)
			    mygc.drawLine(mx, my, mx2, my2);
		   	else
		      	    drawWLine(mx, my, mx2, my2);
			break;
	     case  SQUARE:
		        if (lineRwidth < 2)
	    		      mygc.drawRect(mx, my, width, height);
		   	else
		             drawWRect(mx, my, width, height);
			break;
	     case  GARROW:
	    		p_tool.drawArrow2(type, mx, my, mx2, my2, lineRwidth);
			break;
	   }
	}
	if (step == 3)
	{
	   x = mx;
	   y = my;
	   x2 = mx2;
	   y2 = my2;
	}
   }

   public void move(Graphics g, int dx, int dy, int step) {
	mygc = g;
	move(dx, dy, step);
   }

   public void highlight (boolean hilit)
   {
	is_hilited = hilit;
	PlotUtil.restoreArea(getDim());
	mygc = PlotUtil.getGC();
	if (hilit) {
	    mygc.setXORMode(PlotUtil.getWinBackground());
	}
	else {
	    mygc.setPaintMode();
	}
	drawItem(mygc);
	mygc.dispose();
   }

   public Rectangle getDim()
   {
	int  nx, ny, nw, nh;

	if ((type == LINE) || (type == GARROW))
	{
	    if (x2 < x) {
		nx = x2;
		nw = x - x2 + 2;
	    }
	    else {
		nx = x;
		nw = x2 - x + 2;
	    }
	    if (y2 < y) {
		ny = y2;
		nh = y - y2 + 2;
	    }
	    else {
		ny = y;
		nh = y2 - y + 2;
	    }
	    return new Rectangle(nx - line_width, ny - line_width,
		 nw + line_width * 2, nh + line_width * 2);
	}
	if (type == TEXT)
	{
	    return new Rectangle(x, y - height, width+1 , height);
	}
	else if (type  >= ARROWUP && type <= ARROWLUP)
	{
	    return new Rectangle(x, y - height, width+1 , height +1);
	}
	else
	{
	    return new Rectangle(x - line_width, y - line_width,
		 width + line_width * 2+1, height + line_width * 2+1);
	}
   }

   public Rectangle getVector()
   {
	if ((type == LINE) || (type == GARROW))
	{
	    return new Rectangle(x, y, x2, y2);
	}
	return getDim();
   }

   public Rectangle getRegion()
   {
	int  nx, ny, nx2, ny2, rw;
	int  w;

	if (lineRwidth > 1)
	    w = lineRwidth / 2 + 1;
	else
	    w = 0;
	if ((type == LINE) || (type == GARROW))
	{
	    rw = 0;
	    if (type == GARROW)
	    {
		if (p_width > p_height)
		    rw = (int) (p_width * 0.02 / 2.0) + w;
		else
		    rw = (int) (p_height * 0.02 / 2.0) + w;
	    }
	    if (x2 > x)
	    {
		nx = x;
		nx2 = x2;
	    }
	    else
	    {
		nx = x2;
		nx2 = x;
	    }
	    if (y < y2)
	    {
		ny = y;
		ny2 = y2;
	    }
	    else
	    {
		ny = y2;
		ny2 = y;
	    }
	    nx = nx - rw;
	    ny = ny - rw;
	    nx2 = nx2 + rw;
	    ny2 = ny2 + rw;
	    if (nx < 0)
		nx = 0;
	    if (ny < 0)
		ny = 0;
	    return new Rectangle(nx, ny, nx2, ny2);
	}
	if (type == TEXT)
	{
	    ny =  y - height;
	    ny2 =  ny +text_height + 1;
	    return new Rectangle(x, ny, x+width+1 , ny2);
	}
	else if (type  >= ARROWUP && type <= ARROWLUP)
	{
	    ny =  y - height;
	    return new Rectangle(x, ny, x+width+1 , ny+height +1);
	}
	else
	{
	    nx = x - w;
	    if (nx < 0)
		nx = 0;
	    ny = y - w;
	    if (ny < 0)
		ny = 0;
	    return new Rectangle(nx, ny, x+width+1+w, y+height+1+w);
	}
   }


   public int getType()
   {
        return type;
   }

   public void delete ()
   {
	try {
	   super.finalize();
	}
	catch (Throwable  e)
	{ }
   }

   public void setFontInfo ()
   {
	font = new Font(font_name, font_style, (int)(font_size * ratio));
	mygc.setFont(font);
	fontMetric = mygc.getFontMetrics();
	width = fontMetric.stringWidth(text);
	height = fontMetric.getAscent();
	text_height = height + fontMetric.getDescent();
   }

   public void setItemFont (String f, int t, int s)
   {
	font_name = f;
	font_style = t;
	font_size = s;
	setFontInfo();
   }

   public void setText (String t)
   {
	text = t;
   }

   public void setColors (Color c, Color d, Color e)
   {
	fg = c;
	bg = d; 
	hg = e;
   }

   public void setHilitColor (Color c)
   {
        hg = c;
   }

   public void setFgColor (Color c)
   {
        fg = c;
   }

   public void setLineWidth (int w)
   {
        if (w > 0)
            line_width = w;
        else
            line_width = 1;
	lineRwidth = (int) (line_width * 72.0 * 0.6 / (double)scrn_res);
   }

   public int getLineWidth ()
   {
	return lineRwidth;
   }

   public void setScrnRes (int w)
   {
	scrn_res = w;
   }

   public void setPreference()
   {
	if (prefWin == null)
            return;
        fg = prefWin.getItemColor();
        line_width = prefWin.getItemLineWidth();
        font_name = prefWin.getFontName();
        font_style = prefWin.getFontStyle();
        font_size = prefWin.getFontSize();
	lineRwidth = (int) (line_width * 72.0 * 0.6 / (double)scrn_res);
	if (type == TEXT)
	    setFontInfo();
   }

   public void showPreference()
   {
        if (prefWin == null)
            return;
        prefWin.setItemLineWidth (line_width);
        prefWin.setItemColor (fg);
	if (type == TEXT)
            prefWin.setFontChoice (font_name, font_style, font_size);
   }

   public void setPrefWindow(PlotItemPref  p)
   {
        prefWin = p;
	setPreference();
   }

   public void setTextInfo(TextInput  p) {
	if (type != TEXT)
	     return;
	p.setData(text);
	p.setFontFamily(font_name);
	p.setFontStyle(font_style);
	p.setFontSize(font_size);
	p.updateFont();
   }
	

   public void setRatio (double r, int pw, int ph)
   {
	if ((r == ratio) && (pw == p_width) && (ph == p_height))
	    return;
	ratio = r;
	p_width = pw;
	p_height = ph;
	x = (int) (org_x * pw);
	y = (int) (org_y * ph);
	if (type == TEXT)
	{
	    setFontInfo();
	    return;
	}
	else
	{
	    width = (int) (org_width * pw);
	    height = (int) (org_height * ph);
	    if ((type == LINE) || (type == GARROW))
	    {
	    	x2 = width;
	    	y2 = height;
	    }
	}
   }

   public void writeToFile(PrintWriter os)
   {
	int	k;

	os.print(""+vcommand+" '-draw', 'color', ");
	k = fg.getRed();
	Format.print(os, "%.3f, ", (double) k);
	k = fg.getGreen();
	Format.print(os, "%.3f, ", (double) k);
	k = fg.getBlue();
	Format.print(os, "%.3f", (double) k);
	os.println(" )");
	os.print(""+vcommand+" '-draw', 'linewidth', ");
	Format.print(os, "%d", line_width);
	os.println(" )");

	if (type == TEXT)
	{
	    os.print(""+vcommand+" '-draw', 'font', ");
	    if (font_name.equals("Serif"))
	       os.print("'Times");
	    else
	    {
	       if (font_name.equals("SansSerif"))
	          os.print("'Helvetica");
	       else if (font_name.equals("Monospaced"))
	          os.print("'Courier");
	    }
	    if(font_style == Font.BOLD)
	       os.print("-Bold");
	    else if(font_style == Font.ITALIC)
	       os.print("-Italic");
	    Format.print(os, "', %d, ", font_size);
	    os.print("'"+font_name+"', ");
	    Format.print(os, "%d ", font_style);
	    os.println(" )");
	    os.print(""+vcommand+" '-draw', 'text', ");
	    Format.print(os, "%f, ", org_x );
	    Format.print(os, "%f, ", org_y);
	    os.print("'");
	    for (k = 0; k < text.length(); k++)
	    {
		if (text.charAt(k) == '\\')
		    os.print("\\\\\\");
		else if (text.charAt(k) == '\'')
		    os.print("\\");
	    	Format.print(os, "%c", text.charAt(k));
	    }
	    os.println("')");
	    return;
	}
	if (type == LINE)
	{
	    os.print(""+vcommand+" '-draw', 'line', ");
	    Format.print(os, "%f, ", org_x);
	    Format.print(os, "%f, ", org_y);
	    Format.print(os, "%f, ", org_width);
	    Format.print(os, "%f ", org_height);
	    os.println(")");
	    return;
	}
	if (type == GARROW)
	{
	    os.print(""+vcommand+" '-draw', 'garrow', ");
	    Format.print(os, "%f, ", org_x);
	    Format.print(os, "%f, ", org_y);
	    Format.print(os, "%f, ", org_width);
	    Format.print(os, "%f ", org_height);
	    os.println(")");
	    return;
	}
	if (type == SQUARE)
	{
	    os.print(""+vcommand+" '-draw', 'square', ");
	    Format.print(os, "%f, ", org_x);
	    Format.print(os, "%f, ", org_y);
	    Format.print(os, "%f, ", org_width);
	    Format.print(os, "%f ", org_height);
	    os.println(")");
	    return;
	}
	if (type >= ARROWUP && type <= ARROWLUP)
	{
	    os.print(""+vcommand+" '-draw', 'arrow', ");
	    Format.print(os, "%d, ", type - ARROWUP + 1);
	    Format.print(os, "%f, ", org_x);
	    Format.print(os, "%f, ", org_y);
	    Format.print(os, "%f, ", org_width);
	    Format.print(os, "%f ", org_height);
	    os.println(")");
	    return;
	}
   }


   private int       x, y, width, height, text_height;
   private int       p_width, p_height;
   private double    org_x, org_y, org_width, org_height;
   private int       x2, y2;
   private int       mx, my, mx2, my2;
   private int	     type = 0;
   private boolean   is_hilited = false;
   private Font	     font = null;
   private FontMetrics fontMetric;
   private PlotEditTool p_tool;
   private String    font_name;
   private String    text;
   private int	     font_style, font_size;
   private int	     line_width = 1;
   private int	     lineRwidth = 1;
   private int	     scrn_res = 90;
   private int[]   xpnts = new int[4];
   private int[]   ypnts = new int[4];
   private double    ratio = 1.0;
   private Graphics  mygc;
   private Color     fg, bg, hg;
   private PlotItemPref  prefWin = null;
}
