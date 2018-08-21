/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.util.*;
import java.beans.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VScroll extends JComponent implements VObjIF, VObjDef,
    DropTargetListener, PropertyChangeListener
{
    public String type = null;
    public String label = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String choiceValue = null;
    public String objName = null;
    public Color  fgColor = Color.black;
    public Color  bgColor;
    public Font   font = null;
    public Font   font2 = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private JLabel labelWidget;
    private JButton upWidget, downWidget;
    private int    width = 0;
    private int    height = 0;
    private int    choiceIndex = 0;
    private int    isActive = 1;
    private Vector choiceVector = null;
    private ButtonIF vnmrIf;
    private boolean inModalMode = false;

    private FontMetrics fm = null;
    private float fontRatio = 1;
    private int rWidth = 0;
    private int rWidth2 = 0;
    private int nWidth = 0;
    private int iconWidth = 5;
    private int rHeight = 0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);



    public VScroll(SessionShare sshare, ButtonIF vif, String typ) {
	this.type = typ;
	this.vnmrIf = vif;
	this.fg = "PlainText";
        this.fontSize = "8";
        this.label = "null";
	setOpaque(false);
	labelWidget = new JLabel(this.label);
	ImageIcon icon = Util.getImageIcon("up.gif");
	if (icon != null)
	    iconWidth = icon.getIconWidth();
	upWidget = new JButton(icon);
	icon = Util.getImageIcon("down.gif");
	downWidget = new JButton(icon);
	if (icon != null)
	    iconWidth = iconWidth + icon.getIconWidth();
	setLayout(new vScrollLayout());
	add(labelWidget);
	add(upWidget);
	add(downWidget);
	bgColor = Util.getBgColor();
        setBackground(bgColor);
	upWidget.setBorder(new VButtonBorder());
	downWidget.setBorder(new VButtonBorder());
	ml = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                    }
                }
             }
	};

	ActionListener al = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                scrollAction(evt);
            }
	};

	upWidget.addActionListener(al);
	downWidget.addActionListener(al);

	new DropTarget(this, this);
	new DropTarget(labelWidget, this);
	new DropTarget(upWidget, this);
	new DropTarget(downWidget, this);
	DisplayOptions.addChangeListener(this);
    }

     public void propertyChange(PropertyChangeEvent evt)
     {
        if (fg != null)
	{
	    fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
	}
	changeFont();
	bgColor = Util.getBgColor();
	setBackground(bgColor);
    }

    public void setDefLabel(String s) {
    	this.label = s;
	labelWidget.setText(s);
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
	labelWidget.setOpaque(s);
	upWidget.setOpaque(s);
	downWidget.setOpaque(s);
	if (s) {
           addMouseListener(ml);
           if (font != null)
                labelWidget.setFont(font);
           fontRatio = 1.0f;
	   defDim = getPreferredSize();
           curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
           curDim.width = defDim.width;
           curDim.height = defDim.height;
           rWidth2 = rWidth;
	}
        else {
           removeMouseListener(ml);
	}
	inEditMode = s;
    }

    public void changeFont() {
	font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        labelWidget.setFont(font);
        rHeight = font.getSize();
	fontRatio = 1.0f;
	font2 = null;
        fm = labelWidget.getFontMetrics(font);
	if (choiceVector != null) {
	    calSize();
	    if (!inEditMode) {
                if ((curDim.width > 0) && (rWidth > curDim.width)) {
                        adjustFont();
                }
            }
        }
        repaint();
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    public String getAttribute(int attr) {
        switch (attr) {
          case TYPE:
                     return type;
          case LABEL:
                     return label;
          case FGCOLOR:
                     return fg;
          case BGCOLOR:
                     return bg;
          case SHOW:
                     return showVal;
          case FONT_NAME:
                     return fontName;
          case FONT_STYLE:
                     return fontStyle;
          case FONT_SIZE:
                     return fontSize;
	  case SETCHOICE:
                     return null;
	  case SETCHVAL:
	  case VALUES:
                     return choiceValue;
	  case SETVAL:
                     return setVal;
	  case VARIABLE:
                     return vnmrVar;
	  case CMD:
                     return vnmrCmd;
	  case VALUE:
		     if (choiceVector == null)
                        return null;
		     if (choiceIndex < choiceVector.size()) {
                        return (String) choiceVector.elementAt(choiceIndex);
		     }
                     else
                        return null;
	  case PANEL_NAME:
                     return objName;
          default:
                    return null;
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
          case TYPE:
                     type = c;
                     break;
          case LABEL:
                     label = c;
                     labelWidget.setText(c);
                     break;
          case FGCOLOR:
                     fg = c;
                     fgColor = VnmrRgb.getColorByName(c);
                     labelWidget.setForeground(fgColor);
                     repaint();
                     break;
	  case BGCOLOR:
                     bg = c;
		     if (c != null) {
                        bgColor = VnmrRgb.getColorByName(c);
                        setOpaque(true);
                     }
                     else {
                        bgColor = Util.getBgColor();
                        setOpaque(inEditMode);
                     }
                     setBackground(bgColor);
                     repaint();
                     break;
          case SHOW:
                     showVal = c;
                     break;
          case FONT_NAME:
                     fontName = c;
                     break;
          case FONT_STYLE:
                     fontStyle = c;
                     break;
          case FONT_SIZE:
                     fontSize = c;
                     break;
          case SETVAL:
                     setVal = c;
                     break;
	  case SETCHVAL:
	  case VALUES:
                     choiceValue = c;
                     choiceVector = ParamInfo.parseChoiceStr(c);
		     choiceIndex = 0;
		     if (choiceVector != null)
        	       labelWidget.setText((String) choiceVector.elementAt(0));
		     rWidth = 0;
                     break;
	  case VARIABLE:
                     vnmrVar = c;
                     break;
	  case CMD:
                     vnmrCmd = c;
                     break;
	  case VALUE:
        	     labelWidget.setText(c);
                     break;
          case PANEL_NAME:
                     objName = c;
                     break;
        }
    }


    public void scrollAction(ActionEvent  evt) {
	if (inModalMode || inEditMode || choiceVector == null)
	    return;
	if (vnmrIf == null || isActive < 0)
	    return;
        Object obj = evt.getSource();
        if (obj instanceof JButton) {
            JButton b = (JButton) obj;
	    if (b.equals(upWidget))
		choiceIndex++;
	    else
		choiceIndex--;
	    if (choiceIndex < 0)
		choiceIndex = 0;
	    if (choiceIndex >= choiceVector.size())
		choiceIndex = choiceVector.size() - 1;
	    if (vnmrCmd != null) {
                vnmrIf.sendVnmrCmd(this, vnmrCmd);
	    }
            if (setVal != null)
	        vnmrIf.asyncQueryParam(this, setVal);
	    else
 	        labelWidget.setText((String) choiceVector.elementAt(choiceIndex));
	}
    }

    public void destroy() {}

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void setValue(ParamIF pf) {
	if (pf != null && pf.value != null)
            labelWidget.setText(pf.value);
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
	    isActive = Integer.parseInt(s);
            if (isActive > 0) {
                if (bg != null) {
                    setBackground(bgColor);
		    labelWidget.setOpaque(true);
                    labelWidget.setBackground(bgColor);
		    upWidget.setOpaque(true);
		    downWidget.setOpaque(true);
                    upWidget.setBackground(bgColor);
                    downWidget.setBackground(bgColor);
                }
		else {
		    labelWidget.setOpaque(inEditMode);
		    upWidget.setOpaque(inEditMode);
		    downWidget.setOpaque(inEditMode);
		}
            }
            else {
		labelWidget.setOpaque(true);
		upWidget.setOpaque(true);
		downWidget.setOpaque(true);
                if (isActive == 0) {
                    setBackground(Global.IDLECOLOR);
                    labelWidget.setBackground(Global.IDLECOLOR);
                    upWidget.setBackground(Global.IDLECOLOR);
                    downWidget.setBackground(Global.IDLECOLOR);
		}
                else {
                    setBackground(Global.NPCOLOR);
                    labelWidget.setBackground(Global.NPCOLOR);
                    upWidget.setBackground(Global.NPCOLOR);
                    downWidget.setBackground(Global.NPCOLOR);
		}
            }
            if (isActive >= 0) {
                upWidget.setEnabled(true);
                downWidget.setEnabled(true);
                if (setVal != null) {
                    vnmrIf.asyncQueryParam(this, setVal);
                }
            }
            else {
                upWidget.setEnabled(false);
                downWidget.setEnabled(false);
            }
	    repaint();
        }
    }

    public void updateValue() {
	if (vnmrIf == null)
	    return;
        if (showVal != null) {
	    vnmrIf.asyncQueryShow(this, showVal);
        }
        else if (setVal != null) {
	    vnmrIf.asyncQueryParam(this, setVal);
        }
    }


    public void paint(Graphics g) {
	super.paint(g);
	if (!isEditing)
	    return;
	Dimension  psize = getSize();
	if (isFocused)
            g.setColor(Color.yellow);
	else
            g.setColor(Color.green);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }


    public void refresh() {
    }

    class vScrollLayout implements LayoutManager {
        private Dimension psize;

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
                psize = getPreferredSize();
            return  psize; // unused
        } // preferredLayoutSize()

       public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()
       public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
              Dimension dim0 = getSize();
              Dimension dim1 = downWidget.getPreferredSize();
              Dimension dim2 = upWidget.getPreferredSize();
	      int x = dim0.width - dim1.width - 3;
	      int y = (dim0.height - dim1.height) / 2;
	      if (x < 0)
		 x = 0;
	      if (y < 0)
		 y = 0;
	      downWidget.setBounds(x, y, dim1.width, dim1.height);
	      x = x - dim2.width ;
	      if (x < 0)
		 x = 0;
	      upWidget.setBounds(x, y, dim2.width, dim2.height);
	      iconWidth = dim1.width + dim2.width + 5;
	      x = dim0.width - iconWidth;
	      if (x <= 0)
		 x = 0;
              labelWidget.setBounds(0, 0, x, dim0.height);
	   }
       }
    } // vScrollLayout


    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
 	    VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {
	    if (vnmrCmd != null) {
                vnmrIf.sendVnmrCmd(this, vnmrCmd);
	    }
    }

    public void setDefLoc(int x, int y) {
        defLoc.x = x;
        defLoc.y = y;
    }

    public Point getDefLoc() {
        tmpLoc.x = defLoc.x;
        tmpLoc.y = defLoc.y;
        return tmpLoc;
    }

    public void setSizeRatio(double x, double y) {
        double xRatio =  x;
        double yRatio =  y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
	if (defDim.width <= 0)
	   defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double)defDim.width * xRatio);
        curDim.height = (int) ((double)defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }

    public void calSize() {
        if (fm == null) {
            font = labelWidget.getFont();
            fm = labelWidget.getFontMetrics(font);
        }
        rHeight = font.getSize();
	rWidth = iconWidth;
        rWidth2 = rWidth;
	if (choiceVector == null)
	    return;
	String str;
	int len = 0;
	int count = choiceVector.size();
	for (int k = 0; k < count; k++) {
	    str = (String) choiceVector.elementAt(k);
	    if (str != null) {
		len = fm.stringWidth(str);
	  	if (len > rWidth)
		    rWidth = len;
	    }
	}
	rWidth += iconWidth;
        rWidth2 = rWidth;
    }

    public void reshape(int x, int y, int w, int h) {
        if (inEditMode) {
           defLoc.x = x;
           defLoc.y = y;
           defDim.width = w;
           defDim.height = h;
        }
        curDim.width = w;
        curDim.height = h;
        curLoc.x = x;
        curLoc.y = y;
        if (!inEditMode) {
            if (rWidth <= 0) {
                calSize();
            }
            if ((w != nWidth) || (w < rWidth2)) {
                adjustFont();
            }
        }
        super.reshape(x, y, w, h);
    }

    public Point getLocation() {
        if (inEditMode) {
           tmpLoc.x = defLoc.x;
           tmpLoc.y = defLoc.y;
        }
        else {
           tmpLoc.x = curLoc.x;
           tmpLoc.y = curLoc.y;
        }
        return tmpLoc;
    }

    public void adjustFont() {
        Font  curFont = null;

        if (curDim.width <= 0)
           return;
        int w = curDim.width;
        int h = curDim.height;
        nWidth = w;
        if (choiceVector == null) {
            rWidth2 = 0;
            return;
        }
        if (w > rWidth2) {
           if (fontRatio >= 1.0f)
              return;
        }
        float s = (float)(w - iconWidth) / (float)(rWidth - iconWidth);
        if (rWidth > w) {
           if (s > 0.98f)
                s = 0.98f;
           if (s < 0.5f)
                s = 0.5f;
        }
        if (s > 1)
                s = 1;
        if (s == fontRatio)
            return;
        fontRatio = s;
        s = (float) rHeight * fontRatio;
        if ((s < 10) && (rHeight > 10))
            s = 10;
        if (fontRatio < 1) {
            if (font2 == null) {
                String fname = font.getName();
                if (!fname.equals("Dialog"))
                    //font2 = new Font("Dialog", font.getStyle(), rHeight);
                    font2 = DisplayOptions.getFont("Dialog", font.getStyle(), rHeight);
                else
                    font2 = font;
            }
            if (s < (float) rHeight)
                s += 1;
            //curFont = font2.deriveFont(s);
            curFont = DisplayOptions.getFont(font2.getName(), font2.getStyle(), (int)s);
        }
        else
            curFont = font;
        w -= iconWidth;
        int fh = 0;
        String strfont = curFont.getName();
        int nstyle = curFont.getStyle();
        while (s > 10) {
            FontMetrics fm2 = labelWidget.getFontMetrics(curFont);
            rWidth2 = 0;
            String str;
            int len = 0;
            int count = choiceVector.size();
            for (int k = 0; k < count; k++) {
                str = (String) choiceVector.elementAt(k);
                if (str != null) {
               	    len = fm2.stringWidth(str);
                    if (len > rWidth2)
                        rWidth2 = len;
                }
            }
            fh = curFont.getSize();
            if ((rWidth2 < w) && (fh < h))
                break;
            s = s - 0.5f;
            if (s < 10)
                break;
            //curFont = curFont.deriveFont(s);
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
        }
        if (rWidth2 > nWidth)
            rWidth2 = nWidth;
        labelWidget.setFont(curFont);
    }

}

