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
import javax.swing.text.*;
import javax.swing.border.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VEntry extends JTextField
    implements VEditIF, VObjIF, VObjDef, DropTargetListener,
               PropertyChangeListener, StatusListenerIF, ActionComponent
{
    public String type = null;
    public String label = null;
    public String value = "";
    public String precision = null;
    public String fg = null;
    public String m_bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String m_strDisAbl = null;
    public String tipStr = null;
    public String objName = null;
    public Color  fgColor = null;
    public Color  bgColor, orgBg;
    public Font   font = null;
    protected String m_strSubtype = null;
    private int m_isActive = 1;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean bFocused = false;
    protected boolean m_bParameter = false;
    protected static boolean m_bMouse = true;
    private MouseAdapter mlEditor;
    private MouseAdapter mlNonEditor;
    private RightsList rightsList=null;
    protected String m_helplink = null;
    private ButtonIF vnmrIf;
    protected String keyStr = null;
    protected String valStr = null;
    private boolean inModalMode = false;
    private VEntry m_objEntry;
    private VTextUndoMgr undo=null;
    protected String m_strKeyWord = null;
    private String statusParam = null;
    protected String m_actionCmd = null;
    protected String m_parameter = null;

    private int nHeight = 0;
    private int rHeight = 90;
    private int fontH = 0;
    private int digits = 0;
    private double xRatio = 1.0;
    private double yRatio = 1.0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private boolean panel_enabled=true;
    // private Set<ActionListener> m_actionListenerList
    //     = new TreeSet<ActionListener>();
    private java.util.List<ActionListener> m_actionListenerList;

    public final static String[] m_arrStrDisAbl = {"Grayed out", "Label" };

    /** The array of the attributes that are displayed in the edit template.*/
    private final static Object[][] m_attributes = {
        {new Integer(VARIABLE), "Vnmr variables:    "},
        {new Integer(SETVAL),   "Value of item:"},
        {new Integer(SHOW),     "Enable condition:"},
        {new Integer(CMD),      "Vnmr command:"},
        {new Integer(NUMDIGIT), "Decimal Places:"},
        {new Integer(DISABLE),  "Disable Style:", "menu", m_arrStrDisAbl},
        {new Integer(STATPAR),  "Status parameter:"},
        {new Integer(TOOLTIP),  Util.getLabel(TOOLTIP) }
    };
    
    private final static Object[][] m_attributes_H = {
        {new Integer(VARIABLE), "Vnmr variables:    "},
        {new Integer(SETVAL),   "Value of item:"},
        {new Integer(SHOW),     "Enable condition:"},
        {new Integer(CMD),      "Vnmr command:"},
        {new Integer(NUMDIGIT), "Decimal Places:"},
        {new Integer(DISABLE),  "Disable Style:", "menu", m_arrStrDisAbl},
        {new Integer(STATPAR),  "Status parameter:"},
        {new Integer(TOOLTIP),  Util.getLabel(TOOLTIP) },
        {new Integer(HELPLINK), Util.getLabel("blHelp")}
    };

    public VEntry(SessionShare sshare, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        this.fg = "black";
        this.fontSize = "12";
        m_objEntry = this;
        setText("");
        setMargin(new Insets(0, 2, 0, 2));
        setBorder();
        setOpaque(true);
        setHorizontalAlignment(JTextField.LEFT);
        super.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent ev) {
                entryAction();
            }
        });

        addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent evt) {
                bFocused = false;
                removeFocusedObj();
                focusLostAction();
            }

            public void focusGained(FocusEvent evt) {
                bFocused = true;
                setFocusedObj();
                focusGainedAction();
            }
        });

        mlEditor = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        if (!m_bParameter)
                            ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                        else
                        {
                            Component comp = ((Component)evt.getSource()).getParent();
                            if (comp instanceof VObjIF)
                                ParamEditUtil.setEditObj((VObjIF)comp);
                        }
                    }
                }
             }
        };

        mlNonEditor = new CSHMouseAdapter();
        addMouseListener(mlNonEditor);

        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
        Caret cr = getCaret();
        if (cr != null)
           cr.setBlinkRate(1000);
    }

    public void setBorder(){
//    	 if(DisplayOptions.LAF.equals("vnmrj"))
//            setBorder(Util.entryBorder());
    }
    /**
     * NB:The background color is now set from the componant's enabled
     *    and editable status using UI attributes set in DisplayOptions.java
     *    
     *    editable:     uses DisplayOptions "VJEntryBGColor"
     *    not editable: uses DisplayOptions "VJInactiveBGColor"
     *    disabled:     uses DisplayOptions "VJDisabledBGColor"
     */
   public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            m_isActive = Integer.parseInt(s);
            if (setVal != null) {
                vnmrIf.asyncQueryParam(this, setVal, precision);
            }

            if(m_isActive >= 0 )
                setEditable(true);
	    else if(m_isActive < 0)
                setEditable(false);

            if(!panel_enabled || m_isActive<0)
                setEnabled(false);
            else
                setEnabled(true);

	    setBackground(m_bg, m_isActive);   

            setFgColor();
            repaint();
         }
    }

    public void setBackground(String bg, int active) {
        setOpaque(true);
        if (active == 0) {
	    bgColor = DisplayOptions.getColor("VJInactiveBG");
            //bgColor = Global.IDLECOLOR;
        } else if (active < 0) {
	    bgColor = DisplayOptions.getColor("VJDisabledBG");
            //bgColor = Global.NPCOLOR;
        }
        else {
            if (bg==null || bg.length()==0 || bg.equals("default")) {
               bgColor = DisplayOptions.getColor("VJEntryBG");
               if (bgColor != null && bgColor.equals(Color.BLACK)) {
                   bgColor = Util.getBgColor();
               }
            } else if (bg.equals("transparent")) {
               bgColor=null;
               setOpaque(false);
            } else {
               bgColor = DisplayOptions.getColor(bg);
            }
        }
        setBackground(bgColor);
        setBorder();
    }

    /**
     * NB:The text color is set as follows:
     *    editable:     uses DisplayOptions "VJEditableTextColor" <set by UI theme>
     *    not editable: uses DisplayOptions "VJInactiveFGColor" <set by UI theme>
     *    disabled:     uses DisplayOptions "VJDisabledFGColor"  <set by UI theme>
     */
    private void setFgColor(){
        Color c=Color.black;
        if(!isEditable())
        	c=DisplayOptions.getColor("VJDisabledFG");
        else if(!isEnabled())
        	c=DisplayOptions.getColor("VJInactiveFG");
        else
        	c=DisplayOptions.getColor("VJEditableText");

        // doesn't work well for some LAFs since background color is not available
    	/*
        Color c=Color.darkGray;
        if(isEditable() && isEnabled()){
            if(fg!=null)
                c=DisplayOptions.getColor(fg);
            Color b = getBackground();
            int fv = c.getRed();
            int bv = b.getRed();
            int d1 = Math.abs(fv - bv);
            if (d1 < 60) {
                int d2 = Math.abs(c.getGreen() - b.getGreen());
                int d3 = Math.abs(c.getBlue() - b.getBlue());
                fv = 0;
                if (d1 > 30)
                  fv++;
                if (d2 > 30) {
                  fv++;
                  if (d2 > 60)
                     fv++;
               }
               if (d3 > 30) {
                  fv++;
                  if (d3 > 60)
                     fv++;
               }
               if (fv < 2) {
                  fv = c.getRed();
                  bv = c.getGreen();
                  if (fv > 120 && bv > 120)
                     c = Color.black;
                  else
                     c = Color.white;
               }
            }
        }
        else if(!isEditable()){
            c=Color.blue;
            if(DisplayOptions.hasColor("VJInactiveFG"))
                c=DisplayOptions.getColor("VJInactiveFG");
        }
        */
        setForeground(c);           
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
        changeFont();
        setFgColor();
        setBorder();
        setBackground(m_bg, m_isActive);
        repaint();
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
           // Be sure both are cleared out
           removeMouseListener(mlNonEditor);
           removeMouseListener(mlEditor);
           addMouseListener(mlEditor);
           setOpaque(true);
           if (font != null) {
                setFont(font);
                fontH = font.getSize();
                rHeight = fontH;
           }
           if (defDim.width <= 0)
                defDim = getPreferredSize();
           curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
           curDim.width = defDim.width;
           curDim.height = defDim.height;
           xRatio = 1.0;
           yRatio = 1.0;
        } else {
            // Be sure both are cleared out
           removeMouseListener(mlEditor);
           removeMouseListener(mlNonEditor);
           addMouseListener(mlNonEditor);

	       setBackground(m_bg, m_isActive);
           isFocused = s;
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
        if (!inEditMode) {
            if (!s && bFocused)
                 focusLostAction();
            return;
        }
        isFocused = s;
        repaint();
    }

    /**
     *  Returns the attributes array object.
     */
    public Object[][] getAttributes()
    {
        VnmrjIF vif = Util.getVjIF();
        if (vif != null)
            rightsList = vif.getRightsList();
      
        if(rightsList != null && rightsList.isApproved("mayedithelpfield"))
            return m_attributes_H;
        else
            return m_attributes;
    }

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
            return m_bg;
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
        case SETVAL:
            return setVal;
        case NUMDIGIT:
            return precision;
        case SUBTYPE:
            return m_strSubtype;
        case KEYSTR:
            return keyStr;
        case KEYVAL:
            return keyStr==null ? null: getText();
        case KEYWORD:
            return m_strKeyWord;
        case DISABLE:
            return m_strDisAbl;
        case STATPAR:
            return statusParam;
        case TOOL_TIP:
            return null;
        case TOOLTIP:
            return tipStr;
        case ACTIONCMD:
            return m_actionCmd;
        case PANEL_PARAM:
            return m_parameter;
        case PANEL_NAME:
            return objName;
        case HELPLINK:
            return m_helplink;
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
            setFgColor();
            break;
        case BGCOLOR:
            m_bg = c;
            setBackground(m_bg, m_isActive);
            repaint();
            break;
        case SHOW:
            showVal = c;
            //setBorder();
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
        case CMD:
            vnmrCmd = c;
            break;
        case NUMDIGIT:
            precision = c;
            digits = 0;
            if (c != null) {
                try {
                    digits = Integer.parseInt(c);
                } catch (NumberFormatException e) { }
            }
            break;
        case SUBTYPE:
            m_strSubtype = c;
            if (c != null && c.equals("parameter"))
                m_bParameter = true;
            else
                m_bParameter = false;
            break;
        case LABEL:
            label=c;
            break;
        case VALUE:
            if (c != null)
                c = c.trim();
            value = c;
            setText(c);
            break;
        case KEYSTR:
            keyStr=c;
            break;
        case KEYVAL:
            setText(c);
            break;
        case KEYWORD:
            m_strKeyWord = c;
            return;
        case DISABLE:
            m_strDisAbl = c;
            break;
        case ENABLED:
            panel_enabled = c.equals("false") ? false : true;
            setEnabled(panel_enabled);
	    setBackground(m_bg, m_isActive);
             break;
        case STATPAR:
            statusParam = c;
            if (c != null)
                updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        case TOOL_TIP:
        case TOOLTIP:
            tipStr = c;
            if (c != null && c.length() > 0)
               setToolTipText(Util.getLabel(c));
            else
               setToolTipText(null);
            break;
        case ACTIONCMD:
            m_actionCmd = c;
            break;
        case PANEL_PARAM:
            m_parameter = c;
            break;
        case PANEL_NAME:
            objName = c;
            break;
        case HELPLINK:
            m_helplink = c;
            break;
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

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void updateStatus(String msg) {
        if (msg == null || msg.length() == 0 || msg.equals("null")) {
            return;
        }
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals(statusParam)) {
                value = tok.nextToken().trim(); // Exclude any units
                if (value.equals("-")) value = "";
                setText(value);
            }
        }
    }

    public void updateValue() {
        if (vnmrIf == null)
            return;
        if (showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
        }
        else if (setVal != null) {
            vnmrIf.asyncQueryParam(this, setVal, precision);
        }
    }

    public void setValue(ParamIF pf) {
        if (pf != null) {
            value = pf.value;
            if (!getText().equals(value)) {
                if (value != null)
                    value = value.trim();
                int k = 0;
                if (hasFocus())
                    k = getSelectionEnd();
                setText(value);
                if (digits > 3)
                    setCaretPosition(0);
                // setScrollOffset(0);
                // if (getCaretPosition() > value.length())
                //    setCaretPosition(value.length());
                if (k > 0)
                    selectAll();
            }
        }
    }


    private void focusLostAction() {
        if(undo!=null)
           Undo.removeUndoMgr(undo,this);

        if (inModalMode || inEditMode)
            return;
        if (vnmrCmd == null && m_actionCmd == null)
            return;
        if ((m_isActive < 0) || (vnmrIf == null))
            return;
        String d = getText();
        if (d != null)
            d = d.trim();
        if(value != null) {
            value = value.trim();
            if ( ! value.equals(d)) {
                value = d;
                if (vnmrCmd != null) {
                    vnmrIf.sendVnmrCmd(this, vnmrCmd);
                }
                sendActionCmd();
            }
        }
        else if (d != null) {
            value = d;
            if (vnmrCmd != null) {
                vnmrIf.sendVnmrCmd(this, vnmrCmd);
            }
            sendActionCmd();
        }
        if (!m_bMouse && !inEditMode)
            select(0,0);
    }

    private void focusGainedAction() {
        if(undo==null)
            undo=new VTextUndoMgr(this);
        Undo.setUndoMgr(undo,this);
        if (!m_bMouse && !inEditMode)
            selectAll();
    }

    private void setFocusedObj() {
        VObjUtil.setFocusedObj((VObjIF)this);
    }

    private void removeFocusedObj() {
        VObjUtil.setFocusedObj((VObjIF)this);
    }


    private void entryAction() {
        if (inModalMode || inEditMode)
            return;
        if (vnmrCmd == null && m_actionCmd == null)
            return;
        if (!hasFocus())
            return;
        if (vnmrIf == null)
            return;
        value = getText();
        if (value != null)
            value = value.trim();
        if (m_isActive >= 0) {
            if (vnmrCmd != null) {
                vnmrIf.sendVnmrCmd(this, vnmrCmd);
            }
            sendActionCmd();
        }
    }

    /**
     * Send an ActionEvent to all the action listeners, only if the
     * actionCmd has been set.
     */
    private void sendActionCmd() {
        if (m_actionCmd != null) {
            ActionEvent event = new ActionEvent(this, hashCode(), m_actionCmd);
            /********
            for (ActionListener listener : m_actionListenerList) {
                listener.actionPerformed(event);
            }
            ********/
            Iterator<ActionListener> iter = m_actionListenerList.iterator();
            while (iter.hasNext())
            {
                ActionListener listener = iter.next();
                listener.actionPerformed(event);
            }
        }
    }

    /**
     * Sets the action command to the given string.
     * @param actionCommand The new action command.
     */
    public void setActionCommand(String actionCommand) {
        m_actionCmd = actionCommand;
    }

    /**
     * Add an action listener to the list of listeners to be notified
     * of actions.
     */
    public void addActionListener(ActionListener listener) {
        if (m_actionListenerList == null)
           m_actionListenerList = Collections.synchronizedList(new ArrayList<ActionListener>());

        m_actionListenerList.add(listener);
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
    } // drop

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
        value = getText();
        if (value != null)
            value = value.trim();
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
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

    public void setBounds(int x, int y, int w, int h) {
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
        super.setBounds(x, y, w, h);
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
       
//       if ((rHeight < 10) && (fontH > 10))
//           rHeight = 10;
       if (oldH != rHeight) {
           //Font  curFont = font.deriveFont((float) rHeight);
           Font curFont = DisplayOptions.getFont(font.getName(),font.getStyle(),(int)(rHeight));
           setFont(curFont);
       }
       if (rHeight > h)
           rHeight = h;
   }
    
    /* CSHMouseAdapter
     * 
     * Mouse Listener to put up Context Sensitive Help (CSH) Menu and
     * respond to selection of that menu.  The panel's .xml file must have
     * "helplink" set to the keyword/topic for Robohelp.  It must be a 
     * topic listed in the .properties file for this help manual.
     * If helplink is not set, it will open the main manual.
     */
    private class CSHMouseAdapter extends MouseAdapter  {

        public CSHMouseAdapter() {
            super();
        }
        
        // mousePressed and mouseReleased are ONLY for VEntry.
        // These are not to be used for other V objects
        public void mousePressed(MouseEvent evt)
        {
            m_bMouse = true;
        }

        public void mouseReleased(MouseEvent evt)
        {
            m_bMouse = false;
        }

        public void mouseClicked(MouseEvent evt) {
            int btn = evt.getButton();
            if(btn == MouseEvent.BUTTON3) {
                // Find out if there is any help for this item. If not, bail out
                String helpstr=m_helplink;
                if(helpstr==null){
                    Container group = getParent();
                    if(group instanceof VGroup)
                        helpstr=((VGroup)group).getAttribute(HELPLINK);
                    if(helpstr==null){
                        Container group2 = group.getParent();
                        if(group2 instanceof VGroup)
                            helpstr=((VGroup)group2).getAttribute(HELPLINK);
                        if(helpstr==null){
                            Container group3 = group2.getParent();
                            if(group3 instanceof VGroup)
                                helpstr=((VGroup)group3).getAttribute(HELPLINK);
                        }
                    }
                }
                // If no help is found, don't put up the menu, just abort
                if(helpstr == null) {
                    return;
                }
                
                // Create the menu and show it
                JPopupMenu helpMenu = new JPopupMenu();
                String helpLabel = Util.getLabel("CSHMenu");
                JMenuItem helpMenuItem = new JMenuItem(helpLabel);
                helpMenuItem.setActionCommand("help");
                helpMenu.add(helpMenuItem);
                    
                ActionListener alMenuItem = new ActionListener()
                    {
                        public void actionPerformed(ActionEvent e)
                        {
                            // Get the helplink string for this object
                            String helpstr=m_helplink;
                            
                            // If helpstr is not set, see if there is a higher
                            // level VGroup that has a helplink set.  If so, use it.
                            // Try up to 3 levels of group above this.
                            if(helpstr==null){
                                  Container group = getParent();
                                  if(group instanceof VGroup)
                                         helpstr=((VGroup)group).getAttribute(HELPLINK);
                                  if(helpstr==null){
                                      Container group2 = group.getParent();
                                      if(group2 instanceof VGroup)
                                             helpstr=((VGroup)group2).getAttribute(HELPLINK);
                                      if(helpstr==null){
                                          Container group3 = group2.getParent();
                                          if(group3 instanceof VGroup)
                                                 helpstr=((VGroup)group3).getAttribute(HELPLINK);
                                      }
                                  }
                            }
                            // Get the ID and display the help content
                            CSH_Util.displayCSHelp(helpstr);
                        }
                    };
                helpMenuItem.addActionListener(alMenuItem);
                    
                Point pt = evt.getPoint();
                helpMenu.show(VEntry.this, (int)pt.getX(), (int)pt.getY());

            }
             
        }
    }  /* End CSHMouseAdapter class */
}
