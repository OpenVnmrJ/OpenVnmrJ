/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;
import java.awt.image.*;
import java.beans.*;
import java.util.*;
import java.awt.dnd.*;
import java.awt.event.*;
import javax.swing.*;

import vnmr.util.*;
import vnmr.ui.*;

// public class VButton extends XJButton implements VObjIF, VObjDef, VEditIF, MouseListener
public class VButton extends JButton implements VObjIF, VObjDef, VEditIF,
        DropTargetListener, ActionListener, PropertyChangeListener,
        StatusListenerIF, ActionComponent, ExpListenerIF, VSizeIF {
    public String type = null;
    public String label = null;
    public String precision = null;
    public String vnmrVar = null;
    public String labelVar = null;
    public String vnmrCmd = null;
    public String vnmrCmd2 = null;
    public String showVal = null;
    public String setVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String tipStr = null;
    public String fg = null;
    public String bg = null;
    public String iconName = null;
    public String borderStr = null;
    public String stretchStr = null;
    public String objName = null;
    public Color fgColor = null;
    public Color bgColor = null; // Normal bg color, without rollover
    public Font font = null;
    public Font font2 = null;
    private String statusParam = null;
    private String statusEnable = null;
    protected boolean inEditMode = false;
    protected boolean bToolButton = false;
    protected boolean isEditing = false;
    protected boolean isFocused = false;
    protected boolean inModalMode = false;
    protected boolean panel_enabled = true;
    protected boolean bNoBorder = false;
    protected boolean bBorderOnEnter = false;
    protected boolean bPaintBorder = true;
    private MouseAdapter mlEditor;
    private MouseAdapter mlNonEditor;
    private RightsList rightsList=null;
    protected String m_helplink = null;
    protected ButtonIF vnmrIf;
    protected int isActive = 1;
    protected String strIs3D = Util.getLabel("mlYes");
    protected String m_viewport = null;

    private FontMetrics fm = null;
    private float fontRatio = 1;
    protected int rWidth = 0;
    private int rWidth2 = 0;
    private int nWidth = 0;
    private int rHeight = 0;
    private int stretchDir = VStretchConstants.NONE;
    private double xRatio = 1.0;
    private double yRatio = 1.0;
    private Dimension defDim = new Dimension(0, 0);
    private Dimension curDim = new Dimension(0, 0);
    private Dimension tmpDim = new Dimension(0, 0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private VButtonIcon icon = null;
    private ImageIcon jicon = null;

    private String m_actionCmd = null;
    // private Set<ActionListener> m_actionListenerList = new TreeSet<ActionListener>();
    private java.util.List <ActionListener> m_actionListenerList;

    public VButton(SessionShare sshare, ButtonIF vif, String typ) {
        super();
        this.type = typ;
        this.vnmrIf = vif;
        // this.fg = "black";
        this.fontName = "Dialog";
        this.fontSize = "12";
        this.label = "";

        setBackground(bg);

        setMargin(new Insets(0, 0, 0, 0));

        // addMouseListener(this);
        super.addActionListener(this);

        mlEditor = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if((modifier & (1 << 4)) != 0) {
                    if(clicks >= 2) {
                        ParamEditUtil.setEditObj((VObjIF)evt.getSource());
                    }
                }
            }
        };
        
        mlNonEditor = new CSHMouseAdapter();
    
        // Start with the non editor listener.  If we change to Editor mode
        // it will be changed.
        addMouseListener(mlNonEditor);
        
        setFocusPainted(false);
        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
    }

    public void mouseEntered(MouseEvent me) {
        /***
        if(getModel().isEnabled()) {
            Color bg = bgColor;
            if(bg != null || (bg = Util.getParentBackground(this)) != null) {
                setBackground(Util.changeBrightness(bg, 10));
                repaint();
            }
        }
        ***/
    }

    public void mouseExited(MouseEvent me) {
        /***
        if(getModel().isEnabled()) {
            setBackground(bgColor);
            repaint();
        }
        ***/
    }

    public void mouseClicked(MouseEvent me) {
    }

    public void mousePressed(MouseEvent me) {
        //if (!inEditMode)
       //    requestFocus();
    }

    public void mouseReleased(MouseEvent me) {
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    @Override
    public boolean isBorderPainted() {
        return bPaintBorder;
    }

    @Override
    public void setBorderPainted(boolean b) {
        boolean oldValue = bPaintBorder;
        bPaintBorder = b;
        if (b != oldValue)
            repaint();
    }


    // VEditIF interface

    protected static final String[] m_arrStrYesNo = { Util.getLabel("mlYes"),
            Util.getLabel("mlNo") };
    private final static String[] m_arrStrTtlJust = {"Left","Center","Right"};
    /** The array of the attributes that are displayed in the edit template. */
    private final static Object[][] attributes = {
            { new Integer(LABEL), Util.getLabel(LABEL) },
            { new Integer(ICON), Util.getLabel(ICON) },
            { new Integer(VARIABLE), Util.getLabel(VARIABLE) },
            { new Integer(SHOW), Util.getLabel(SHOW) },
            { new Integer(CMD), Util.getLabel(CMD) },
            { new Integer(STATPAR), Util.getLabel(STATPAR) },
            { new Integer(STATSHOW), Util.getLabel(STATSHOW) },
            { new Integer(VAR2), Util.getLabel(LABELVARIABLE) },
	    { new Integer(SETVAL),   Util.getLabel(LABELVALUE) },
            { new Integer(JUSTIFY), "Label justification:", "menu", m_arrStrTtlJust},
            { new Integer(TOOLTIP), Util.getLabel(TOOLTIP) },
            { new Integer(BGCOLOR), Util.getLabel(BGCOLOR), "color" }
    };
           //  { new Integer(DECOR1), Util.getLabel("vbDECOR1"), "radio",
           //          m_arrStrYesNo }

    private final static Object[][] attributes_H = {
        { new Integer(LABEL), Util.getLabel(LABEL) },
        { new Integer(ICON), Util.getLabel(ICON) },
        { new Integer(VARIABLE), Util.getLabel(VARIABLE) },
        { new Integer(SHOW), Util.getLabel(SHOW) },
        { new Integer(CMD), Util.getLabel(CMD) },
        { new Integer(STATPAR), Util.getLabel(STATPAR) },
        { new Integer(STATSHOW), Util.getLabel(STATSHOW) },
        { new Integer(VAR2), Util.getLabel(LABELVARIABLE) },
	{ new Integer(SETVAL),   Util.getLabel(LABELVALUE) },
        { new Integer(JUSTIFY), "Label justification:", "menu", m_arrStrTtlJust},
        { new Integer(TOOLTIP), Util.getLabel(TOOLTIP) },
        { new Integer(BGCOLOR), Util.getLabel(BGCOLOR), "color" },
        {new Integer(HELPLINK), Util.getLabel("blHelp")}
    };

    public Object[][] getAttributes() {
        VnmrjIF vif = Util.getVjIF();
        if (vif != null)
            rightsList = vif.getRightsList();
      
        if(rightsList != null && rightsList.isApproved("mayedithelpfield"))
            return attributes_H;
        else
            return attributes;
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
        if(fg != null) {
            fgColor = DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        setBackground(bg);
        changeFont();
    }

    public void setBackground(String name) {
        if(name == null || name.length() == 0 || name.equals("transparent") || name.equals("VJBackground")) {
            bgColor = UIManager.getColor("Button.background");
            // bgColor = Util.getBgColor();
        } else {
            bgColor = DisplayOptions.getColor(name);
        }
        setBackground(bgColor);
    }


    public void actionPerformed(ActionEvent e) {
        if(inModalMode || inEditMode || isActive < 0)
            return;
        if(vnmrIf == null)
            vnmrIf = (ButtonIF) Util.getActiveView();
        VObjIF vobj = VObjUtil.getFocusedObj();
        if (vobj != null) {
             Component focusComp = (Component) vobj;
             if (focusComp.isShowing())
                 vobj.changeFocus(false);
             else
                 VObjUtil.setFocusedObj(null);
        }

        if(vnmrIf != null && vnmrCmd != null) {
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
        } else {
            sendActionCmd();
        }
    }

    /**
     * Send an ActionEvent to all the action listeners, only if the actionCmd
     * has been set.
     */
    private void sendActionCmd() {
        if (m_actionListenerList == null)
             return;
        if(m_actionCmd != null) {
            ActionEvent event = new ActionEvent(this, hashCode(), m_actionCmd);
            for(ActionListener listener : m_actionListenerList) {
                listener.actionPerformed(event);
            }
        }
    }

    /**
     * Sets the action command to the given string.
     * 
     * @param actionCommand
     *            The new action command.
     */
    public void setActionCommand(String actionCommand) {
        m_actionCmd = actionCommand;
    }

    /**
     * Add an action listener to the list of listeners to be notified of
     * actions.
     */
    public void addActionListener(ActionListener listener) {
        if (m_actionListenerList == null)
          m_actionListenerList = Collections.synchronizedList(new LinkedList<ActionListener>());

        if (!m_actionListenerList.contains(listener))
           m_actionListenerList.add(listener);
    }

    public boolean isRequestFocusEnabled() {
        return Util.isFocusTraversal();
    }

    public void setDefLabel(String s) {
        this.label = s;
        setText(s);
        rWidth = 0;
        setToolBarButton(bToolButton);
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        if(s) {
            // Be sure both are cleared out
            removeMouseListener(mlNonEditor);
            removeMouseListener(mlEditor);
            addMouseListener(mlEditor);
            if(font != null)
                setFont(font);
            if(defDim.width <= 0)
                defDim = getPreferredSize();
            fontRatio = 1.0f;
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
            xRatio = 1.0;
            yRatio = 1.0;
            rWidth2 = rWidth;
        } else {
            // Be sure both are cleared out
            removeMouseListener(mlEditor);
            removeMouseListener(mlNonEditor);
            addMouseListener(mlNonEditor);
        }
        inEditMode = s;
    }

    private void setDefSize() {
        if(rWidth <= 0)
            calSize();
        defDim.width = rWidth + 8;
        defDim.height = rHeight + 8;
        setPreferredSize(defDim);
    }

    public void changeFont() {
        if (bToolButton) {
            setToolBarButton(bToolButton);
            return;
        }
        font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
        setFont(font);
        font2 = null;
        fm = getFontMetrics(font);
        calSize();
        if(!isPreferredSizeSet()) {
            setDefSize();
        }
        fontRatio = 1.0f;
        if(!inEditMode) {
            if((curDim.width > 0) && (rWidth > curDim.width)) {
                adjustFont(curDim.width, curDim.height);
            }
        }
        repaint();
    }


    public String getAttribute(int attr) {
        switch(attr) {
        case TYPE:
            return type;
        case LABEL:
        case VALUE:
            return label;
        case FGCOLOR:
            return fg;
        case BGCOLOR:
            return bg;
	case SETVAL:
            return setVal;
        case SHOW:
            return showVal;
        case FONT_NAME:
            return fontName;
        case FONT_STYLE:
            return fontStyle;
        case FONT_SIZE:
            return fontSize;
        case CMD:
            return vnmrCmd;
        case VARIABLE:
            return vnmrVar;
        case VAR2:
            return labelVar;
        case NUMDIGIT:
            return precision;
        case ICON:
            return iconName;
        case STATPAR:
            return statusParam;
        case STATSHOW:
            return statusEnable;
        case DECOR1:
            return strIs3D;
        case TOOL_TIP:
            return null;
        case TOOLTIP:
            return tipStr;
        case ACTIONCMD:
            return m_actionCmd;
        case TRACKVIEWPORT:
            return m_viewport;
        case HELPLINK:
            return m_helplink;   
        case BORDER:
            if (bBorderOnEnter)
               return "onEnter"; 
            if (bNoBorder)
               return "None"; 
            return borderStr;
        case STRETCH:
            return stretchStr;
        case JUSTIFY:
            int a = getHorizontalAlignment();
            if (a == SwingConstants.LEFT)
               return "Left";
            if (a == SwingConstants.RIGHT)
               return "Right";
            return "Center";
        case PANEL_NAME:
            return objName;
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        switch(attr) {
        case TYPE:
            type = c;
            break;
        case LABEL:
        case VALUE:
            label = c;
            if (tipStr == null && c != null) {
               if (c.length() > 0) {
                  String tp = Util.getTooltipText(c, null);
                  if (tp != null && tp.length() > 0)
                     setToolTipText(tp);
               }
            }
            setText(c);
            rWidth = 0;
            setToolBarButton(bToolButton);
            break;
        case FGCOLOR:
            fg = c;
            fgColor = DisplayOptions.getColor(fg);
            setForeground(fgColor);
            repaint();
            break;
        case BGCOLOR:
            bg = c;
            setBackground(bg);
            repaint();
            break;
        case SHOW:
            showVal = c;
            updateStatus(ExpPanel.getStatusValue(statusParam));
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
        case NUMDIGIT:
            precision = c;
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case VAR2:
            labelVar = c;
            break;
        case ICON:
            iconName = c;
            icon = null;
            if(c != null) {
                String iname;
                if(c.indexOf('.') > 0)
                    iname=c;
                else
                    iname=c + ".gif";
                jicon = Util.getVnmrImageIcon(iname);
                if(jicon != null) {
                    icon = new VButtonIcon(jicon.getImage());
                    icon.setStretch(stretchDir);
                    if (stretchDir != VStretchConstants.NONE)
                        setIcon(icon);
                    else
                        setIcon(jicon);
                }
                else{
                    jicon = Util.getImageIcon(iname);
                    if(jicon!=null){
                        icon = new VButtonIcon(jicon.getImage());
                        icon.setStretch(stretchDir);
                        if (stretchDir != VStretchConstants.NONE)
                            setIcon(icon);
                        else
                            setIcon(jicon);
                    }
                }
            }     
            if(icon == null)
                setIcon(icon);
            break;
        case STATPAR:
            statusParam = c;
            updateStatusParam(statusParam);
            break;
        case STATSHOW:
            statusEnable = c;
            updateStatusParam(statusParam);
            break;
        case ENABLED:
            panel_enabled = c.equals("false") ? false : true;
            setEnabled(panel_enabled && isActive >= 0);
            break;
        case DECOR1:
            c = c.toLowerCase();
            /***  
            if(c.equals(Util.getLabel("mlYes"))) {
                set3D(true);
            } else {
                set3D(false);
            }
            ***/
            strIs3D = c;
            // setBackground(bgColor);
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
        case TRACKVIEWPORT:
            m_viewport = c;
            break;
        case HELPLINK:
            m_helplink = c;
            break;
        case BORDER:
            borderStr = c;
            bNoBorder = false;
            bBorderOnEnter = false;
            if (c != null) {
                if (c.equalsIgnoreCase("none"))
                    bNoBorder = true;
                else if (c.equalsIgnoreCase("onenter")) {
                    bNoBorder = true;
                    bBorderOnEnter = true;
                }
            }
            setBorderPainted(!bNoBorder);
            if (bNoBorder)
                setContentAreaFilled(false);
            break;
        case STRETCH:
            stretchStr = c;
            stretchDir = VStretchConstants.NONE;
            if (c != null) {
                if (c.equalsIgnoreCase("horizontal"))
                   stretchDir = VStretchConstants.HORIZONTAL;
                else if (c.equalsIgnoreCase("vertical"))
                   stretchDir = VStretchConstants.VERTICAL;
                else if (c.equalsIgnoreCase("both"))
                   stretchDir = VStretchConstants.BOTH;
                else if (c.equalsIgnoreCase("even"))
                   stretchDir = VStretchConstants.EVEN;
            }
            if (stretchDir != 0 && icon != null) {
                icon.setStretch(stretchDir);
                setIcon(icon);
            }
            else {
                if (jicon != null)
                   setIcon(jicon);
            }
            break;
        case JUSTIFY:
            if (c == null)
                return;
            int h = SwingConstants.CENTER;
            if (c.equalsIgnoreCase("left"))
                h = SwingConstants.LEFT;
            else if (c.equalsIgnoreCase("right"))
                h = SwingConstants.RIGHT;
            setHorizontalAlignment(h);
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

    public void updateStatus(String msg) {
        if(msg == null || msg.length() == 0 || msg.equals("null")
                || statusParam == null) {
            return;
        }
        StringTokenizer tok = new StringTokenizer(msg);
        if(tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if(parm.equals(statusParam)) {
                String vstring = tok.nextToken();
                if(statusEnable != null) {
                    if(hasToken(statusEnable, vstring)) {
                        updateStatus(1);
                    } else {
                        updateStatus(-1);
                    }
                }
            }
        }
    }

    private void updateStatus(int flag) {
        setShowValue(new ParamIF("", "", Integer.toString(flag)));
    }

    private void updateStatusParam(String param) {
        if(statusParam == null || statusParam.trim().length() == 0
                || statusEnable == null || statusEnable.trim().length() == 0) {
            return;
        }

        String statMsg = ExpPanel.getStatusValue(statusParam);
        if(statMsg == null) {
            updateStatus(-1);
        } else {
            updateStatus(statMsg);
        }
    }

    public void setValue(ParamIF pf) {
        if(pf != null && pf.value != null) {
            label = pf.value;
            setText(label);
            rWidth = 0;
            setToolBarButton(bToolButton);
        }
    }

    public void setShowValue(int flag) {
        setShowValue(new ParamIF("", "", Integer.toString(flag)));
    }

    public void setShowValue(ParamIF pf) {
        if (vnmrIf == null)
            vnmrIf = (ButtonIF) Util.getActiveView();
        if(pf != null && pf.value != null) {
            String s = pf.value.trim();
            isActive = Integer.parseInt(s);
            if(panel_enabled && isActive > 0) {
                setEnabled(true);
            } else {
                setEnabled(false);
            }
            if(setVal != null && vnmrIf != null)
                vnmrIf.asyncQueryParam(this, setVal);
        }
    }

    public void updateValue() {
        if (m_viewport != null && m_viewport.toLowerCase().startsWith("y")) {
            vnmrIf = Util.getActiveView();
        }
        if(vnmrIf == null)
            return;
        if(showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
        } else if(setVal != null) {
            vnmrIf.asyncQueryParam(this, setVal, precision);
        }
    }

    public void updateValue(Vector params) {
        if (m_viewport != null && m_viewport.toLowerCase().startsWith("y")) {
            vnmrIf = Util.getActiveView();
        }
        StringTokenizer tok;
        String v;
        int pnum = params.size();

        if(vnmrVar == null)
            return;
        tok = new StringTokenizer(vnmrVar, " ,\n");
        while(tok.hasMoreTokens()) {
            v = tok.nextToken();
            for(int k = 0; k < pnum; k++) {
                if(v.equals(params.elementAt(k))) {
                    updateValue();
                    return;
                }
            }
        }
        if(labelVar == null || setVal == null)
            return;
        tok = new StringTokenizer(labelVar, " ,\n");
        while(tok.hasMoreTokens()) {
            v = tok.nextToken();
            for(int k = 0; k < pnum; k++) {
                if(v.equals(params.elementAt(k))) {
                    vnmrIf.asyncQueryParam(this, setVal, precision);
                    return;
                }
            }
        }
    }

    private boolean hasToken(String str, String tok) {
        if(str == null)
            return false;
        if(tok == null)
            return true;
        StringTokenizer strtok = new StringTokenizer(str);
        while(strtok.hasMoreTokens()) {
            if(tok.equals(strtok.nextToken()))
                return true;
        }
        return false;
    }

    public Dimension getMinSize() {
        int w = 0;
        int h = 0;

        if (label != null && label.length() > 0) {
            if(defDim.width <= 0)
                 defDim = getPreferredSize();
            font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
            setFont(font);
            fm = getFontMetrics(font);
            calSize();
            h = rHeight;
            w = rWidth;
        }
        else if (jicon != null) {
            w = jicon.getIconWidth();
            h = jicon.getIconHeight();
        }
        defDim.width = w + 4;
        defDim.height = h + 4;
         
        return new Dimension(w + 4, h + 4);
    }

    public void setToolBarButton(boolean b) {
        bToolButton = b;
        if(b) {
            Dimension dim = getMinSize();
            if (dim.width > 6 && dim.height > 6)
               setPreferredSize(dim);
            else
               setVisible(false);
        }
    }

    public void refresh() {
    }

    public void destroy() {
        jicon = null;
        icon = null;
        DisplayOptions.removeChangeListener(this);
    }

    public void addDefChoice(String s) {
    }

    public void addDefValue(String s) {
    }

    public void dragEnter(DropTargetDragEvent e) {
    }

    public void dragExit(DropTargetEvent e) {
    }

    public void dragOver(DropTargetDragEvent e) {
    }

    public void dropActionChanged(DropTargetDragEvent e) {
    }

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if(vnmrCmd == null || vnmrIf == null)
            return;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    public void setSizeRatio(double x, double y) {
        xRatio = x;
        yRatio = y;
        if(x > 1.0)
            xRatio = x - 1.0;
        if(y > 1.0)
            yRatio = y - 1.0;
        if(defDim.width <= 0) {
            if(!isPreferredSizeSet())
                setDefSize();
            defDim = getPreferredSize();
        }
        curLoc.x = (int)((double)defLoc.x * xRatio);
        curLoc.y = (int)((double)defLoc.y * yRatio);
        curDim.width = (int)((double)defDim.width * xRatio);
        curDim.height = (int)((double)defDim.height * yRatio);
        if(!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }

    public void calSize() {
        if (fm == null) {
            font = getFont();
            fm = getFontMetrics(font);
        }
        rHeight = font.getSize();
        rWidth = 6;
        if(label != null)
            rWidth += fm.stringWidth(label);
        if(getIcon() != null)
            rWidth += getIcon().getIconWidth();
        rWidth2 = rWidth;
    }

    public void setBounds(int x, int y, int w, int h) {
        if(bToolButton) {
            super.setBounds(x, y, w, h);
            return;
        }

        if(inEditMode) {
            defLoc.x = x;
            defLoc.y = y;
            defDim.width = w;
            defDim.height = h;
        }
        curDim.width = w;
        curDim.height = h;
        curLoc.x = x;
        curLoc.y = y;
        if (icon != null)
            icon.setSize(w, h);
        if(!inEditMode) {
            if(rWidth <= 0) {
                calSize();
            }
            if((w != nWidth) || (rWidth2 > w)) {
                adjustFont(w, h);
            }
        }
        super.setBounds(x, y, w, h);
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

    public Point getLocation() {
        if(inEditMode) {
            tmpLoc.x = defLoc.x;
            tmpLoc.y = defLoc.y;
        } else {
            tmpLoc.x = curLoc.x;
            tmpLoc.y = curLoc.y;
        }
        return tmpLoc;
    }

    public Point getLocation(Point pt) {
        return getLocation();
    }

    public void adjustFont(int w, int h) {
        Font curFont = null;
        if(bToolButton)
            return;

        if(w <= 0)
            return;
        nWidth = w;
        if(w > rWidth2) {
            if(fontRatio >= 1.0f)
                return;
        }
        float s = (float)w / (float)rWidth;
        if(rWidth > w) {
            if(s > 0.98f)
                s = 0.98f;
            if(s < 0.5f)
                s = 0.5f;
        }
        if(s > 1)
            s = 1;
        if(s == fontRatio)
            return;
        fontRatio = s;
        s = (float)rHeight * fontRatio;
        if((s < 10) && (rHeight >= 10))
            s = 10;
        if(fontRatio < 1) {
            if(font2 == null) {
                String fname = font.getName();
                if(!fname.equals("Dialog"))
                    font2 = DisplayOptions.getFont("Dialog", font.getStyle(),
                            rHeight);
                else
                    font2 = font;
            }
            if(s < (float)rHeight)
                s += 1;
            curFont = DisplayOptions.getFont(font2.getName(), font2.getStyle(),
                    (int)s);
        } else
            curFont = font;
        String strfont = curFont.getName();
        int nstyle = curFont.getStyle();
        while(s > 9) {
            FontMetrics fm2 = getFontMetrics(curFont);
            rWidth2 = 6;
            if(label != null)
                rWidth2 += fm2.stringWidth(label);
            if(getIcon() != null)
                rWidth2 += getIcon().getIconWidth();
            int fh = curFont.getSize();
            if((rWidth2 < nWidth) && (fh < h))
                break;
            if(s < 10)
                break;
            s--;
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
        }
        if(rWidth2 > w)
            rWidth2 = w;
        setFont(curFont);
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

        public void mouseClicked(MouseEvent evt) {
            int btn = evt.getButton();
            if(btn == MouseEvent.BUTTON3) {
                // Find out if there is any help for this item. If not, bail out
                String helpstr=m_helplink;
                
                // If label is null, try tool tip
                String label2use;
                if(label == null || label.length() == 0) {
                    label2use = getAttribute(TOOLTIP);
                }
                else
                    label2use = label;
             
                // If helpstr is not set, see if there is a higher
                // level VGroup that has a helplink set.  If so, use it.
                // Try up to 3 levels of group above this.
                if(helpstr==null){
                    Container group = getParent();
                    helpstr = CSH_Util.getHelpFromGroupParent(group, label2use, LABEL, HELPLINK);
                }
                
                // If no help available, don't put up the menu.
                if(!CSH_Util.haveTopic(helpstr))
                    return;
            
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
                            
                            // If label is null, try tool tip
                            String label2use;
                            if(label == null || label.length() == 0) {
                                label2use = getAttribute(TOOLTIP);
                            }
                            else
                                label2use = label;

                            
                            // If helpstr is not set, see if there is a higher
                            // level VGroup that has a helplink set.  If so, use it.
                            // Try up to 3 levels of group above this.
                            if(helpstr==null){
                                Container group = getParent();
                                helpstr = CSH_Util.getHelpFromGroupParent(group, label2use, LABEL, HELPLINK);
                            }
                            
                            // Get the ID and display the help content
                            CSH_Util.displayCSHelp(helpstr);
                        }

                    };
                helpMenuItem.addActionListener(alMenuItem);
                    
                Point pt = evt.getPoint();
                helpMenu.show(VButton.this, (int)pt.getX(), (int)pt.getY());

            }
        }

        public void mouseEntered(MouseEvent me) {
            if (bNoBorder) {
                if (bBorderOnEnter)
                    setBorderPainted(true);
                else
                    setContentAreaFilled(true);
            }
        }

        public void mouseExited(MouseEvent me) {
            if (bNoBorder) {
                if (bBorderOnEnter)
                    setBorderPainted(false);
                else
                    setContentAreaFilled(false);
            }
        }
        
    }  /* End CSHMouseAdapter class */

}
