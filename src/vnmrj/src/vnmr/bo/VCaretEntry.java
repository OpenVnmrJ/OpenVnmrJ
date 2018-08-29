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
import javax.swing.text.*;
import javax.swing.event.*;
import javax.swing.plaf.*;
import javax.swing.border.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VCaretEntry extends JTextField
    implements VEditIF, VObjIF, VObjDef, DropTargetListener,
    PropertyChangeListener, CaretListener
{
	private int caretValue=0;
	private String caretQuery=null;
    private String caretCmd = null;
    private String caretColor = null;

	private boolean debug=false;

    private String type = null;
    private String label = null;
    private String value = null;
    private String precision = null;
    private String fg = null;
    private String bg = null;
    private String vnmrVar = null;
    private String vnmrCmd = null;
    private String showVal = null;
    private String setVal = null;
    private String fontName = null;
    private String fontStyle = null;
    private String fontSize = null;
    private String m_strDisAbl = null;
    private Color  fgColor = null;
    private Color  bgColor, orgBg;
    private Font   font = null;
    private int isActive = 1;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private boolean isEnabled = true;
    private String keyStr = null;
    private String valStr = null;
    private boolean inModalMode = false;
    private DefaultCaret caret;

    private int nHeight = 0;
    private int rHeight = 90;
    private int fontH = 0;
    private double xRatio = 1.0;
    private double yRatio = 1.0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);


    public VCaretEntry(SessionShare sshare, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        this.fg = "black";
        this.fontSize = "8";
        setText("");
        setOpaque(false);
        orgBg = VnmrRgb.getColorByName("darkGray");
        setHorizontalAlignment(JTextField.LEFT);
        setMargin(new Insets(0, 2, 0, 2));

        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2)
                        ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                }
            }
        };
        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
    	setEditable(true);
    	caret=new VCaret();
    	setCaret(caret);
    	addCaretListener(this);
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

    //============= PropertyChangeListener interface ===============

    public void propertyChange(PropertyChangeEvent evt){
        if(fg!=null){
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        changeFont();
     }

    //============= CaretListener interface =======================

	public void caretUpdate(CaretEvent e) {
        int dot = e.getDot();
        int mark = e.getMark();
		if (dot == mark) {  // no selection
		    if(caretValue!=dot){
			    caretValue=dot;
		    }
		    if(debug && caretCmd != null)
			    System.out.println("caret @: "+caretValue);
			if(hasFocus()) {
				// Update widget and vnmr variables every time something
				// happens in the widget
				sendTextCmd();
				sendCaretCmd();
			}
			return;
        }
	}

    //============= VEditIF interface ==============================

    private final static String[] m_arrStrDisAbl = {"Grayed out", "Label" };

    /** The array of the attributes that are displayed in the edit template.*/
    private final static Object[][] m_attributes = {
	{new Integer(VARIABLE),	"Vnmr variables:    "},
	{new Integer(SETVAL),   "Text query:"},
	{new Integer(CMD),      "Text command:"},
	{new Integer(SETVAL2),  "Caret query:"},
	{new Integer(CMD2),     "Caret command:"},
	{new Integer(SHOW),	    "Enable condition:"},
	{new Integer(NUMDIGIT), "Decimal Places:"},
	{new Integer(BGCOLOR),  "Background color:","color"},
	{new Integer(COLOR2),   "Caret color:","color"},
	{new Integer(DISABLE),  "Disable style:", "menu", m_arrStrDisAbl}
    };

    /**
     *  Returns the attributes array object.
     */
    public Object[][] getAttributes()
    {
	     return m_attributes;
    }

    //============= VObjIF interface ==============================

    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
 			return type;
		case LABEL:
            return null;
        case VALUE:
		    value = getText();
		    if (value != null)
			    value = value.trim();
            return value;
        case FGCOLOR:
            return fg;
        case BGCOLOR:
            return bg;
        case COLOR2:
            return caretColor;
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
        case CMD:
            return vnmrCmd;
        case CMD2:
            return caretCmd;
        case SETVAL:
             return setVal;
 	    case SETVAL2:
		    return caretQuery;
        case NUMDIGIT:
            return precision;
        case KEYSTR:
             return keyStr;
        case KEYVAL:
            return keyStr==null ? null: getText();
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
		switch (attr) {
        case TYPE:
            type = c;
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            repaint();
            break;
        case BGCOLOR:
            if(!DisplayOptions.isOption(DisplayOptions.COLOR,c))
                c=null;
            bg = c;
            if (c != null) {
                bgColor = DisplayOptions.getColor(c);
                setOpaque(true);
            }
            else {
                bgColor = Util.getBgColor();
                if (isActive < 1)
                    setOpaque(true);
                else
                    setOpaque(inEditMode);
            }
            setBackground(bgColor);
            repaint();
            break;
        case COLOR2:
            caretColor=c;
            if(DisplayOptions.isOption(DisplayOptions.COLOR,c)){
                Color col=DisplayOptions.getColor(c);
                setCaretColor(col);
                setSelectionColor(col);
            }
            break;
        case SHOW:
            showVal = c;
            setBorder(isEnabled);
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
        case SETVAL2:
 		    caretQuery=c;
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case CMD2:
 		    caretCmd=c;
            break;
        case NUMDIGIT:
            precision = c;
            break;
        case LABEL:
            label=c;
            break;
        case VALUE:
            value = c;
            setText(c);
            break;
        case KEYSTR:
            keyStr=c;
            break;
        case KEYVAL:
            setText(c);
            break;
        case DISABLE:
            m_strDisAbl = c;
            break;
        }
    }

    public ButtonIF getVnmrIF() { return vnmrIf;}
    public void setVnmrIF(ButtonIF vif) { vnmrIf = vif;}

    public void updateValue() {
        if(inModalMode)
            return;
        if (vnmrIf == null)
            return;
        if (showVal != null)
            vnmrIf.asyncQueryShow(this, showVal);
        else if (setVal != null)
            vnmrIf.asyncQueryParam(this, setVal);
        if(caretQuery != null)
            vnmrIf.asyncQueryParamNamed("caret", this, caretQuery);
    }

    public void setValue(ParamIF pf) {
        if(pf == null || pf.value == null)
             return;
        if(pf.name.equals("caret"))
       		setCaretValue(pf.value);
        else
            setTextValue(pf.value);
    }

    public void setShowValue(ParamIF pf) {
        int newActive = isActive;
        if (pf != null && pf.value != null) {
             String  s = pf.value.trim();
             newActive = Integer.parseInt(s);
             if (newActive > 0) {
                 setOpaque(inEditMode);
                 if (bg != null) {
                     setOpaque(true);
                     setBackground(bgColor);
                 }
             }
             else {
                 setOpaque(true);
                 if (newActive == 0)
                     setBackground(Global.IDLECOLOR);
                 else
                     setBackground(Global.NPCOLOR);
             }
             if (newActive >= 0) {
                 setEnabled(true);
                 isEnabled=true;
                 if (setVal != null)
                     vnmrIf.asyncQueryParam(this, setVal, precision);
             }
             else {
                 setEnabled(false);
                 isEnabled = false;
             }
             setBorder(isEnabled);
        }
        if (newActive != isActive) {
            isActive = newActive;
            repaint();
        }
    }

	public void sendVnmrCmd() {
        if(vnmrCmd == null || vnmrCmd.length()==0)
		    return;
		vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    public void refresh() { }
    public void  destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
		VObjDropHandler.processDrop(e, this, inEditMode);
    }

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void setDefLabel(String s) {
        this.value = s;
        setText(s);
        setScrollOffset(0);
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        if (s) {
            addMouseListener(ml);
            setOpaque(s);
           if (font != null) {
                setFont(font);
                fontH = font.getSize();
                rHeight = fontH;
           }
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
            if ((bg != null) || (isActive < 1))
                setOpaque(true);
            else
                setOpaque(false);
        }
        inEditMode = s;
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
	fontH = font.getSize();
        rHeight = fontH;
        if (!inEditMode) {
             if ((curDim.height > 0) && (rHeight > curDim.height)) {
                 adjustFont(curDim.width, curDim.height);
             }
        }
        repaint();
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    //============= private methods and classes ==========================

	private String subString(String s, String m, String d){
        String r="";
        StringTokenizer tok= new StringTokenizer(s," \t\':+-*/\"/()=,\0",true);
        while(tok.hasMoreTokens()){
        	String t=tok.nextToken();
        	if(t.equals(m))
        		r+=d;
        	else
        		r+=t;
        }
		return r;
	}

    private void setBorder(boolean isEnabled) {
        JTextField txfTmp = new JTextField();
        if (isEnabled)
            setBorder(txfTmp.getBorder());
        else {
            String strStyle = getAttribute(DISABLE);
            if (strStyle != null && strStyle.equals(m_arrStrDisAbl[1])) {
                setBorder(null);
                setBackground(Util.getBgColor());
            } else  {
                setBorder(txfTmp.getBorder());
                setBackground(Global.NPCOLOR);
            }
        }
    }

	public void sendTextCmd() {
        if(vnmrCmd == null || vnmrCmd.length()==0)
		    return;
		vnmrIf.sendVnmrCmd(this, vnmrCmd);
		if(debug)
		    System.out.println("textCmd: "+vnmrCmd);
    }

	private void sendCaretCmd() {
		if(caretCmd == null || caretCmd.length()==0)
		    return;
		String cmd=caretCmd;
	 	if (cmd.indexOf("$VALUE") >= 0)
   			cmd=subString(caretCmd,"$VALUE",""+caretValue);
		vnmrIf.sendVnmrCmd(this, cmd);
		if(debug)
		    System.out.println("caretCmd: "+cmd);
    }

	private void setCaretValue(String s) {
        try{
           int i=Integer.parseInt(s);
           setCaretValue(i);
        }
        catch(NumberFormatException e){
	   Messages.writeStackTrace(e);
          // System.out.println("invalid caret position "+caretValue);
        }
    }

	private void setCaretValue(int i) {

		caret.setVisible(true);

		caretValue=i;
		int min=0;
		int max=getText().length();

		if(i<min) i=min;
		if(i>max) i=max;

		if(debug)
		    System.out.println("setSelectionStart: "+i);
		setSelectionStart(i);
           setSelectionEnd(i);
     }

	private void setTextValue(String s) {
		if(debug)
		    System.out.println("setTextValue: "+s);
        setText(s);
		setScrollOffset(0);
    }

	class VCaret extends DefaultCaret
	{
	    VCaret(){
	       super();
	       setBlinkRate(500);
	    }
		public void focusLost(FocusEvent e){
		    inModalMode=false;
		    setVisible(true);
		    if(debug)
		    	System.out.println("lost focus");
		    setBlinkRate(500);
 		    setCaretValue(caretValue);
		}
		public void focusGained(FocusEvent e){
		    inModalMode=true;
		    if(debug)
		        System.out.println("gained focus");
		    setBlinkRate(500);
		    //setCaretValue(caretValue);
		    super.focusGained(e);
		}
	}

   public void setSizeRatio(double x, double y) {
        xRatio =  x;
        yRatio =  y;
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
        if (!inEditMode) {
           if ((h != nHeight) || (h < rHeight)) {
              adjustFont(w, h);
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

    public void adjustFont(int w, int h) {
        if (h <= 0)
           return;
        int oldH = rHeight;
        if (font == null) {
            font = getFont();
            fontH = font.getSize();
            rHeight = fontH;
        }
        nHeight = h;
        if (fontH >= h)
            rHeight = h - 2;
        else
            rHeight = fontH;
        if ((rHeight < 10) && (fontH > 10))
            rHeight = 10;
        if (oldH != rHeight) {
            //Font  curFont = font.deriveFont((float) rHeight);
            Font curFont = DisplayOptions.getFont(font.getName(),font.getStyle(),rHeight);
            setFont(curFont);
        }
        if (rHeight > h)
            rHeight = h;
    }

}
