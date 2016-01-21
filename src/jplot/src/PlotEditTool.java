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
import java.awt.event.*;
import java.util.*;
import javax.swing.*;
// import PlotConfig.*;

class PlotEditTool extends JPanel implements  ActionListener, PlotDefines, PlotObj
{
    public PlotEditTool(String dir)
    {
	Dimension  butDim;
	int	   bx, by;
	String     icondir, icon_name;

        this.setLayout(null);
	sysdir = dir;
	icondir = sysdir+"/user_templates/icon/";
        icon_name = icondir+"plot_erase.icon";
        eraseBut = new PlotEditButton(icon_name, ERASE);
        icon_name = icondir+"plot_text.icon";
        textBut = new PlotEditButton(icon_name, TEXTBUT);
        icon_name = icondir+"plot_line.icon";
        lineBut = new PlotEditButton(icon_name, LINE);
        icon_name = icondir+"plot_rectangle.icon";
        rectBut = new PlotEditButton(icon_name, SQUARE);
        icon_name = icondir+"plot_pref.icon";
        colorBut = new PlotEditButton(icon_name, COLORBUT);
        icon_name = icondir+"plot_eraseAll.icon";
        eraseAllBut = new PlotEditButton(icon_name, ERASEALL);
        icon_name = icondir+"plot_arrowR.icon";
        arrowR = new PlotEditButton(icon_name, ARROWRR);
        icon_name = icondir+"plot_arrowL.icon";
        arrowL = new PlotEditButton(icon_name, ARROWLL);
        icon_name = icondir+"plot_printer.icon";
        printBut = new PlotEditButton(icon_name, PRINT);
        butDim = lineBut.getMinimumSize();



	eraseBut.setToolTipText("erase selected item");
	eraseAllBut.setToolTipText("erase all items");
	colorBut.setToolTipText("color & font");
	printBut.setToolTipText("plot");
	textBut.setToolTipText("text input");
	lineBut.setToolTipText("draw line");
	rectBut.setToolTipText("draw square");
	arrowR.setToolTipText("draw arrow");
	arrowL.setToolTipText("draw arrow");

	this.add(eraseBut);
	this.add(colorBut);
	this.add(printBut);
	this.add(textBut);
	this.add(lineBut);
	this.add(rectBut);
	this.add(eraseAllBut);
	this.add(arrowR);
	this.add(arrowL);

	bx = 4;
	by = 8;
	lineBut.setBounds(bx, by, butDim.width,  butDim.height);
	by = by + butDim.height + 8;
	arrowL.setBounds(bx, by, butDim.width,  butDim.height);
	by = by + butDim.height + 8;
	colorBut.setBounds(bx, by, butDim.width,  butDim.height);
	by = by + butDim.height + 8;
	eraseAllBut.setBounds(bx, by, butDim.width,  butDim.height);
	by = by + butDim.height + 18;
	printBut.setBounds(bx, by, butDim.width,  butDim.height);

	bx = butDim.width + 8;
	by = 8;
	rectBut.setBounds(bx, by, butDim.width,  butDim.height);
	by = by + butDim.height + 8;
	arrowR.setBounds(bx, by, butDim.width,  butDim.height);
	by = by + butDim.height + 8;
	textBut.setBounds(bx, by, butDim.width,  butDim.height);
	by = by + butDim.height + 8;
	eraseBut.setBounds(bx, by, butDim.width,  butDim.height);

	this.setSize(butDim.width*2+12, butDim.height * 5);
	textBut.addActionListener(this);
	lineBut.addActionListener(this);
	rectBut.addActionListener(this);
	eraseBut.addActionListener(this);
	colorBut.addActionListener(this);
	eraseAllBut.addActionListener(this);
	arrowL.addActionListener(this);
	arrowR.addActionListener(this);
	printBut.addActionListener(this);
	arrow_draw = new PlotArrowDraw();
	lineBut.setAutoRelease(false);
	rectBut.setAutoRelease(false);
	arrowL.setAutoRelease(false);
	arrowR.setAutoRelease(false);
	textInput = new TextInput(this);
	PlotUtil.setTextWindow(textInput);
        setSize(bx+butDim.width + 8, by + butDim.height + 8);
        setPreferredSize(new Dimension(bx+butDim.width + 8, by + butDim.height + 8));
	validate();
    }

    public void setParentInfo(PlotConfig.PlPanel parent)
    {
	p_object = parent;
        mygc = parent.getGraphics();
	initColors();
        scrn_res = p_object.getScreenRes();
    }

    private void initColors()
    {
        fg = PlotUtil.getWinForeground();
        bg = PlotUtil.getWinBackground();
        hg = PlotUtil.getHighlightColor();
    }

    public void actionPerformed(ActionEvent event)
    {
	PlotEditButton target = (PlotEditButton) event.getSource();
	String s = null;

	win_mode = 0;
	if ((activeBut != null) && (activeBut != target))
	    activeBut.setActive(false);
	activeBut = null;
	p_width = p_object.getWidth();
	p_height = p_object.getHeight();
	win_mode = target.getId();
	if (win_mode == ERASE)
	{
	    delete();
	    win_mode = 0;
	    return;
	}
	if (win_mode == ERASEALL)
	{
	    delete_all();
	    win_mode = 0;
	    return;
	}

	if (win_mode != COLORBUT)
	    clearHighlight();
	
	switch (win_mode) {
	 case ARROWRR:
	 case ARROWLL:
	    	    Point spt = p_object.getResolution();
	    	    if (spt.x > spt.y)
	        	arrowwidth = (int) (spt.x * 0.02 * ratio);
	    	    else
	        	arrowwidth = (int) (spt.y * 0.02 * ratio);
	    	    arrowheight = arrowwidth;
	    	    arrow_draw.setSize(arrowwidth, arrowheight);
	    	    p_object.setModifyMode(true);
	    	    x1 = -1; 
	    	    y1 = -1;
	    	    mygc.setXORMode(PlotUtil.getWinBackground());
	    	    mygc.setColor(hg);
		    activeBut = target;
		    return;
	 case TEXTBUT:
	    	    if (textInput == null)
		    {
			textInput = new TextInput(this);
			PlotUtil.setTextWindow(textInput);
	    		textInput.setTextColor(fg);
		    }
	    	    textInput.show();
	    	    textInput.setState(Frame.NORMAL);
		    return;
	 case COLORBUT:
	    	    color_proc();
		    return;
	 case LINE:
	 case SQUARE:
	    	    p_object.setModifyMode(true);
	    	    x1 = -1; 
	    	    y1 = -1;
	    	    mygc.setXORMode(PlotUtil.getWinBackground());
	    	    mygc.setColor(hg);
		    activeBut = target;
		    return;
	 case PRINT:
		    p_object.vprint();
		    return;
	}
    }

    public void  clearActions()
    {
	win_mode = 0;
	clearHighlight();
	x1 = -1;
	x2 = -1;
	y1 = -1;
	inputStr = null;
	lineBut.setActive(false);
	rectBut.setActive(false);
	arrowL.setActive(false);
	arrowR.setActive(false);
    }

    public void  moveText(int  x, int y )
    {
	if (inputStr == null)
	    return;
	int y2 = 0;
	if (x1 > 0 && y1 > 0) {
	    y2 = y1 - text_h - 1;
	    if(y2 < 0)
		y2 = 0;
	    PlotUtil.restoreArea(new Rectangle(x1, y2, text_w+6, text_h+6));
	}
	if (y <= 4 || x <= XMARGIN)
	{
	    x1 = -1;
	    y1 = -1;
	    return;
	}
	x1 = x;
	y1 = y - 3;
	mygc.drawImage(text_img, x1, y1 - text_h, text_w, text_h, null);
	mygc.drawRect(x1, y1 - text_h, text_w, text_h);
    }

    public void  createTextItem() {
	if (inputStr == null)
	    return;
	if (x1 < 0 || y1 < 0)
	    return;
	int y2 = y1 - text_h - 1;
	if(y2 < 0)
	    y2 = 0;
	PlotUtil.restoreArea(new Rectangle(x1, y2, text_w+6, text_h+6));
	mygc.drawImage(text_img, x1, y1 - text_h, text_w, text_h, null);
	PlotText textItem = new PlotText(inputStr, x1, y1, ratio);
	textItem.setScrnRes(scrn_res);
	textItem.setImage(text_img);
	textItem.setItemFont (font_family, font_style, font_size);
	textItem.setFgColor (fg);
	item_list.add(textItem);
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
	int	n, k, x1, y1, x2, y2;

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

    public void  moveSquare(int  x, int y, boolean hilit )
    {
	PlotMisItem  s_item;
	int	sx, sy, w, h;

	if (x1 > 0 && x2 > 0)
	{
	    if (x1 > x2) {  w = x1 - x2; sx = x2;  }
	    else {  w = x2 - x1; sx = x1;  }
	    if (y1 > y2) {  h = y1 - y2; sy = y2;  }
	    else {  h = y2 - y1; sy = y1;  }
	    if ((w != 0) && (h != 0))
	    {
	    	if (lineRwidth == 1)
	       	    mygc.drawRect(sx, sy, w, h);
	    	else
	       	    drawWRect(sx, sy, w, h);
	    }
	}
	if (x <= XMARGIN || y <= YMARGIN || x >= p_width || y >= p_height)
	{
	    x2 = -1;
	    return;
	}
	x2 = x - XMARGIN;
	y2 = y - YMARGIN;
	if (!hilit)
            mygc.setColor(fg);
	if (x1 > x2) {  w = x1 - x2; sx = x2;  }
	else {  w = x2 - x1; sx = x1;  }
	if (y1 > y2) {  h = y1 - y2; sy = y2;  }
	else {  h = y2 - y1; sy = y1;  }
	if ((w == 0) || (h == 0))
	    return;
	if (lineRwidth == 1)
	     mygc.drawRect(sx, sy, w, h);
	else
	     drawWRect(sx, sy, w, h);
	if (!hilit) 
	{
	   if ((w < 2) || (h < 2))
	   {
		if (lineRwidth == 1)
		    mygc.drawRect(sx, sy, w, h);
		else
		    drawWRect(sx, sy, w, h);
		return;
	   }
	   s_item = new PlotMisItem(this, SQUARE, sx, sy, w,h,p_width, p_height, ratio);
	   s_item.setColors(fg, bg, hg);
	   s_item.setPrefWindow(ipPref);
	   s_item.setScrnRes(scrn_res);
	   item_list.add(s_item);
	   s_item.drawItem();
	}
    }

    public void  moveLine(int  x, int y, boolean hilit )
    {
	PlotMisItem  s_item;
	int	     type;

	if (x1 > 0 && x2 > 0)
	{
	    if (lineRwidth == 1)
	       mygc.drawLine(x1, y1, x2, y2);
	    else
	    {
		drawWLine(x1, y1, x2, y2);
	    }
	}
	if (x <= XMARGIN || y <= YMARGIN || x >= p_width || y >= p_height)
	{
	    x2 = -1;
	    y2 = -1;
	    return;
	}
	x2 = x - XMARGIN;
	y2 = y - YMARGIN;
	if (!hilit)
	{
	    mygc.setPaintMode();
            mygc.setColor(fg);
	}
	if (!hilit) 
	{
	   type = 0;
	   if (win_mode == DLINE)
		type = LINE;
	   else if (win_mode == ARROWLL2)
		type = ARROWLL;
	   else if (win_mode == ARROWRR2)
		type = ARROWRR;
	   s_item = new PlotMisItem(this, type, x1, y1, x2, y2,p_width, p_height, ratio);
	   s_item.setColors(fg, bg, hg);
	   s_item.setPrefWindow(ipPref);
	   s_item.setScrnRes(scrn_res);
	   item_list.add(s_item);
	   s_item.drawItem();
	}
	else
	{
	    if (lineRwidth == 1)
	   	mygc.drawLine(x1, y1, x2, y2);
	    else
	    {
		drawWLine(x1, y1, x2, y2);
	    }
	}
    }

    public void  movePoint(int  x, int y, boolean bigpoint, boolean last)
    {
	if (x1 > 0 && x2 > 0)
	    mygc.drawLine(x1, y1, x2, y2);
	if (x > XMARGIN && y > YMARGIN)
	{
	    x1 = x - XMARGIN;
	    y1 = y - YMARGIN;
	    y2 = y1;
	    if (bigpoint)
		x2 = x1 + 3;
	    else
		x2 = x1;
	    if (!last)
	       mygc.drawLine(x1, y1, x2, y2);
	}
	else
	{
	    x1 = -1;
	    x2 = -1;
	    y1 = -1;
	}
    }

    public void  drawArrow(Graphics gc, int item, int  x, int y)
    {
	arrow_draw.draw(gc, item, x, y);
    }

    public void  drawArrow(int item, int  x, int y)
    {
	arrow_draw.draw(mygc, item, x, y);
    }

    public void  drawArrow2(Graphics gc, int item, int  x, int y, int x2, int y2, int lw)
    {
	arrow_draw.draw2(gc, item, x, y, x2, y2, lw);
    }

    public void  drawArrow2(int item, int  x, int y, int x2, int y2, int lw)
    {
	arrow_draw.draw2(mygc, item, x, y, x2, y2, lw);
    }

    public void  moveArrow(int  x, int y, boolean hilit )
    {
	PlotMisItem  s_item;

	if (x1 > 0)
	    arrow_draw.draw(mygc, win_mode, x1, y1);
	y = y - YMARGIN;
	if (x < 1 || y < arrowheight || x >= p_width || y >= (p_height-arrowheight))
	{
	    x1 = -1;
	    y1 = -1;
	    return;
	}
	if (!hilit)
	{
	    mygc.setPaintMode();
            mygc.setColor(fg);
	}
	x1 = x;
	y1 = y - YMARGIN;
	arrow_draw.draw(mygc, win_mode, x1, y1);
	if (!hilit) 
	{
	   s_item = new PlotMisItem(this, win_mode, x1, y1, arrowwidth, arrowheight,p_width, p_height, ratio);
	   s_item.setColors(fg, bg, hg);
	   s_item.setPrefWindow(ipPref);
	   s_item.setScrnRes(scrn_res);
	   item_list.add(s_item);
	   s_item.drawItem();
	}
    }

    public void mEvent(MouseEvent evt)
    {
	int	pmode;
        int     x = evt.getX();
        int     y = evt.getY();

	if (win_mode == 0)
	{
	    clearActions();
	    return;
	}
	pmode = evt.getModifiers();
	switch (evt.getID()) {
	 case MouseEvent.MOUSE_PRESSED:
			
			switch (win_mode) {
			    case TEXT:
					moveText(x, y);
					break;
			    case LINE:
					movePoint(x, y, false, true);
					win_mode = DLINE;
					break;
			    case ARROWRR:
					movePoint(x, y, false, true);
					win_mode = ARROWRR2;
					break;
			    case ARROWLL:
					movePoint(x, y, false, true);
					win_mode = ARROWLL2;
					break;
			    case SQUARE:
					movePoint(x, y, false, true);
					win_mode = MSQUARE;
					break;
			    case IMOVE:
					if ((pmode & (1 << 4)) == 0) // not button 1
					{
			    		    clearActions();
			    		    return;
					}
				        if (x > XMARGIN && y > YMARGIN)
					{
					   x1 = x;
					   y1 = y;
					   moveStart();
					}
					else
					   x1 = -1;
					break;
			}
			if (x1 < 0)
			    clearActions();
			x2 = -1;
			break;
              case MouseEvent.MOUSE_RELEASED:
			if (win_mode >= ARROWUP && win_mode <= ARROWLUP)
			{
				moveArrow(x, y, false);
				clearActions();
				return;
			}
			switch (win_mode) {
			    case TEXT:
					createTextItem();
					break;
			    case DLINE:
			    case ARROWLL2:
			    case ARROWRR2:
					moveLine(x, y, false);
					break;
			    case MSQUARE:
					moveSquare(x, y, false);
					break;
			    case IMOVE:
					moveStop();
					break;
			}
			clearActions();
			break;
              case MouseEvent.MOUSE_CLICKED:
			break;
              case MouseEvent.MOUSE_ENTERED:
			break;
              case MouseEvent.MOUSE_EXITED:
			if (win_mode >= ARROWUP && win_mode <= ARROWLUP)
			{
				moveArrow(-1, -1, true);
				return;
			}
			switch (win_mode) {
			    case TEXT:
					moveText(-1, -1);
					break;
			    case LINE:
			    case SQUARE:
			    case ARROWRR:
			    case ARROWLL:
					movePoint(-1, -1, true, true);
					break;
			    case DLINE:
			    case ARROWRR2:
			    case ARROWLL2:
					moveLine(-1, -1, true);
					break;
			    case MSQUARE:
					moveSquare(x, y, true);
					break;
			    case IMOVE:
					break;
			}
			break;
	      case MouseEvent.MOUSE_MOVED:
			if (win_mode >= ARROWUP && win_mode <= ARROWLUP)
			{
				moveArrow(x, y, true);
				return;
			}
			switch (win_mode) {
			    case TEXT:
					moveText(x, y);
					break;
			    case LINE:
			    case SQUARE:
			    case ARROWRR:
			    case ARROWLL:
					movePoint(x, y, true, false);
					break;
			}
			break;
              case MouseEvent.MOUSE_DRAGGED:
			if (win_mode >= ARROWUP && win_mode <= ARROWLUP)
			{
				moveArrow(x, y, true);
				return;
			}
			switch (win_mode) {
			    case TEXT:
					moveText(x, y);
					break;
			    case DLINE:
			    case ARROWRR2:
			    case ARROWLL2:
					moveLine(x, y, true);
					break;
			    case MSQUARE:
					moveSquare(x, y, true);
					break;
			    case IMOVE:
				        if (x > XMARGIN && y > YMARGIN)
					    hilit_item.move(mygc, x-x1, y-y1, 1);
					break;
			}
			break;
	}
    }

    public void keyMove(int dir)
    {
	int	x, y;
 
	if (hilit_item == null)
	    return;
	Rectangle dim = hilit_item.getRegion();
	x = 0;
	y = 0;
	switch (dir) {
	 case LEFT:
		x =  - 1;
		break;
	 case RIGHT:
		x = 1;
		break;
	 case UP:
		y = -1;
		break;
	 case DOWN:
		y = 1;
		break;
	 case STOP:
		break;
	 default:
		return;
	}
	if ((dim.x + x > XMARGIN) && (dim.y + y > YMARGIN))
	{
	    mygc.setXORMode(PlotUtil.getWinBackground());
            mygc.setColor(hg);
	    hilit_item.move(mygc, x, y, 3);
	}
	if (dir == STOP)
	{
	    moveStop();
	    clearActions();
	}
    }

    public void putText(boolean flag, String str)
    {
	x1 = -1;
	y1 = -1;
	inputStr = str;
	if (!flag)
	{
	    if (win_mode != 0)
	    {
	   	win_mode = 0;
	    	p_object.setModifyMode(false);
	    }
	    return;
	}
	if (inputStr == null || inputStr.length() <= 0)
	    return;
	win_mode = TEXT;
	p_object.setModifyMode(true);
	font_family = textInput.getFontFamily();
	font_style = textInput.getFontStyle();
	font_size = textInput.getFontSize();
	font = new Font(font_family, font_style, (int)(font_size * ratio));
	mygc.setFont(font);
	fontMetric = mygc.getFontMetrics();
        textwidth = fontMetric.stringWidth(inputStr);
        textheight = fontMetric.getHeight();
	text_img = null;
        preview_text(inputStr, fg);
	if (text_img == null) {
	    win_mode = 0;
	    return;
	}
	PlotUtil.backUp();
        mygc.setColor(hg);
	mygc.setPaintMode();
    }


    public void changeTextItem(PlotText s) {
	font_family = s.getFontFamily();
	font_style = s.getFontStyle();
	font_size = s.getFontSize();
	inputStr = s.getValue();
	font = new Font(font_family, font_style, (int)(font_size * ratio));
	mygc.setFont(font);
	fontMetric = mygc.getFontMetrics();
        textwidth = fontMetric.stringWidth(inputStr);
        textheight = fontMetric.getHeight();
	text_img = null;
        preview_text(inputStr, fg);
	if (text_img == null) {
	    return;
	}
	PlotUtil.backUp();
	s.setImage(text_img);
    }

    public void setRatio(double r, int pw, int ph)
    {
	EditItemIF  item;

	p_width = pw;
	p_height = ph;
	ratio = r;
	for (int k = 0; k < item_list.size(); k++) {
	    item = (EditItemIF) item_list.elementAt(k);
	    item.setRatio(ratio, pw, ph);
	}
	if (pw > ph)
	    arrowwidth = (int) (pw * 0.02 * ratio);
	else
	    arrowwidth = (int) (ph * 0.02 * ratio);
	arrowheight = arrowwidth;
	arrow_draw.setSize(arrowwidth, arrowheight);
    }

    public void output(PrintWriter os)
    {
	EditItemIF  item;
	Point  spt = p_object.getResolution();

	os.println(""+vcommand+" '-draw', 'start', 0.0, 0.0, 1.0, 1.0 )");

	for (int k = 0; k < item_list.size(); k++) {
	    item = (EditItemIF) item_list.elementAt(k);
	    item.writeToFile(os);
	}

	os.println(""+vcommand+" '-draw', 'end', 0 )");
    }


    public void paint()
    {
	PlotMisItem  item;
    }

    public void show_items()
    {
	EditItemIF  item;

	for (int k = 0; k < item_list.size(); k++) {
	    item = (EditItemIF) item_list.elementAt(k);
	    item.drawItem();
	}
    }

    public void show_items(Graphics gc)
    {
	EditItemIF  item;

	for (int k = 0; k < item_list.size(); k++) {
	    item = (EditItemIF) item_list.elementAt(k);
	    item.drawItem(gc);
	}
    }

    public void backUp(Graphics gc)
    {
	EditItemIF  item;

	for (int k = 0; k < item_list.size(); k++) {
	    item = (EditItemIF) item_list.elementAt(k);
	    if (item != hilit_item)
	        item.drawItem(gc);
	}
    }

    public void recoverItems(Rectangle rect)
    {
	int	todo;
	int	xs1, ys1, xs2, ys2;
	EditItemIF item;
	Rectangle  dim2;

	xs1 = rect.x - 1;
	ys1 = rect.y - 1;
	xs2 = rect.x + rect.width;
	ys2 = rect.y + rect.height;
	for (int k = 0; k < item_list.size(); k++) {
	    item = (EditItemIF) item_list.elementAt(k);
	    dim2 = item.getRegion();
	    todo = 0;
	    if (dim2.x <= xs1 && dim2.width >= xs2)
	        todo = 1;
	    else
	    {
	        if (dim2.x >= xs1 && dim2.x <= xs2)
		    todo = 1;
	        else if (dim2.width >= xs1 && dim2.width <= xs2)
		    todo = 1;
	    }
	    if (item == hilit_item)
		todo = 0;
	    if (todo > 0)
	    {
		if (dim2.y <= ys1 && dim2.height >= ys1)
		    todo = 2;
		else
		{
		    if (dim2.y >= ys1 && dim2.y <= ys2)
			todo = 2;
		    else
		    {
		  	if (dim2.height >= ys1 && dim2.height <= ys2)
			    todo = 2;
		    }
		}
	    }
	    if (todo > 1)
		item.drawItem();
	}
    }

    private int  selectArrowItem(PlotMisItem  item, int x, int y)
    {
	tpoint = new Point(-1, -1);
	Rectangle dim = item.getRegion();
	if (x < dim.x || x > dim.width)
	    return 0;
	if (y < dim.y || y > dim.height)
	    return 0;
	tpoint.x = dim.x - x;
	tpoint.y = dim.y - y;
	return 1;
    }

    private int  selectTextItem(EditItemIF  item, int x, int y)
    {
	tpoint = new Point(-1, -1);
	Rectangle dim = item.getRegion();
	if (x < dim.x || x > dim.width)
	    return 0;
	if (y < dim.y || y > dim.height)
	    return 0;
	tpoint.x = dim.x - x;
	tpoint.y = dim.y - y;
	return 1;
    }

    private int  selectLineItem(PlotMisItem  item, int x, int y)
    {
	tpoint = new Point(-1, -1);
	Rectangle dim = item.getVector();
	double  slope;
	int	sx1, sx2, sy1, sy2;
	int	dx, dy, margin;

	if (dim.x > dim.width) {
	    sx1 = dim.width;
	    sx2 = dim.x;
	    sy1 = dim.height;
	    sy2 = dim.y;
	}
	else {
	    sx1 = dim.x;
	    sx2 = dim.width;
	    sy1 = dim.y;
	    sy2 = dim.height;
	}
	    
	dx = sx2 - sx1;
	margin = 0;
	if (dx < 5)
	   margin = 3;
	if (x < (sx1 - margin) || x > (sx2 + margin))
	      return 0;
	margin = 0;
	dy = Math.abs(sy1 - sy2);
	if (dy < 5)
	   margin = 3;
	if (sy1 < sy2)
	{
	   if (y > (sy2 + margin) || y < (sy1 - margin))
	            return 0;
	}
	else
	{
	   if (y > (sy1 + margin) || y < (sy2 - margin))
	        return 0;
	}
	if (dx < 5 || dy < 5)
	{
	    if (dx < 5)
	    {
		tpoint.x = dx;
		tpoint.y = 0;
	    }
	    else
	    {
		tpoint.x = 0;
		tpoint.y = dy;
	    }
	    return 1;
	}
	dy = sy2 - sy1;
	slope = (double)(dy) / (double)(dx);
	dy = sy1 + (int) ((x - sx1) * slope);
	tpoint.x = 0;
	tpoint.y = Math.abs(dy - y);
	if (tpoint.y > 10)
	    return 0;
	return 1;
    }

    private int  selectSquareItem(PlotMisItem  item, int x, int y)
    {
	int	dx1, dx2, dy1, dy2, dh;
	tpoint = new Point(-1, -1);
	Rectangle dim = item.getRegion();

	dh = item.getLineWidth();
	if (dh < 3)
	   dh = 3;
	dx1 = dim.x - dh;
	dx2 = dim.width + dh;
	dy1 = dim.y - dh;
	dy2 = dim.height + dh;
	if ((y < dy1) || (y > dy2))
		return 0;
	if ((x < dx1) || (x > dx2))
		return 0;
	if (Math.abs(y - dy1) > Math.abs(dy2 - y)) 
	    tpoint.y = Math.abs(dy2 - y);
	else
	    tpoint.y = Math.abs(y - dy1);
	if (Math.abs(x - dx1) > Math.abs(dx2 - x)) 
	    tpoint.x = Math.abs(dx2 - x);
	else
	    tpoint.x = Math.abs(x - dx1);
	if (tpoint.x < tpoint.y)
	    tpoint.y = tpoint.x;
	tpoint.x = 0;
	if (tpoint.y > 10)
	    return 0;
	return 1;
    }

    public void selectItem(MouseEvent evt)
    {
        int     x = evt.getX();
        int     y = evt.getY();
	int	ret;
	int	difx, dify;
	EditItemIF  item, nhitem;

	difx = 9999;
	dify = 9999;
	nhitem = null;
	for (int k = 0; k < item_list.size(); k++) {
	    item = (EditItemIF) item_list.elementAt(k);
	    int  type = item.getType();
	    ret = 0;
	    if (type >= ARROWUP && type <= ARROWLUP) {
	        ret = selectArrowItem((PlotMisItem) item, x, y);
	    }
	    else
	    {
		switch (type) {
		  case TEXT:
			    ret = selectTextItem(item, x, y);
			    break;
		  case LINE:
			    ret = selectLineItem((PlotMisItem) item, x, y);
			    break;
		  case SQUARE:
			    ret = selectSquareItem((PlotMisItem) item, x, y);
			    break;
		  case GARROW:
			    ret = selectLineItem((PlotMisItem) item, x, y);
			    break;
		}
	    }
	    if (ret > 0)
	    {
		if (tpoint.x <= difx && tpoint.y <= dify)
		{
		    nhitem = item;
		    difx = tpoint.x;
		    dify = tpoint.y;
		}
	    }
	}
	if (nhitem == hilit_item)
	    return;
	if (hilit_item != null)
	    hilit_item.highlight(false);
	hilit_item = nhitem;
	if (hilit_item != null)
	{
	    p_object.setModifyMode(true);
	    win_mode = IMOVE;
	    PlotUtil.backUp();
	    hilit_item.highlight(true);
	    hilit_item.showPreference();
	    if (textInput != null)
	       hilit_item.setTextInfo(textInput);
	}
    }

    public void delete_all()
    {
	hilit_item = null;
	if (item_list.size() > 0) {
	    item_list.clear();
	    p_object.repaint();
	    p_object.backUpArea();
	}
    }

    public void moveStart()
    {
	if (hilit_item == null)
	    return;
	mygc = PlotUtil.getGC();
	mygc.setXORMode(PlotUtil.getWinBackground());
	mygc.setColor(hg);
	hilit_item.move(mygc, 0, 0, 0);
    }

    public void moveStop()
    {
	mygc.setPaintMode();
	hilit_item.move(mygc, 0, 0, 2);
    }


    public void delete()
    {
	int	todo;
	EditItemIF item;

	if (hilit_item == null)
	    return;
	PlotUtil.backUp();
	Rectangle dim = hilit_item.getDim();
	for (int k = 0; k < item_list.size(); k++) {
	    item = (EditItemIF) item_list.elementAt(k);
	    if (item == hilit_item)
	    {
		item_list.remove(item);
		hilit_item = null;
		break;
	    }
	}
	PlotUtil.restoreArea(dim);
    }

    public void color_proc()
    {
	if (ipPref == null)
	    return;
	ipPref.show();
	ipPref.setState(Frame.NORMAL);
    }

    static String checkString(String s)
    {
	int	k, len, slash;
	StringBuffer buf;

	len = s.length();
	buf = new StringBuffer(len+1);
	k = 0;
	slash = 0;
        while ( k < len)
        {
             if (s.charAt(k) == '\\')
	     {
		 if (slash == 3)
		 {
		     buf.append(s.charAt(k));
		     slash = 0;
		 }
		 else
		     slash++;
	     }
	     else
	     {
		 slash = 0;
                 buf.append(s.charAt(k));
	     }
	     k++;
        }
        return(buf.toString());
    }

    public void addItem (String data)
    {
	String  name, str;
	int	type, len, k, k1, k2;
	double  x, y, w, h;
	StringTokenizer tok = new StringTokenizer(data, " ,\n\t");
        name = tok.nextToken();
        name = tok.nextToken();
	type = -1;
        name = tok.nextToken();
	if (name.equals("'font'"))
        {
            name = tok.nextToken();
	    font_size = Format.atoi(tok.nextToken());
	    name = tok.nextToken();
	    len = name.length();
	    font_family = name.substring(1, len - 1);
	    font_style = Format.atoi(tok.nextToken());
	    return;
        }
	if (name.equals("'text'"))
        {
	    type = TEXT;
            x = Format.atof(tok.nextToken());
            y = Format.atof(tok.nextToken());
	    name = tok.nextToken("\0\n");
	    name.trim();
	    len = name.length();
	    if ( len < 3)
		return;
	    k = 0;
	    while (k < len - 1)
	    {
		if (name.charAt(k) == '\'')
		{
		    k++;
		    break;
		}
	        k++;
	    }
	    k1 = k;
	    k2 = 0;
	    while (k1 < len)
	    {
		if (name.charAt(k1) == '\'')
		    k2 = k1;
	        k1++;
	    }
	    if (k2 > k)
	    {
	        str = name.substring(k, k2);
		inputStr = checkString(str);
	    }
	    else
		return;
	    font = new Font(font_family, font_style, (int)(font_size * ratio));
	    mygc.setFont(font);
	    fontMetric = mygc.getFontMetrics();
            textwidth = fontMetric.stringWidth(inputStr);
            textheight = fontMetric.getHeight();
	    text_img = null;
	    if (item_color != null)
                preview_text(inputStr, item_color);
	    else
                preview_text(inputStr, fg);
	    if (text_img == null) {
	    	return;
	    }
	    int  x1 = (int) (p_width * x);
	    int  y1 = (int) (p_height * y);
	    PlotText textItem = new PlotText(inputStr, x1, y1, ratio);
	    textItem.setScrnRes(scrn_res);
	    textItem.setImage(text_img);
	    textItem.setItemFont (font_family, font_style, font_size);
	    if (item_color != null)
	        textItem.setFgColor (item_color);
	    else
	        textItem.setFgColor (fg);
	    item_list.add(textItem);
	    textItem.drawItem();

	    return;
        }
	
	if (name.equals("'color'"))
	{
            x = Format.atof(tok.nextToken());
            y = Format.atof(tok.nextToken());
            w = Format.atof(tok.nextToken());
	    item_color = new Color((int)x, (int)y, (int)w);
	    return;
	}
	if (name.equals("'linewidth'"))
	{
            line_width = Format.atoi(tok.nextToken());
	    lineRwidth = (int) (line_width * 72.0 * 0.6 / (double)scrn_res);
	    if (lineRwidth < 1)
		lineRwidth = 1;
	    return;
	}

	if (name.equals("'line'"))
	    type = LINE;
	else if (name.equals("'square'"))
	    type = SQUARE;
	else if (name.equals("'garrow'"))
	    type = GARROW;
	else if (name.equals("'arrow'"))
	    type = Format.atoi(tok.nextToken()) + ARROWUP - 1;
	if (type < 0)
	    return;
        x = Format.atof(tok.nextToken());
        y = Format.atof(tok.nextToken());
        w = Format.atof(tok.nextToken());
        h = Format.atof(tok.nextToken());
	PlotMisItem item = new PlotMisItem(this, type, x, y, w, h,p_width, p_height, ratio);
	item_list.add(item);
	item.setPrefWindow(ipPref);
	item.setScrnRes(scrn_res);
	if (item_color != null)
	   item.setColors(item_color, bg, hg);
	else
	   item.setColors(fg, bg, hg);
	item.setLineWidth(line_width);
    }

    public void clearHighlight()
    {
	if (hilit_item != null)
	{
	    hilit_item.highlight(false);
	    hilit_item = null;
	}
	p_object.setModifyMode(false);
    }

    public PlotObj getHighlight()
    {
	return (PlotObj) hilit_item;
    }

    public void setForeground(Color c)
    {
        fg = c;
	if (textInput != null)
	    textInput.setTextColor(fg);
    }

    public void setHilitColor(Color c)
    {
        hg = c;
    }

    public void setFgColor(Color c)
    {
        fg = c;
	if (textInput != null)
	    textInput.setTextColor(fg);
    }

    public void setPrefWindow(PlotItemPref c)
    {
	ipPref = c;
    }

    public void setLineWidth(int c)
    {
	if (c > 0)
	   line_width = c;
	else
	   line_width = 1;
	lineRwidth = (int) (line_width * 72.0 * 0.6 / (double)scrn_res);
	if (lineRwidth < 1)
	   lineRwidth = 1;
    }

    public void setPreference()
    {
	if (ipPref == null)
	   return;
	line_width = ipPref.getItemLineWidth();
	lineRwidth = (int) (line_width * 72.0 * 0.6 / (double)scrn_res);
	if (lineRwidth < 1)
	   lineRwidth = 1;
	if (hilit_item != null)
	{
	   Rectangle dim = hilit_item.getDim();
	   PlotUtil.restoreArea(dim);
	   hilit_item.setPreference();
	   hilit_item.drawItem();
	}
    }

    public void showPreference()
    {
	if (hilit_item != null)
	   hilit_item.showPreference();
    }

    public void setColors(Color a, Color b, Color c)
    {
	EditItemIF  item;

	fg = a;
	bg = b;
	hg = c;
	for (int k = 0; k < item_list.size(); k++) {
	    item = (EditItemIF) item_list.elementAt(k);
	    item.setColors(fg, bg, hg);
	}
    }


    public void preview_text(String str, Color c) {
	text_img = PlotUtil.getTextImage(inputStr, font_family, font_style,
			font_size, textwidth, textheight, c);
	if (text_img == null) {
	    return;
	}
	text_w_org = text_img.getWidth(null);
        text_h_org = text_img.getHeight(null);
	text_w = (int) (text_w_org * ratio);
	text_h = (int) (text_h_org * ratio);
    }


    private Vector  item_list = new Vector();
    private int	    x1, x2, y1, y2;
    private int	    p_width, p_height;
    private double  ratio = 1.0;
    private Point   tpoint;
    private PlotConfig.PlPanel p_object;
    public Graphics mygc = null;
    PlotEditButton colorBut;
    PlotEditButton eraseBut, lineBut, rectBut, textBut, eraseAllBut;
    PlotEditButton arrowL, arrowR, printBut;
    private PlotEditButton activeBut = null;
    private String  inputStr = null;
    private String  font_family;
    private String  sysdir, userdir;
    private Font    font;
    private int     font_style, font_size;
    private int     textwidth, textheight;
    private int     arrowwidth, arrowheight;
    private int     line_width = 1;
    private int     lineRwidth = 1;
    private int	    win_mode = 0;
    private int	    scrn_res = 90;
    private int	    text_w_org, text_h_org, text_w, text_h;
    private Color   item_color = null;
    private Color   fg, bg, hg;
    private int[]   xpnts = new int[4];
    private int[]   ypnts = new int[4];
    private TextInput textInput = null;
    private FontMetrics fontMetric;
    private EditItemIF  hilit_item = null;
    private PlotArrowDraw  arrow_draw = null;
    private PlotItemPref  ipPref = null;
    private Image   text_img;
    static final int XMARGIN = 2;
    static final int YMARGIN = 3;
    static final double pi = 3.1416;
    static final int LEFT = 1;
    static final int RIGHT = 2;
    static final int UP = 3;
    static final int DOWN = 4;
    static final int STOP = 5;
}
