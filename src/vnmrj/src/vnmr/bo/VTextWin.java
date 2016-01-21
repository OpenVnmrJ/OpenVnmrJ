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
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VTextWin extends JTextArea
    implements VObjIF, VObjDef, DropTargetListener, PropertyChangeListener,
               ActionComponent
{
    static private final int JEXECARRAYVAL = 28;

    public String type = null;
    public String value = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String vnmrCmd2 = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String editable = null;
    public String objName = null;
    public Color  fgColor = Color.black;
    public Color  bgColor, orgBg;
    public Font   font = null;
    protected String wrap = "yes";
    protected String wrapUnit = "char";
    protected String actionCmd = null;
    protected String parameter = null;

    private boolean isEditing = false;
    private boolean inEditMode = false;
    private FocusAdapter fl;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private VObjIF pobj;
    private Component pcomp;
    private DragSource dragSource;
    private DragGestureRecognizer dragRecognizer;
    private DragGestureListener dragListener;
    private boolean inModalMode = false;
    private VTextUndoMgr undo=null;

    private float fontRatio = 1;
    private Point defLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private double curXRatio = 1.0;
    private Set<ActionListener> m_actionListenerList
        = new TreeSet<ActionListener>();

    public VTextWin(VObjIF p, SessionShare sshare, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        this.pobj = p;
        this.pcomp = (Component) p;
        this.fg = "black";
        this.fontSize = "8";
        setText(" ");
        setEditable(false);
        setOpaque(false);
        orgBg = getBackground();
        setMargin(new Insets(2, 2, 2, 2));
        setLineWrap(true);
        setWrapStyleWord("word".equals(wrapUnit));

        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        ParamEditUtil.setEditObj(pobj);
                        ParamEditUtil.setEditObj2((VObjIF) evt.getSource());
                    }
                }
            }
        };
        fl = new FocusAdapter() {
            public void focusLost(FocusEvent evt) {
                focusLostAction();
            }
            public void focusGained(FocusEvent evt) {
                focusGainedAction();
            }
        };
        addFocusListener(fl);
        DisplayOptions.addChangeListener(this);
        new DropTarget(this, this);
/*
        dragSource = new DragSource();
        dragListener = new VObjDragListener(dragSource);
        dragRecognizer = dragSource.createDefaultDragGestureRecognizer(null,
                          DnDConstants.ACTION_COPY, dragListener);
*/
    }

    // PropertyChangeListener interface

    private void setFgColor() {
        if (fg == null || fg.length() < 1 || fg.equals("default"))
             fgColor = UIManager.getColor("TextArea.foreground");
        else
             fgColor = DisplayOptions.getColor(fg);
        setForeground(fgColor);
    }

    private void setBgColor() {
        if (bg == null || bg.length() < 1 ||
                        bg.equals("transparent")||bg.equals("VJBackground"))
            bgColor = UIManager.getColor("TextArea.background");
        else
            bgColor = DisplayOptions.getColor(bg);
        setBackground(bgColor);
    }


    public void propertyChange(PropertyChangeEvent evt){
        setFgColor();
        setBgColor();
        changeFont();
    }

    public FocusAdapter getFocusListener() { return fl; }

    public Component getContainer() {
        return (Component) pobj;
    }

    public void setDefLabel(String s) {
        this.value = s;
        setText(s);
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
/*
        if (s) {
           dragRecognizer.setComponent(this);
        }
        else {
           dragRecognizer.setComponent(null);
        }
*/
        repaint();
    }

    public void setEditMode(boolean s) {
        if (bg == null)
           setOpaque(s);
        if (s) {
           addMouseListener(ml);
           setFont(font);
           fontRatio = 1;
        }
        else
           removeMouseListener(ml);
        inEditMode = s;
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        fontRatio = 1;
        if (!inEditMode) {
           if (curXRatio < 1.0)
             adjustFont((float) curXRatio);
        }
        repaint();
    }

    public void changeFocus(boolean s) {
    }

    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
        case LABEL:
            return null;
        case VALUE:
            return value;
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
        case SETVAL:
            return setVal;
        case VARIABLE:
            return vnmrVar;
        case EDITABLE:
            return editable;
        case CMD:
            return vnmrCmd;
        case WRAP:
            return wrap;
        case UNITS:
            return wrapUnit;
        case ACTIONCMD:
            return actionCmd;
        case PANEL_PARAM:
            return parameter;
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
            break;
        case VALUE:
            value = c;
            setText(c);
            break;
        case FGCOLOR:
            fg = c;
            setFgColor();
            repaint();
            break;
        case BGCOLOR:
            bg = c;
            if (c != null) {
                setOpaque(true);
            }
            else {
                setOpaque(inEditMode);
            }
            setBgColor();
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
        case VARIABLE:
            vnmrVar = c;
            break;
        case EDITABLE:
            if (c.equals("yes") || c.equals("true")){
                setEditable(true);
                editable = "yes";
            }
            else {
                setEditable(false);
                editable = "no";
            }
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case WRAP:
            if (c.equals("yes") || c.equals("true")) {
                setLineWrap(true);
                wrap = "yes";
            } else {
                setLineWrap(false);
                wrap = "no";
            }
            break;
        case UNITS:
            wrapUnit = c;
            setWrapStyleWord("word".equals(wrapUnit));
            break;
        case ACTIONCMD:
            actionCmd = c;
            break;
        case PANEL_PARAM:
            parameter = c;
            break;
        case ENABLED:
/*
            setEditable(c.equals("false")?false:true);
*/
            break;
        case PANEL_NAME:
            objName = c;
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
        if (pf != null) {
            value = pf.value;
            if (setVal != null && setVal.startsWith("ARRAY ")
                && value != null && value.startsWith("s "))
            {
                value = valueArrayToText(value);
            }
            setText(value);
            setCaretPosition(0);
        }
    }

    /**
     * Concatenate all the array elements returned by a JEXECARRAYVAL
     * request to Vnmr.
     * These strings look something like
     * <pre>
     * s 3 First array element; second element; I am the last element
     * </pre>
     * Escaped characters are also converted back into
     * their normal Unicode values.
     * @param in The string returned by the JEXECARRAYVAL request.
     */
    private static String valueArrayToText(String in) {
        String value = "";
        StringTokenizer toker = new StringTokenizer(in);
        toker.nextToken(); // Throw away "s"
        int nelems = Integer.parseInt(toker.nextToken());
        if (toker.hasMoreTokens()) {
            in = toker.nextToken("").trim(); // Remainder of string

            // Concatenate elements into one string
            toker = new StringTokenizer(in, ";");
            if (toker.countTokens() == nelems) {
                value = in.replaceAll("; ", "");
            }

            // Translate escapes back to Unicode characters.
            // Note that these are terminated by "." instead of ";".
            int k = value.lastIndexOf("&#");
            if (k > 0) {
                StringBuffer sbValue = new StringBuffer(value);
                for ( ; k > 0; k = sbValue.lastIndexOf("&#", k - 1)) {
                    int j = sbValue.indexOf(".", k);
                    int i = k + 2;
                    if (j > i) {
                        int chr = Integer.parseInt(sbValue.substring(i, j));
                        sbValue.replace(k, j + 1, String.valueOf((char)chr));
                    }
                }
                value = sbValue.toString();
            }
        }
        return value;
    }

    /**
     * Puts as much as possible of the given text into the given parameter.
     * The parameter is arrayed as necessary to
     * fit in the maximum amount of text, and special characters are
     * escaped.
     * @param param The name of the parameter in which to store the text.
     * @param text The text to store.
     * @return A MAGICAL command to set the parameter's value.
     */
    private static String constructVnmrArrayCmd(String param, String text) {
        final int elemsize = 255; // Max string length Vnmr can handle
        final int cmdsize = 8191; // Max command length Vnmr can handle

        StringBuffer sbCmd = new StringBuffer(param).append("=");

        // Substitute escapes for special characters
        // Use &#nn. instead of &#nn; because ";" is the separator
        // used to return the list of array element values.
        StringBuffer sbText = new StringBuffer(text);
        boolean isSpecialChars = false;
        for (int i = sbText.length() - 1; i >= 0; --i) {
            char ch = sbText.charAt(i);
            if (ch == '&' || ch == '\'' || ch == '\\' || ch == ';'
                || ch < 32 || ch >= 127)
            {
                sbText.replace(i, i+1, "&#" + (int)ch + ".");
                isSpecialChars = true;
            }
        }
        if (isSpecialChars) {
            text = sbText.toString();
        }
        
        int len = text.length();
        String separator = "";
        for (int i = 0; i < len; i += elemsize) {
            // Adjust "len" if necessary so we don't make the command too long
            int spaceRemaining = cmdsize - sbCmd.length();
            int punctuationChars = 3; // the ,'' characters
            len = Math.min(len, i + spaceRemaining - punctuationChars);
            String elem = text.substring(i, Math.min(i + elemsize, len));
            sbCmd.append(separator).append("'").append(elem).append("'");
            separator = ",";
        }
        return sbCmd.toString();
    }

    public void setShowValue(ParamIF pf) {
    }

    public void updateValue() {
        if (vnmrIf == null)
            return;
        if (setVal != null) {
            if (setVal.startsWith("ARRAY ")) {
                vnmrIf.asyncQueryARRAY(this,
                                       JEXECARRAYVAL,
                                       setVal.substring(6));
            } else {
                vnmrIf.asyncQueryParam(this, setVal);
            }
        }
        if (showVal != null) {
        }
    }

    private void focusLostAction() {
        if (inModalMode || inEditMode)
            return;
        String strvalue = getText();
        if (strvalue == null)
            strvalue = "";
        if ((vnmrCmd != null && vnmrIf != null) || actionCmd != null) {
            if (!strvalue.equals(value)) {
                value = strvalue;
                sendVnmrCmd();
                sendActionCmd();
            }
        }
        Undo.removeUndoMgr(undo,this);
    }

    private void focusGainedAction() {
        if (inModalMode || inEditMode || vnmrCmd == null || vnmrIf == null)
            return;
            if(undo==null)
                undo=new VTextUndoMgr(this);
            Undo.setUndoMgr(undo,this);
    }

    public void paint(Graphics g) {
        super.paint(g);
    }

    public void refresh() {
    }

    public void destroy() {
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

    /**
     * Called when the contents of the text box have changed to send
     * the appropriate command to Vnmr.
     * If the XML specifies
     * <pre>
     * vc="ARRAY parameter_name"
     * </pre>
     * the parameter "parameter_name" is arrayed as necessary to
     * fit in the maximum amount of text, and special characters are
     * escaped.
     */
    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
        if (vnmrCmd.startsWith("ARRAY ")) {
            StringTokenizer toker = new StringTokenizer(vnmrCmd);
            toker.nextToken();  // Ignore "ARRAY" token
            if (toker.hasMoreTokens()) {
                String param = toker.nextToken();
                vnmrIf.sendVnmrCmd(this, constructVnmrArrayCmd(param, value));
            }
        } else {
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
    }

    /**
     * Send an ActionEvent to all the action listeners, only if the
     * actionCmd has been set.
     */
    private void sendActionCmd() {
        if (actionCmd != null) {
            ActionEvent event = new ActionEvent(this, hashCode(), actionCmd);
            for (ActionListener listener : m_actionListenerList) {
                listener.actionPerformed(event);
            }
        }
    }

    /**
     * Sets the action command to the given string.
     * @param actionCommand The new action command.
     */
    public void setActionCommand(String actionCommand) {
        actionCmd = actionCommand;
    }

    /**
     * Add an action listener to the list of listeners to be notified
     * of actions.
     */
    public void addActionListener(ActionListener listener) {
        m_actionListenerList.add(listener);
    }

    public void setSizeRatio(double x, double y) {
        double x1 =  x;
        double y1 =  y;
        if (x1 > 1.0)
            x1 = x1 - 1.0;
        if (y1 > 1.0)
            y1 = y1 - 1.0;
        if (x1 != curXRatio) {
            curXRatio = x1;
            if (!inEditMode)
                adjustFont((float)x1);
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

    public void adjustFont(float r) {
        if (font == null) {
            font = getFont();
        }
        int rHeight = font.getSize();
        float newH = (float)rHeight * r;
        if (newH < 10)
            newH = 10;
        if (newH == fontRatio)
            return;
        fontRatio = newH;
        //Font  curFont = font.deriveFont(newH);
        Font curFont = DisplayOptions.getFont(font.getName(),font.getStyle(),(int)newH);
        setFont(curFont);
    }
}

