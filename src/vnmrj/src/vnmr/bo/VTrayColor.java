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
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import java.beans.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VTrayColor extends JComponent implements VObjIF, VObjDef,
	   PropertyChangeListener
{
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private JLabel   toolName;
    private VColorChooser colorDialog;
    public JTextField tipText;
    public String bg = null;
    public String colorStr = null;
    public String fg = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String labelVal = null;
    public String nameVal = null;
    public String nameStr = null;
    public String setVal = null;
    public String showVal = null;
    public String tipStr = null;
    public String type = null;
    public String vnmrCmd = null;
    public String vnmrVar = null;

    public Color  fgColor = Color.black;
    public Color  bgColor, orgBg;
    public Font   font = null;

    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Dimension tmpDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);

    private final static Object[][] m_attributes = {
        {new Integer(PANEL_NAME), Util.getLabel("Name of item:") },
        {new Integer(SHOW),     Util.getLabel(SHOW) },
        {new Integer(SETVAL), Util.getLabel("Color:") },
        {new Integer(TOOLTIP), Util.getLabel(TOOLTIP) }
    };


    public VTrayColor(SessionShare sshare, ButtonIF vif, String typ) {
	this.vnmrIf = vif;
	this.type = typ;
        this.toolName = new JLabel();
        this.tipText = new JTextField("");
        this.colorDialog = new VColorChooser(sshare, vif, typ);

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
        setLayout(new BorderLayout());
        add(this.colorDialog, BorderLayout.EAST);
        add(this.toolName, BorderLayout.WEST);
        add(this.tipText, BorderLayout.CENTER);
	DisplayOptions.addChangeListener(this);
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
         if (bg != null)
             bgColor=DisplayOptions.getColor(bg);
         else
             bgColor = Util.getBgColor();
         setBackground(bgColor);
         if (fg!=null)
            fgColor=DisplayOptions.getColor(fg);
         toolName.setForeground(fgColor);
         tipText.setForeground(fgColor);
         changeFont();
    }

    public void destroy() {
    }

    public void setDefLabel(String s) {
    }

    public void setDefColor(String c) {
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
	setOpaque(s);
	if (s) {
           addMouseListener(ml);
           curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
	   defDim = getPreferredSize();
           curDim.width = defDim.width;
           curDim.height = defDim.height;
        }
        else
           removeMouseListener(ml);
	inEditMode = s;
    }

    public void changeFont() {
        font = DisplayOptions.getFont(fontName,fontStyle,fontSize);
        tipText.setFont(font);
        toolName.setFont(font);

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
              return labelVal;
          case VALUE:
              colorStr = colorDialog.getAttribute(VObjDef.VALUE);
              return null;
          case SHOW:
              return showVal;
          case CMD:
              return vnmrCmd;
          case VARIABLE:
              return vnmrVar;
          case SETVAL:
              colorStr = colorDialog.getAttribute(VObjDef.VALUE);
              return colorStr;
          case TOOL_TIP:
              return null;
          case TOOLTIP:
              // tipStr = tipText.getText().trim(); 
              return tipStr;
          case PANEL_NAME:
              return nameVal;
          case FONT_NAME:
              return fontName;
          case FONT_STYLE:
              return fontStyle;
          case FONT_SIZE:
              return fontSize;
          case FGCOLOR:
              return fg;
          case BGCOLOR:
              return bg;
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
              labelVal = c;
              break;
          case VALUE:
              colorStr = c;
              if (c != null)
                 colorDialog.setAttribute(VObjDef.VALUE, c);
              break;
          case SHOW:
              showVal = c;
              break;
          case CMD:
              vnmrCmd = c;
              break;
          case VARIABLE:
              vnmrVar = c;
              break;
          case SETVAL:
              colorStr = c;
              if (c != null)
                 colorDialog.setAttribute(VObjDef.VALUE, c);
              break;
          case TOOL_TIP:
              tipStr = c;
              if (c != null)
                 tipText.setText(Util.getLabel(c));
              break;
          case TOOLTIP:
              tipStr = c;
              if (c != null)
                 tipText.setText(Util.getLabel(c));
              break;
          case PANEL_NAME:
              nameVal = c;
              if (c != null) {
                 nameStr = Util.getLabel(c);
                 toolName.setText(nameStr);
              }
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
          case FGCOLOR:
              fg = c;
              break;
          case BGCOLOR:
              bg = c;
              break;
       }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void setValue(ParamIF pf) {
    }

    public void setShowValue(ParamIF pf) {
        if (pf == null || pf.value == null)
            return;
        String  s = pf.value.trim();
        int isActive = Integer.parseInt(s);
        if (isActive > 0) {
             setOpaque(inEditMode);
             if (bg != null) {
                  setOpaque(true);
             }
             if (bgColor != null) {
                  setBackground(bgColor);
             }
        }
        else {
             setOpaque(true);
             if (isActive == 0)
                 setBackground(Global.IDLECOLOR);
             else
                 setBackground(Global.NPCOLOR);
        }
    }

    public void updateValue() {
        if (vnmrIf == null) {
            vnmrIf = (ButtonIF)Util.getActiveView();
            if (vnmrIf == null)
                return;
        }
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
	Dimension  psize = getPreferredSize();
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

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

    public void setSizeRatio(double x, double y) {
	double rx = x;
	double ry = y;
        if (rx > 1.0)
            rx = x - 1.0;
        if (ry > 1.0)
            ry = y - 1.0;
	if (defDim.width <= 0)
	    defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * rx);
        curLoc.y = (int) ((double) defLoc.y * ry);
        curDim.width = (int) ((double)defDim.width * rx);
        curDim.height = (int) ((double)defDim.height * ry);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
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

    public void reshape(int x, int y, int w, int h) {
        if (inEditMode) {
           defLoc.x = x;
           defLoc.y = y;
           defDim.width = w;
           defDim.height = h;
        }
        curLoc.x = x;
        curLoc.y = y;
        curDim.width = w;
        curDim.height = h;
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

    public Dimension getItemPreferredSize(int item) {
        Dimension dim = new Dimension(0, 0);
        switch (item) {
            case 1:
                   dim = toolName.getPreferredSize();
                   break;
            case 2:
                   dim = tipText.getPreferredSize();
                   break;
            case 3:
                   dim = colorDialog.getPreferredSize();
                   break;
        }
        return dim;
    }

    public void setItemPreferredSize(int item, Dimension dim) {
        switch (item) {
            case 1:
                   toolName.setPreferredSize(dim);
                   break;
            case 2:
                   tipText.setPreferredSize(dim);
                   break;
            case 3:
                   colorDialog.setPreferredSize(dim);
                   break;
        }
    }

    public VColorChooser getColorChooser() {
        return colorDialog;
    }

    public Object[][] getAttributes()
    {
         return m_attributes;
    }
}

