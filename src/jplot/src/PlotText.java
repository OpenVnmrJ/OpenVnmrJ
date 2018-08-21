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
import java.io.*;
import javax.swing.*;
import java.awt.image.BufferedImage;


public class PlotText extends JComponent implements PlotDefines, EditItemIF {
	private String value = null;
	private Color  fgColor;
	private Color  hgColor;
	private String fontName = null;
        private int fontStyle = Font.PLAIN;
	private int fontSize = 12;
	private Graphics gx = null;
	private MouseAdapter ml;
	private int    org_w, org_h;
	private int    panel_w, panel_h;
	private int    x, y, w, h;
	private int    mx, my;
	private double ratio;
	private double rx, ry, rw, rh;
	private boolean hilited = false;
	private Image  img;

	public PlotText(String v, int x, int y, double r) {
	   this.value = v;
	   this.x = x;
	   this.y = y;
	   this.panel_w = PlotUtil.getWinWidth();
	   this.panel_h = PlotUtil.getWinHeight();
	   this.ratio = r;
	   gx = PlotUtil.getGC();
	}


	public String getValue() {
	   return value;
	}

	public void setImage(Image m) {
	   org_w = m.getWidth(null);
	   org_h = m.getHeight(null);
	   w = (int) ((double) org_w * ratio);
	   h = (int) ((double) org_h * ratio);
	   img = m;
        
	}

	public void setItemFont(String n, int s, int z) {
	   fontName = n;
	   fontStyle = s;
	   fontSize = z;
	}

	public void showPreference()
	{
	    PlotItemPref prefWin = PlotUtil.getPrefWindow();
            if (prefWin == null)
		return;
	    prefWin.setItemLineWidth (1);
            prefWin.setItemColor (fgColor);
            prefWin.setFontChoice (fontName, fontStyle, fontSize);
	}

	public void drawItem(Graphics gr, int sx, int sy) {
	    if (img == null)
		return;
	    gr.setPaintMode();
	    gr.drawImage(img, sx, sy - h, w, h, null);
	    if (hilited) {
	        gr.setColor(Color.red);
		int x2 = sx + w - 1;
		int y2 = sy - h;
	        gr.drawLine(sx, y2, x2, y2);
	        gr.drawLine(sx, sy, sx, y2);
	        gr.drawLine(sx, sy, x2, sy);
	        gr.drawLine(x2, sy, x2, y2);
	    }
        }

	public void drawItem(Graphics gr) {
	    drawItem(gr, x, y);
        }

	public void drawItem() {
	    if (img == null)
		return;
	    gx = PlotUtil.getGC();
	    drawItem(gx, x, y);
	}

	public void move(int dx, int dy, int step) {
	    if (step == 0) { // prepare to move
		mx = x;
		my = y;
		return;
	    }
	    if (step == 1 || step == 3) { // moving
		int y2 = my - h - 1;
		if (y2 < 0)
		  y2 = 0;
		PlotUtil.restoreArea(new Rectangle(mx, y2, w + 2, h + 2));
		mx = x + dx;
		if (mx < 0)
		   mx = 0;
		my = y + dy;
		if (my < 0)
		   my = 0;
	        drawItem(gx, mx, my);
	        if (step == 3) { // move by arrow key
		   x = mx;
		   y = my;
		}
		return;
	    }
	    if (step == 2) { // stop
		x = mx;
		y = my;
		return;
	    }
	    else {
	    	x = x + dx;
	    	y = y + dx;
	    }
	}

	public void move(Graphics g, int dx, int dy, int step) {
	    gx = g;
	    move (dx, dy, step);
	}

	public void highlight (boolean hi, Graphics g) {
	    hilited = hi;
	    int y2 = y - h - 1;
	    if (y2 < 0)
		  y2 = 0;
	    PlotUtil.restoreArea(new Rectangle(x, y2, w + 2, h + 2));
	    drawItem(g);
	}

	public void highlight (boolean hi) {
	    highlight (hi, gx);
	}

	public Rectangle getDim() {
	    return new Rectangle(x, y-h, w, h);
	}


	public Rectangle getRegion() {
	    return new Rectangle(x, y-h, x+w, y);
	}

	public Rectangle getVector() {
	    return new Rectangle(x, y-h, x+w, y);
	}

	public int getType() {
	    return TEXT;
	}

	public void setRatio (double r, int pw, int ph) {
	    rx = (double) x / (double) panel_w;
	    ry = (double) y / (double) panel_h;
	    ratio = r;
	    x = (int) (pw * rx);
	    y = (int) (ph * ry);
	    w = (int) ((double) org_w * r);
	    h = (int) ((double) org_h * r);
	    panel_w = pw;
	    panel_h = ph;
	}

	public void setScrnRes (int r) {
	}

	public void setFgColor (Color f) {
	    fgColor = f;
	}

	public void setColors (Color fg, Color bg, Color hg) {
	    fgColor = fg;
	    hgColor = hg;
	}

	public void setTextInfo(TextInput t) {
	    TextInput tx = PlotUtil.getTextWindow();
	    if (tx == null)
		return;
	    tx.setData(value);
            tx.setFontFamily(fontName);
            tx.setFontStyle(fontStyle);
            tx.setFontSize(fontSize);
            tx.setTextColor(fgColor);
            tx.updateFont();
	}

	public void setPreference() {
	    PlotItemPref prefWin = PlotUtil.getPrefWindow();
            if (prefWin == null)
		return;
	    fgColor = prefWin.getItemColor();
	    fontName = prefWin.getFontName();
	    fontStyle = prefWin.getFontStyle();
	    fontSize = prefWin.getFontSize();
	    PlotEditTool t = PlotUtil.getEditTool();
	    if (t == null)
		return;
	    t.changeTextItem(this);
	}

	public String getFontFamily() {
	    return fontName;
	}

	public int getFontStyle() {
	    return fontStyle;
	}

	public int getFontSize() {
	    return fontSize;
	}

	public void writeToFile(PrintWriter os) {
	     int k;

	    if (value == null || value.length() <= 0)
		return;
	    os.print(""+vcommand+" '-draw', 'color', ");
	    k = fgColor.getRed();
	    Format.print(os, "%.3f, ", (double) k);
	    k = fgColor.getGreen();
	    Format.print(os, "%.3f, ", (double) k);
	    k = fgColor.getBlue();
	    Format.print(os, "%.3f", (double) k);
	    os.println(" )");
	    os.print(""+vcommand+" '-draw', 'font', ");
	    if (fontName.equals("Serif"))
               os.print("'Times");
            else
            {
               if (fontName.equals("SansSerif"))
                  os.print("'Helvetica");
               else
                  os.print("'Courier");
            }
            if (fontStyle == Font.BOLD)
               os.print("-Bold");
            else if(fontStyle == Font.ITALIC)
               os.print("-Italic");
	    Format.print(os, "', %d, ", fontSize);
            os.print("'"+fontName+"', ");
            Format.print(os, "%d ", fontStyle);
            os.println(" )");
	    rx = (double) x / (double) panel_w;
	    ry = (double) y / (double) panel_h;
	    os.print(""+vcommand+" '-draw', 'text', ");
            Format.print(os, "%f, ", rx );
            Format.print(os, "%f, ", ry);
            os.print("'");
            for (k = 0; k < value.length(); k++)
            {
                if (value.charAt(k) == '\\')
                    os.print("\\\\\\");
                else if (value.charAt(k) == '\'')
                    os.print("\\");
                Format.print(os, "%c", value.charAt(k));
            }
            os.println("')");
	}

}

