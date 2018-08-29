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
import java.io.*;
import java.util.*;
import java.lang.*;
import java.awt.image.*;
// import PlotConfig.*;

public class DrawRegion  implements PlotDefines, PlotObj
{
   public DrawRegion(PlotConfig.PlPanel parent, int num, int x, int y, int w, int h)
   {  
	if (parent == null)
            return;
	id = num;
	p_object = parent;
	mgc = PlotUtil.getGC();
	orgx = x;
	orgy = y;
	width = w;
	height = h;
	ratio = 1.0;
	setDimRatio();
   }

   public DrawRegion(PlotConfig.PlPanel parent, int x, int y, int w, int h, double r)
   {  
	if (parent == null)
            return;
	p_object = parent;
	mgc = PlotUtil.getGC();
	orgx = x;
	orgy = y;
	width = w;
	height = h;
	ratio = r;
	setDimRatio();
   }

   public void drawItem(Graphics g) {
   }

   public void drawItem() {
   }


   private void setDimRatio()
   {
	double	pw, ph;

	p_width = PlotUtil.getWinWidth();
	p_height = PlotUtil.getWinHeight();
	rx = ((double) orgx) / p_width;
	ry = ((double) orgy) / p_height;
	rw = ((double) width) / p_width;
	rh = ((double) height) / p_height;
   }

   public void setColors (Color c, Color d, Color e)
   {
        fg = c;
        bg = d;
        hg = e;
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
   }

   public void setBorderColor (Color c)
   {
        bg = c;
	if (!is_selected)
	{
	    mgc.setPaintMode();
	    mgc.setColor(bg);
	    if (borderFlag > 0)
	        mgc.drawRect(orgx, orgy, width, height);
	}
   }

   public void setBorder (int v)
   {
	borderFlag = v;
   }

   public void setFontName (String s)
   {
	if (s != null)
	   font_name = s;
   }

   public void setFontStyle (int s)
   {
	font_style = s;
   }

   public void setFontSize (int s)
   {
	font_size = s;
   }

   public void setHilitColor (Color c)
   {
        hg = c;
	if (is_selected)
	{
	    mgc.setPaintMode();
	    mgc.setColor(hg);
	    mgc.drawRect(orgx, orgy, width, height);
	}
   }

   public void changeParentSize(int  pw, int ph)
   {  
	p_width = pw;
	p_height = ph;
	orgx = (int) ((double) pw * rx);
	orgy = (int) ((double) ph * ry);
	width = (int) ((double) pw * rw);
	height = (int) ((double) ph * rh);
   }

   public void changeSize(Rectangle dm)
   {  
	orgx = dm.x;
	orgy = dm.y;
	width = dm.width;
	height = dm.height;
	setDimRatio();
   }

   public void setLocation(int x, int y)
   {  
	orgx = x;
	orgy = y;
	rx = ((double) orgx) / p_width;
	ry = ((double) orgy) / p_height;
   }

   public void setRatio(double r)
   {  
	ratio = r;
   }

   public void regionDraw(int x, int y, int w, int h) {
	mgc.setClip(x, y, w, h);
	regionDraw(mgc);
   }

   public void regionPaint(Graphics gc)
   {
	int	x2, y2, x3;

	if (gc == null)
	    return;
	if (width <= 6 || height <= 6)
	    return;
        gc.setPaintMode();

	if (image_data != null)
	    gc.drawImage(image_data, orgx, orgy, width, height, null);
	if (is_selected) {
            gc.setColor(hg);
	}
	else {
            gc.setColor(bg);
	}
	if (borderFlag > 0) {
            gc.drawRect(orgx, orgy, width, height);
	}
	if (!is_selected)
	    return;

	if (is_moving)
	    return;
	x2 = orgx + width;
	y2 = orgy + height;
	if (width > 6)
	   mark_w = 6;
	else
	   mark_w = width;
	if (height > 6)
	   mark_h = 6;
	else
	   mark_h = height;
	x2 = x2 - mark_w;
	y2 = y2 - mark_h;
	gc.fillRect(orgx, orgy, mark_w, mark_h);
	gc.fillRect(orgx, y2, mark_w, mark_h);
	gc.fillRect(x2, orgy, mark_w, mark_h);
	gc.fillRect(x2, y2, mark_w, mark_h);
	if (width > mark_w * 3 + 4)
	{
	    x3 = orgx + (width - mark_w) / 2;
	    gc.fillRect(x3, orgy, mark_w, mark_h);
	    gc.fillRect(x3, y2, mark_w, mark_h);
	}
	if (height > mark_h * 3 + 4)
	{
	    y2 = orgy + (height - mark_h) / 2;
	    gc.fillRect(orgx, y2, mark_w, mark_h);
	    gc.fillRect(x2, y2, mark_w, mark_h);
	}
   }



   public void regionDraw(Graphics gc)
   {
	int	x2, y2, x3;

	if (gc == null)
	    return;
	if (width <= 6 || height <= 6)
	    return;
        gc.setPaintMode();
        mgc.setPaintMode();

	if (image_data != null)
	    gc.drawImage(image_data, orgx, orgy, width, height, null);
	if (is_selected) {
            gc.setColor(hg);
            mgc.setColor(hg);
	}
	else {
            gc.setColor(bg);
            mgc.setColor(bg);
	}
	if (borderFlag > 0) {
            gc.drawRect(orgx, orgy, width, height);
            mgc.drawRect(orgx, orgy, width, height);
	}
	if (!is_selected)
	    return;

	if (is_moving)
	    return;
	x2 = orgx + width;
	y2 = orgy + height;
	if (width > 6)
	   mark_w = 6;
	else
	   mark_w = width;
	if (height > 6)
	   mark_h = 6;
	else
	   mark_h = height;
	x2 = x2 - mark_w;
	y2 = y2 - mark_h;
	gc.fillRect(orgx, orgy, mark_w, mark_h);
	mgc.fillRect(orgx, orgy, mark_w, mark_h);
	gc.fillRect(orgx, y2, mark_w, mark_h);
	mgc.fillRect(orgx, y2, mark_w, mark_h);
	gc.fillRect(x2, orgy, mark_w, mark_h);
	mgc.fillRect(x2, orgy, mark_w, mark_h);
	gc.fillRect(x2, y2, mark_w, mark_h);
	mgc.fillRect(x2, y2, mark_w, mark_h);
	if (width > mark_w * 3 + 4)
	{
	    x3 = orgx + (width - mark_w) / 2;
	    gc.fillRect(x3, orgy, mark_w, mark_h);
	    gc.fillRect(x3, y2, mark_w, mark_h);
	    mgc.fillRect(x3, orgy, mark_w, mark_h);
	    mgc.fillRect(x3, y2, mark_w, mark_h);
	}
	if (height > mark_h * 3 + 4)
	{
	    y2 = orgy + (height - mark_h) / 2;
	    gc.fillRect(orgx, y2, mark_w, mark_h);
	    gc.fillRect(x2, y2, mark_w, mark_h);
	    mgc.fillRect(orgx, y2, mark_w, mark_h);
	    mgc.fillRect(x2, y2, mark_w, mark_h);
	}
   }

   public void regionDraw()
   {
	mgc = PlotUtil.getGC();
	regionDraw(mgc);
   }

   public void print(Graphics  g)
   {
	if (width <= 6 || height <= 6)
	    return;
	if (image_data != null)
	{
	    g.drawImage(image_data, orgx, orgy, width, height, null);
	}
   }

   public void xorOutLine()
   {
	if (xorW < 1 || xorH < 1)
	   return;
        mgc.setXORMode(PlotUtil.getWinBackground());
        mgc.setColor(hg);
	if (borderFlag > 0)
            mgc.drawRect(xorX, xorY, xorW, xorH);
   }

   public void xorOutLine(int x, int y)
   {
	int	x2, y2;

	if (x < 1)
	   x = 1;
	if (x >= p_width)
	   x = p_width - 1;
	if (y < 1)
	   y = 1;
	if (y >= p_height)
	   y = p_height - 1;
        mgc.setXORMode(PlotUtil.getWinBackground());
        mgc.setColor(hg);
	if (xorW > 0 && xorH > 0)
           mgc.drawRect(xorX, xorY, xorW, xorH);
	else
	{
           mgc.setPaintMode();
           mgc.setColor(bg);
           mgc.drawRect(orgx, orgy, width, height);
	   xorW = width;
	   xorH = height;
           mgc.setXORMode(PlotUtil.getWinBackground());
           mgc.setColor(hg);
	}
	x2 = xorX + xorW;
	y2 = xorY + xorH;
	switch (cursor_mode) {
	   case  1:  // W_RESIZE
		   if (x < x2)
		       xorX = x;
		   break;
	   case  2:  // NW_RESIZE
		   if (x < x2 && y < y2)
		   {
		        xorX = x;
			xorY = y;
		   }
		   break;
	   case  3:  // SW_RESIZE
		   if (x < x2 && y > xorY)
		   {
			xorX = x;
		        y2 = y;
		   }
		   break;
	   case  4:  // E_RESIZE
		   if (x > xorX)
		       x2 = x;
		   break;
	   case  5:  // NE_RESIZE
		   if (x > xorX && y < y2)
		   {
		       x2 = x;
		       xorY = y;
		   }
		   break;
	   case  6:  // SE_RESIZE
		   if (x > xorX && y > xorY)
		   {
		       x2 = x;
		       y2 = y;
		   }
		   break;
	   case  7:  // N_RESIZE
		   if (y < y2)
		       xorY = y;
		   break;
	   case  8:  // S_RESIZE
		   if (y > xorY)
		       y2 = y;
		   break;
	}
	xorH = y2 - xorY;
	xorW = x2 - xorX;
	if (xorW > 0 && xorH > 0)
           mgc.drawRect(xorX, xorY, xorW, xorH);
   }

   public void outLine ()
   {
	if (borderFlag > 0)
	{
            mgc.setPaintMode();
	    if (is_selected)
		mgc.setColor(hg);
	    else
		mgc.setColor(bg);
            mgc.drawRect(orgx, orgy, width, height);
	}
   }

   public void changeShape(int  x, int y)
   {

	if (x > orgx)
	    width = x - orgx;
	else
	{
	    width = orgx - x;
	    orgx = x;
	}
	if (y > orgy)
	    height = y - orgy;
	else
	{
	    height = orgy - y;
	    orgy = y;
	}
   }

   public Rectangle getDim()
   {
	return new Rectangle(orgx, orgy, width, height);
   }

   public Rectangle getRegion()
   {
	return new Rectangle(orgx, orgy, orgx + width, orgy + height);
   }

   public Rectangle getResizeDim()
   {
	return new Rectangle(xorX, xorY, xorW, xorH);
   }

   public int getId()
   {
	return id;
   }

   public void add(int x, int y)
   {
   }

   public void clear()
   {
	mgc = PlotUtil.getGC();
	Graphics2D bg2 = (Graphics2D) mgc;
	bg2.setBackground(PlotUtil.getWinBackground());
	mgc.clearRect(orgx, orgy, width + 1, height + 1);
   }

   public void remove(int n)
   {
   }

   public void move(int x, int y)
   {
	if (x < 0)
	    x = 0;
	if (y < 0)
	    y = 0;
	orgx = x;
	orgy = y;
	regionDraw();
   }

   public void move(int x, int y, int step) {
   }

   public void move(Graphics g, int x, int y, int step) {
   }

   public void highlight (boolean hilit) {
   } 

   public void delete ()
   {
	image_data = null;
        try {
           super.finalize();
        }
        catch (Throwable  e)
        { }
   }

   public void setMoveMode(boolean yesNo)
   {
	is_moving = yesNo;
   }

   public void select(boolean yes)
   {
	if (!yes)
	{
	   if (is_selected)
	   	p_object.saveEditData(this);
	   action_flag = 0;
	}
	is_selected = yes;
/*
	regionDraw();
*/
	if (yes)
	   p_object.setEditData(this);
   }
      
   public void resizeRegion (Rectangle rec)
   {
	action_flag = 0;
	if (cursor_mode == 0)
	   return;
	mgc.setPaintMode();
	clear();
	orgx = rec.x;
	orgy = rec.y;
	width = rec.width;
	height = rec.height;
	clear();
	setDimRatio();
	p_object.paint();
   }

   public void mEvent(MouseEvent evt, int action)
   {
	int	x = evt.getX();
	int	y = evt.getY();
	int	new_cursor_mode;
	int	x2, y2;

	switch (action) {
	  case PRESS:
		action_flag = action;
		xorW = 0;
		break;
	  case RELEASE:
		action_flag = 0;
		if (cursor_mode != 0)
		{
        	    mgc.setPaintMode();
		    if (xorW >= MINW && xorH >= MINW)
		    {
			clear();
			orgx = xorX;
			orgy = xorY;
			width = xorW;
			height = xorH;	
			setDimRatio();
        	        p_object.paint();
		    }
		    else
		    {
            		mgc.setColor(hg);
        		mgc.drawRect(orgx, orgy, width, height);
		    }
		    cursor_mode = 0;
		    p_object.changeResizeCursor(cursor_mode);
		}
		break;
	  case HMOVE:
		x2 = orgx + width - 1;
		y2 = orgy + height - 1;
		new_cursor_mode = 0;
		if (x >= orgx && x <= x2 && y >= orgy && y <= y2)
		{
		    if (x >= orgx && x <= orgx +2)
		    {
			new_cursor_mode = 1;
			if (y < orgy + mark_h)
			    new_cursor_mode = 2;
			else if (y > y2 - mark_h)
			    new_cursor_mode = 3;
		    }
		    else if (x >= x2 - 2)
		    {
			new_cursor_mode = 4;
			if (y < orgy + mark_h)
			    new_cursor_mode = 5;
			else if (y > y2 - mark_h)
			    new_cursor_mode = 6;
		    }
		    else if (y >= orgy && y <= orgy + 2)
		    {
			new_cursor_mode = 7;
			if (x < orgx + mark_w)
			    new_cursor_mode = 2;
			else if (x > x2 - mark_w)
			    new_cursor_mode = 5;
		    }
		    else if (y >= y2 - mark_h)
		    {
			new_cursor_mode = 8;
			if (x < orgx + mark_w)
			    new_cursor_mode = 3;
			else if (x > x2 - mark_w)
			    new_cursor_mode = 6;
		    }
		}
		if (new_cursor_mode != cursor_mode)
		{
		    cursor_mode = new_cursor_mode; 
		    p_object.changeResizeCursor(new_cursor_mode);
		}
		if (cursor_mode != 0)
		{
		    xorX = orgx; xorY = orgy;
		    xorW = 0; xorH = 0;
		}
		break;
	  case DRAG:
		if (cursor_mode != 0)
		    xorOutLine(x, y);
		    
		break;
	  case EXIT:
		action_flag = 0;
		break;
	}
   }


   public void viewImage(String mf)
   {
	image_data = null;
	if (tracker == null)
	    tracker = new MediaTracker(p_object);
	image_data = Toolkit.getDefaultToolkit().getImage(mf);
	if (image_data == null)
	    return;
	tracker.addImage(image_data, 0);
        try { tracker.waitForID(0); }
        catch (InterruptedException e) { return; }
	clear();
	regionDraw();
   }

   public void viewImage(Image img) {
	image_data = img;
	clear();
	regionDraw();
   }

   public void appendMacro(String  s)
   {
        if (s.length() <= 0)
	     return;
	if (org_macro == null)
             org_macro = s;
	else
             org_macro += s;
	org_macro += "\n";
   }

   public void inputTemplate(BufferedReader is)
   {
	String str;
	String tok;
        int     n;
	StringTokenizer t;

	try
	{
	    while ((str = is.readLine()) != null)
	    {
           	if (str.length() <= 0)
           	    return;
	   	t = new StringTokenizer(str, " \n");
	   	tok = t.nextToken();
	   	if (tok.length() > 2)
	   	{
	           if (tok.equals(vcommand))
		      return;
	   	}
		if (org_macro == null)
		    org_macro = str;
		else
	   	    org_macro += str;
		org_macro += "\n";
	    }
	}
	catch(IOException e) { }
   }

   public void outputTemplate(PrintWriter os, boolean preview)
   {
	double	pw, ph;
	int	k;

	pw = (double) PlotUtil.getWinWidth();
	ph = (double) PlotUtil.getWinHeight();

	if ((pw < 5) || (ph < 5))
	    return;
	os.print(""+vcommand+" ");
	os.print("'-region', 'start', ");
	/* region x, y, w, h */
	Format.print(os, "%f, ", (double)orgx / pw);
	Format.print(os, "%f, ", (double)orgy / ph);
	if (preview)
	{
	   Format.print(os, "%f, ", ((double)width / pw) * ratio);
	   Format.print(os, "%f ", ((double)height / ph) * ratio);
	}
	else
	{
	Format.print(os, "%f, ", (double)width / pw);
	Format.print(os, "%f ", (double)height / ph);
	}
	os.println(" )");

	os.print(""+vcommand+" '-region', 'color', ");
        k = fg.getRed();
        Format.print(os, "%.2f, ", (double) k);
        k = fg.getGreen();
        Format.print(os, "%.2f, ", (double) k);
        k = fg.getBlue();
        Format.print(os, "%.2f", (double) k);
        os.println(" )");
	os.print(""+vcommand+" '-region', 'linewidth', ");
	if (preview)
            Format.print(os, "%d ", (int) ((double)line_width * ratio));
	else
            Format.print(os, "%d ", line_width);
        os.println(" )");
	os.print(""+vcommand+" '-region', 'font', ");
	if (font_name.equals("Serif"))
            os.print("'Times");
        else
        {
            if (font_name.equals("SansSerif"))
               os.print("'Helvetica");
            else
               os.print("'Courier");
        }
	if (font_style == Font.BOLD)
            os.print("-Bold");
        else if (font_style == Font.ITALIC)
            os.print("-Italic");
        os.print("', ");
	if (preview)
	    Format.print(os, "%d", (int) ((double)font_size * ratio));
	else
	    Format.print(os, "%d", font_size);
        os.print(", '"+font_name+"', ");
        Format.print(os, "%d ", font_style);
        os.println(" )");

	if (preview)
	{
	   os.print(""+vcommand+" '-region', 'window', ");
	   Format.print(os, "%d, ", width);
	   Format.print(os, "%d ", height);
	   os.println(" )");
	}
	if (new_macro != null)
	   os.print(new_macro);
	else if (org_macro != null)
	   os.print(org_macro);
	os.println();
	if (preview)
           os.print(""+vcommand+" '-region', 'pend', ");
	else
           os.print(""+vcommand+" '-region', 'end', ");
	Format.print(os, "%d ", id);
        os.println(" )");
   }

   public String getData(int  new_data)
   {
	if (new_data == 1)
	{
	    if (new_macro != null)
	       return (new_macro);
	}
	return (org_macro);
   }

   public void setData(String data)
   {
	new_macro = data;
   }

   public void dispVnmrImage(String  outName, String inName)
   {
	PrintWriter outStream;
	String	    vimage;
	try
        {
	   outStream = new PrintWriter(new FileWriter(outName));
           if (outStream == null)
                return;
           outputTemplate(outStream, true);
           outStream.close();
	}
	catch(IOException er) { }
   }

   public void showPreference()
   {
	if (prefWin == null)
	    return;
	prefWin.setItemLineWidth (line_width);
	prefWin.setItemColor (fg);
	prefWin.setFontChoice(font_name, font_style, font_size);
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
   }

   public void setPrefWindow(PlotItemPref  p)
   {
	prefWin = p;
	setPreference();
   }

   public void setAttr(String attr)
   {
	StringTokenizer tok;
	String    name;
	int	  v1, v2, v3;

	tok = new StringTokenizer(attr, " ,\n\t");
	tok.nextToken(); /* skip 'jplot' */
	tok.nextToken(); /* skip 'region' */
	name = tok.nextToken();
	if (name.equals("'color'"))
	{
	    v1 = Format.atoi(tok.nextToken());
            v2 = Format.atoi(tok.nextToken());
            v3 = Format.atoi(tok.nextToken());
            fg = new Color(v1, v2, v3);
	}
	else if (name.equals("'linewidth'"))
	{
	    line_width = Format.atoi(tok.nextToken());
	}
	else if (name.equals("'font'"))
	{
	    name = tok.nextToken();
	    font_size = Format.atoi(tok.nextToken());
	    name = tok.nextToken();
	    v1 = name.length();
	    font_name = name.substring(1, v1 - 1);
	    font_style = Format.atoi(tok.nextToken());
	}
   }

   private int id = 0;
   private int orgx = 0;
   private int orgy = 0;
   private int width = 0;
   private int height = 0;
   private int image_w = 0;
   private int image_h = 0;
   private int line_width = 1;
   private int xorX, xorY, xorW, xorH;
   private int mark_w, mark_h;
   private int action_flag;
   private int cursor_mode = 0;
   private int p_width, p_height;
   private String org_macro;
   private String new_macro = null;
   private String font_name = null;
   private int font_style;
   private int font_size;
   private int borderFlag;
   private Image image_data = null;
   private MediaTracker tracker = null;
   private PlotItemPref prefWin = null;

   private double ratio = 1.0;
   private double rx, ry, rw, rh;
   private Color  fg = Color.black;
   private Color  bg = Color.gray;
   private Color  hg = Color.red;
   private boolean is_selected = false;
   private boolean is_moving = false;
   private PlotConfig.PlPanel p_object;
   private static Graphics mgc = null;
   static final int MINW = 6;
}

