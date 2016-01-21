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
import javax.swing.event.*;
import javax.swing.plaf.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VUpDownButton extends UpDownButton implements VObjIF, VEditIF,
  StatusListenerIF, VObjDef, DropTargetListener, PropertyChangeListener
{
    public String type = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public Color  fgColor = null;
    public Color  bgColor, origBg;
    public Font   font = null;
    private String wraps = String.valueOf(false);


    private String statusParam = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;

    private String vnmrCmd = "";
    private String label = "";
    private String limitPar = null;
    private int value = 0;
    private int incr1 = 1;
    private int incr2 = 10;
    private int incr3 = 100;
    //private int min = Integer.MIN_VALUE;
    //private int max = Integer.MAX_VALUE;
    private int min = -0xfffffff;
    private int max = 0xfffffff;
    private VObjIF minMax = null;
    private boolean inModalMode = false;

    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private float fontH = 10;
    private float fontH2 = 10;
    private int rWidth = 900;
    private int nWidth = 0;
    private int nHeight = 0;
    private double xRatio = 1.0;
    private double yRatio = 1.0;

    // These are static to force all the UpDownButtons to look the same.
    // See also UpDownButton.java
    static private String isPointy = String.valueOf(false);
    static private String isRocker = String.valueOf(true);
    static private String hasArrow = String.valueOf(true);
    static private String arrowColorStr = "0000ff";

    public VUpDownButton(SessionShare sshare, ButtonIF vif, String typ) {
    this.type = typ;
    this.vnmrIf = vif;
    this.fg = "PlainText";
        this.fontSize = "8";
    this.fontStyle = "Bold";
    this.label = " ";
    this.value = 0;
    minMax = new MinMax();
    origBg = getBackground();
    bgColor = Util.getBgColor();
    setBackground(bgColor);
    setOpaque(false);

    getDataModel().addChangeListener(new ChangeListener() {
            public void stateChanged(ChangeEvent evt) {
                dataChange(evt);
            }
        });

    // Don't clip values before they're set for real:
    setMinimum(min);
    setMaximum(max);

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
    DisplayOptions.addChangeListener(this);
    new DropTarget(this, this);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
       if(fg!=null){
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        bgColor = Util.getBgColor();
        setBackground(bgColor);
        changeFont();
    }

    /*
    protected void processEvent(AWTEvent e) {
    String str = e.toString();
    String msg = str.substring(str.indexOf('['), str.indexOf(']')+1);
    System.out.println("processEvent("+msg+")");
    super.processEvent(e);
    }
    */

    private void dataChange(ChangeEvent  e) {
    int i;
    if (incr1 != getIncrement() || incr2 != getIncrement(1) || incr3 != getIncrement(2)) {
        incr1 = getIncrement();
        incr2 = getIncrement(1);
        incr3 = getIncrement(2);
        writePersistence();
    }
        if (inModalMode || inEditMode || vnmrIf == null) {
        return;
    }
    if (value == (i = super.getValue())) {
        return;
    }
    value = i;
    if (vnmrCmd != null) {
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
    }

    public void setDefLabel(String s) {
        this.label = s;
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
    if (bg == null)
       setOpaque(s);
    if (s) {
           addMouseListener(ml);
       if (font != null)
                setFont(font);
           if (defDim.width <= 0)
                defDim = getPreferredSize();
       curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
           curDim.width = defDim.width;
           curDim.height = defDim.height;
       xRatio = 1.0;
       yRatio = 1.0;
    }
        else {
           removeMouseListener(ml);
       if (s != inEditMode)
        adjustFont();
    }
    inEditMode = s;
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
    fontH = (float) font.getSize();
    fontH2 = fontH;
        if (!inEditMode) {
             if (curDim.width > 0) {
                 adjustFont();
             }
        }

        repaint();
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    public static String getStaticAttribute(int attr) {
        switch (attr) {
      case POINTY:
        return isPointy;
      case ROCKER:
        return isRocker;
      case ARROW:
        return hasArrow;
      case ARROW_COLOR:
        return arrowColorStr;
      default:
        return null;
        }
    }

    public String getAttribute(int attr) {
        switch (attr) {
          case TYPE:
                     return type;
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
          case VARIABLE:
                     return vnmrVar;
          case VAR2:
                     return limitPar;
          case SETVAL:
                     return setVal;
          case CMD:
                     return vnmrCmd;
      case VALUE:
             return String.valueOf(value);
      case LABEL:
                 return label;
      case MIN:
                 return limitPar == null ? String.valueOf(min) : null;
      case MAX:
                 return limitPar == null ? String.valueOf(max) : null;
      case STATPAR:
                 return statusParam;
          case WRAP:
                     return wraps;
      case POINTY:
      case ROCKER:
      case ARROW:
      case ARROW_COLOR:
                 return getStaticAttribute(attr);
          default:
                    return null;
        }
    }

    public static void setStaticAttribute(int attr, String c) {
        switch (attr) {
          case POINTY:
        isPointy = c;
        setPointy(Boolean.valueOf(c).booleanValue());
        break;
          case ROCKER:
        isRocker = c;
        setRocker(Boolean.valueOf(c).booleanValue());
        break;
          case ARROW:
        hasArrow = c;
        setArrow(Boolean.valueOf(c).booleanValue());
        break;
          case ARROW_COLOR:
        arrowColorStr = c;
        Color color = null;
        try {
        int icolor = Integer.parseInt(c, 16);
        color = new Color(icolor);
        } catch (NumberFormatException exception) {}
        if (color != null) {
        setArrowColor(color);
        }
        break;
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
          case TYPE:
                     type = c;
                     break;
          case FGCOLOR:
                     fg = c;
             if (fg != null) {
                     fgColor=DisplayOptions.getColor(c);
             setForeground(fgColor);
             } else {
                     setForeground(Color.black);
             }
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
          case CMD:
                     vnmrCmd = c;
                     break;
          case VALUE:
                 try {
             value = (int)Double.parseDouble(c);
             super.setValue(value);
             } catch (NumberFormatException ex) { }
                     break;
          case SETINCREMENTS:
                 try {
             value = (int)Double.parseDouble(c);
             getDataModel().setIncrementIndex(value);
             } catch (NumberFormatException ex) { }
                     break;
          case VARIABLE:
                     vnmrVar = c;
             readPersistence();
                     break;
          case VAR2:
            Messages.postDebug("shimButton",
                               "limitPar=" + c + ", minMax=" + minMax);
             if (c != null)
            c = c.trim();
                     limitPar = c;
             if (minMax != null && limitPar != null) {
             vnmrIf.asyncQueryMinMax(minMax, limitPar);
             }
                     break;
          case LABEL:
                     label = c;
             setLabel(label);
                     break;
          case MIN:
                 if (limitPar == null) {
             try {
                 min =  (int)Double.parseDouble(c);
             } catch (NumberFormatException ex) { }
             setMinimum(min);
             }
                     break;
          case MAX:
                 if (limitPar == null) {
             try {
                 max = (int)Double.parseDouble(c);
             } catch (NumberFormatException ex) { }
             setMaximum(max);
             }
                     break;
          case STATPAR:
                     statusParam = c;
             updateStatus(ExpPanel.getStatusValue(statusParam));
             readPersistence();
                     break;
          case WRAP:
                     wraps = c;
                     setWrapping(Boolean.valueOf(c).booleanValue());
                     break;
          case POINTY:
          case ROCKER:
          case ARROW:
          case ARROW_COLOR:
             setStaticAttribute(attr, c);
                     break;
        }
    repaint();
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

    public ButtonIF getVnmrIF() {
    return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
    vnmrIf = vif;
    }

    public void updateValue() {
    if (vnmrIf == null)
        return;
        if (setVal != null) {
        vnmrIf.asyncQueryParam(this, setVal);
        }
        if (showVal != null) {
        vnmrIf.asyncQueryShow(this, showVal);
        }
    }

    public void updateStatus(String msg) {
        if (msg == null || msg.length() == 0 || msg.equals("null") ||
            statusParam == null) {
            return;
        }
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals(statusParam)) {
                String vstring = tok.nextToken();
                try {
                    value = (int)Double.parseDouble(vstring);
                } catch (NumberFormatException ex) {
                    value = 0;
                }
                super.setValue(value);
            }
        }
    }

    public void setValue(ParamIF pf) {
    if (pf != null) {
        try {
        value = (int)Double.parseDouble(pf.value);
        super.setValue(value);
        } catch (NumberFormatException ex) { }
    }
    }

    public void setShowValue(ParamIF pf) {}
    public void refresh() {}
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

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
        curDim.width = w;
        curDim.height = h;
        curLoc.x = x;
        curLoc.y = y;
    if (!inEditMode) {
           if ((w != nWidth) || (h != nHeight)) {
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

    public void setSizeRatio(double x, double y) {
        double newX =  x;
        double newY =  y;
    if ((x == xRatio) && (y == yRatio))
        return;
    xRatio = x;
    yRatio = y;
        if (x > 1.0)
            newX = x - 1.0;
        if (y > 1.0)
            newY = y - 1.0;

    if (defDim.width <= 0)
       defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * newX);
        curLoc.y = (int) ((double) defLoc.y * newY);
        curDim.width = (int) ((double)defDim.width * newX);
        curDim.height = (int) ((double)defDim.height * newY);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }

    public void adjustFont() {
        Font  curFont = null;

        if (curDim.height <= 6)
            return;
        if ((nWidth == curDim.width) && (nHeight == curDim.height))
            return;
        if (font == null) {
            font = getFont();
            if (font == null)
                return;
            fontH = (float) font.getSize();
        }
        nWidth = curDim.width;
        nHeight = curDim.height;
        float s = (float) (curDim.height / 2);
        if (s > fontH)
            s = fontH;
        if (s < 8)
            s = 8;
        if (nWidth >= rWidth) {
           if (s == fontH2) {
              return;
          }
        }
        String strfont = font.getName();
        int nstyle = font.getStyle();
        //curFont = font.deriveFont(s);
        curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
        while (s >= 8) {
            FontMetrics fm2 = getFontMetrics(curFont);
            rWidth = 4;
            if (label != null) {
                rWidth = fm2.stringWidth(label);
            }
            String str = getIncrementStr();
            if (str != null) {
                 rWidth += fm2.stringWidth(str);
                 rWidth += fm2.stringWidth(" +");
             }
            if (rWidth < nWidth)
                 break;
            if (s < 9)
                 break;
            s--;
            //curFont = curFont.deriveFont(s);
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
        }
        if (rWidth > nWidth)
            rWidth = nWidth;
        fontH2 = (float)curFont.getSize();
        setFont(curFont);
    }



    public void dragEnter(DropTargetDragEvent e) {
        VObjDropHandler.processDragEnter(e, this, inEditMode);
    }
    public void dragExit(DropTargetEvent e) {
        VObjDropHandler.processDragExit(e, this, inEditMode);
    }
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, inEditMode);
    }

    public Object[][] getAttributes()
    {
    return attributes;
    }

    class MinMax extends VObjAdapter {
    // Gets "setValue" messages for parameter limits
    public void setValue(ParamIF pf) {
        if (pf != null) {
                Messages.postDebug("shimButton",
                                   "MinMax.setValue(" + pf.value + ")");
        StringTokenizer tok = new StringTokenizer(pf.value);
        if (tok.countTokens() < 4) {
            Messages.postError("VUpDownButton.MinMax.setValue(): "+
                       "not enough tokens: \""+
                       pf.value+"\"");
            return;
        }
        tok.nextToken(); // Toss the "m"
        String mstr;
        try {
            mstr = tok.nextToken();
            max = (int)Double.parseDouble(mstr);
            mstr = tok.nextToken();
            min = (int)Double.parseDouble(mstr);
            setMinimum(min);
            setMaximum(max);
                    Messages.postDebug("shimButton",
                                       "MinMax.setValue()" + ": min=" + min
                                       + ", max=" + max);
            // Ignore the last token (increment)
            //validate(); /* not needed? */
            //repaint(); /* not needed? */
        } catch (NumberFormatException ex) {
                    Messages.postError("VUpDownButton.MinMax(): "
                                       + "Bad number format: "
                                       + pf.value);
                }
        }
    }
    }

    private final static String[] true_false = {"true", "false"};
    private final static Object[][] attributes = {
    {new Integer(VARIABLE),	"Vnmr Variables:"},
    {new Integer(SETVAL),	"Value of item:"},
    {new Integer(LABEL),	"Label:"},
    {new Integer(CMD),	"Vnmr command:"},
    {new Integer(STATPAR),	"Status parameter:"},
    {new Integer(VAR2),	"Limits parameter:"},
    {new Integer(MIN),	"Min allowed value:"},
    {new Integer(MAX),	"Max allowed value:"},
    {new Integer(POINTY),	"Pointy style:", "menu", true_false},
    {new Integer(ROCKER),	"Rocker style:", "menu", true_false},
    {new Integer(ARROW),	"Arrow feedback:", "menu", true_false},
    {new Integer(ARROW_COLOR),	"Arrow color:", "color"},
    {new Integer(WRAP),	"Values wrap around:", "menu", true_false},
    };

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
        value = super.getValue();
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    private String getKey() {
    String key = null;
    StringTokenizer tok;
    if (vnmrVar != null
        && (tok = new StringTokenizer(vnmrVar)).hasMoreTokens())
    {
        key = tok.nextToken();
    } else if (statusParam != null
        && (tok = new StringTokenizer(statusParam)).hasMoreTokens())
    {
        key = tok.nextToken();
    }
    return key;
    }

    private void writePersistence() {
    String key = getKey();
    if (key == null) {
        return;
    }
    String filepath;
    filepath = FileUtil.savePath("USER/PERSISTENCE/UpDownProperties");
    String line;
    StringTokenizer tok;
    StringBuffer outStr = new StringBuffer(key+" "+incr1+" "+incr2+" "+incr3+"\n");
    try {
        BufferedReader in = new BufferedReader(new FileReader(filepath));
        while ((line = in.readLine()) != null) {
        tok = new StringTokenizer(line, " \t");
        if (!tok.hasMoreTokens() || !tok.nextToken().equals(key)) {
            outStr.append(line);
            outStr.append("\n");
        }
        }
            in.close();
    } catch(IOException er) {}

    // Delete extra trailing newlines
    int i;
    for (i=outStr.length()-1; outStr.charAt(i)=='\n'; --i);
    outStr.delete(i+1, outStr.length());

    // Write out new file
    try {
        PrintWriter out = new PrintWriter(new FileWriter(filepath));
        out.println(outStr.toString());
        out.close();
    } catch(IOException er) {
        System.out.println("VUpDownButton: error writing persistence file");
    }
    }

    private void readPersistence() {
        String key = getKey();
        if (key == null) {
            return;
        }
        // Set defaults for special cases
        if (key.equals("lockpower") || key.equals("lockgain")) {
            incr1 = 1;
            incr2 = 5;
            incr3 = 10;
        }
        String filepath;
        filepath = FileUtil.savePath("USER/PERSISTENCE/UpDownProperties");
        String line;
        StringTokenizer tok;
        try {
            BufferedReader in = new BufferedReader(new FileReader(filepath));
            while ((line = in.readLine()) != null) {
                tok = new StringTokenizer(line, " \t");
                if (tok.hasMoreTokens() && tok.nextToken().equals(key)) {
                    if (tok.hasMoreTokens()) {
                        incr1 = (int)Double.parseDouble(tok.nextToken());
                    }
                    if (tok.hasMoreTokens()) {
                        incr2 = (int)Double.parseDouble(tok.nextToken());
                    }
                    if (tok.hasMoreTokens())
                    {
                        incr3 = (int)Double.parseDouble(tok.nextToken());
                    }
                    break;
                }
            }
            in.close();
        } catch(IOException e) { }
        setIncrements(incr1, incr2, incr3);
    }
}

