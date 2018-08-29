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

public class VCheck extends JCheckBox
    implements VObjIF, VEditIF, StatusListenerIF, VObjDef,
               DropTargetListener, PropertyChangeListener, ActionComponent
{
    public String type = null;
    public String label = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String vnmrCmd2 = null;
    public String showVal = null;
    public String setVal = null;
    protected String strSubtype;
    protected boolean m_bParameter = false;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String objName = null;
    public Color  fgColor = null;
    public Font   font = null;
    public Font   font2 = null;
    private int isActive = 1;
    private String statusParam = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean bFocusable = false;
    private MouseAdapter mlEditor;
    private MouseAdapter mlNonEditor;
    private RightsList rightsList=null;
    protected String m_helplink = null;
    private ButtonIF vnmrIf;
    private boolean inModalMode = false;

    private FontMetrics fm = null;
    private float fontRatio = 1;
    private int rWidth = 0;
    private int rWidth2 = 0;
    private int nWidth = 0;
    static  int iconWidth = 0;
    private int rHeight = 0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private boolean panel_enabled=true;
    private Insets myInsets = new Insets(0,0,0,0);
    protected String m_actionCmd = null;
    protected String m_parameter = null;
    protected String tipStr = null;
    protected String m_viewport = null;

    private Set<ActionListener> m_actionListenerList
        = new TreeSet<ActionListener>();


    public VCheck(SessionShare sshare, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        this.fg = "black";
        this.fontSize = "8";
        setBorder(new VButtonBorder());
        setText("");
        //setBackground(null);
        setOpaque(false);

        super.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                boolean flag = ((JCheckBox) e.getSource()).isSelected();
                checkAction(e, flag);
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
                        else {
                            Component comp = ((Component) evt.getSource())
                                    .getParent();
                            if (comp instanceof VParameter)
                                ParamEditUtil.setEditObj((VParameter) comp);
                        }
                    }
                }
            }
        };
        
        mlNonEditor = new CSHMouseAdapter();
        
        // Start with the non editor listener.  If we change to Editor mode
        // it will be changed.
        addMouseListener(mlNonEditor);

        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
        bFocusable = isFocusable();
        setFocusable(false);
    }

    public void  destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
        if(fg != null) {
            fgColor = DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        changeFont();
    }

    public Insets getInsets() {
        return myInsets;
    }

/*
 * public boolean isFocusTraversable() { return false; }
 */

    public boolean isRequestFocusEnabled() {
        return Util.isFocusTraversal();
    }

    public void setVisible(boolean bShow) {
        if (m_bParameter && !inEditMode && vnmrCmd == null && vnmrCmd2 == null
            && m_actionCmd == null && setVal == null)
        {
            bShow = false;
        }
        super.setVisible(bShow);
    }

    public void setDefLabel(String s) {
        this.label = s;
        setText(s);
        rWidth = 0;
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
            /*
             * setEnabled(false);
             */
            // Be sure both are cleared out
            removeMouseListener(mlNonEditor);
            removeMouseListener(mlEditor);
            addMouseListener(mlEditor);
            if (font != null)
                setFont(font);
            fontRatio = 1.0f;
            defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
            rWidth2 = rWidth;
            setFocusable(bFocusable);
         } else {
            /*
             * setEnabled(true);
             */
            // Be sure both are cleared out
            removeMouseListener(mlEditor);
            removeMouseListener(mlNonEditor);
            addMouseListener(mlNonEditor);
            isEditing = s;
            setFocusable(false);
        }
        inEditMode = s;
    }

    public boolean isParameterEnabled() {
        boolean bShow = false;
        if (m_bParameter) {
            Container container = getParent();
            if (container instanceof VParameter) {
                bShow = ((VParameter) container).getParameterEnabled();
            }
        }
        return bShow;
    }

    public void changeFont() {
        font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
        setFont(font);
        fontRatio = 1.0f;
        font2 = null;
        fm = getFontMetrics(font);
        if (label != null) {
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
        case CMD2:
            return vnmrCmd2;
        case SETVAL:
            return setVal;
        case SUBTYPE:
            return strSubtype;
        case VARIABLE:
            return vnmrVar;
        case VALUE:
            if (isSelected())
                return "1";
            else
                return "0";
        case STATPAR:
            return statusParam;
        case ACTIONCMD:
            return m_actionCmd;
        case PANEL_PARAM:
            return m_parameter;
        case TOOL_TIP:
            return null;
        case TOOLTIP:
            return tipStr;
        case TRACKVIEWPORT:
            return m_viewport;
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
        case LABEL:
            label = c;
            setText(c);
            rWidth = 0;
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
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
        case CMD:
            vnmrCmd = c;
            break;
        case CMD2:
            vnmrCmd2 = c;
            break;
        case SETVAL:
            setVal = c;
            break;
        case SUBTYPE:
            strSubtype = c;
            if (strSubtype != null && strSubtype.equals("parameter"))
                m_bParameter = true;
            else
                m_bParameter = false;
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case VALUE:
            if (c.equals("1"))
                setSelected(true);
            else
                setSelected(false);
            break;
        case STATPAR:
            if (c != null)
                c = c.trim();
            statusParam = c;
            if (c != null)
                updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        case ENABLED:
            panel_enabled=c.equals("false")?false:true;
            setEnabled(panel_enabled);
            break;
        case ACTIONCMD:
            m_actionCmd = c;
            break;
        case PANEL_PARAM:
            m_parameter = c;
            break;
        case TOOL_TIP:
        case TOOLTIP:
            tipStr = c;
            if (c != null && c.length() > 0)
               setToolTipText(Util.getLabel(c));
            else
               setToolTipText(null);
            break;
        case TRACKVIEWPORT:
            m_viewport = c;
            break;
        case PANEL_NAME:
            objName = c;
            break;
        case HELPLINK:
            m_helplink = c;
            break;
        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void updateValue() {
        if (m_viewport != null && m_viewport.toLowerCase().startsWith("y")) {
            vnmrIf = Util.getActiveView();
        }
        if (vnmrIf == null)
            return;
        if (showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
        }
        else if (setVal != null) {
            vnmrIf.asyncQueryParam(this, setVal);
        }
        setVisible(true);
    }

    public void setValue(ParamIF pf) {
        if (pf != null) {
            String strValue = pf.value.trim();
            if (strValue != null) {
                int ival;
                try {
                   ival = Integer.parseInt(strValue);
                } catch (NumberFormatException e) {
                   ival = 0;
                }
                if (ival <= 0)
                    setSelected(false);
                else
                    setSelected(true);
            }
        }
    }

    public int getActiveMode() {
        return isActive;
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            isActive = Integer.parseInt(s);
            if (panel_enabled && isActive >= 0) {
                // if it's part of the vparameter, then check if the parameter
                // panel is enabled, if it's not enabled then disable it
                if (m_bParameter && !isParameterEnabled())
                    setEnabled(false);
                else
                    setEnabled(true);
                if (setVal != null)
                    vnmrIf.asyncQueryParam(this, setVal);
            }
            else{
                // if the panel is disabled, update the value
                // if (!panel_enabled && setVal != null && isActive >= 0)
                if (setVal != null)
                    vnmrIf.asyncQueryParam(this, setVal);
                if (m_bParameter && isActive == -2)
                    super.setVisible(false);
                else
                    setEnabled(false);
            }
        }
    }

    public void updateStatus(String msg) {
        if (msg == null || statusParam == null || msg.length() == 0 ||
            msg.equals("null") || msg.indexOf(statusParam) < 0) {
            return;
        }
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals(statusParam)) {
                String status = tok.nextToken();
                setSelected(Boolean.valueOf(status).booleanValue());
            }
        }
    }

    public void paint(Graphics g) {
        super.paint(g);
        if (!isEditing)
            return;
        // Dimension psize = getPreferredSize();
        Dimension psize = getSize();
        if (isFocused)
            g.setColor(Color.yellow);
        else
            g.setColor(Color.green);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height - 1, psize.width - 1, psize.height - 1);
        g.drawLine(psize.width - 1, 0, psize.width - 1, psize.height - 1);
    }

    public void refresh() {
    }

    private void checkAction(ActionEvent ev, boolean set) {
        if (inModalMode || inEditMode || vnmrIf == null)
            return;
        if (set) {
            if (vnmrCmd != null)
                vnmrIf.sendVnmrCmd(this, vnmrCmd);
        } else {
            if (vnmrCmd2 != null)
                vnmrIf.sendVnmrCmd(this, vnmrCmd2);
        }
        sendActionCmd(set);
    }

    /**
     * Send an ActionEvent to all the action listeners, only if the
     * actionCmd has been set.
     */
    private void sendActionCmd(boolean b) {
        if (m_actionCmd != null) {
            ActionEvent event = new ActionEvent(this, hashCode(), m_actionCmd);
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
        m_actionCmd = actionCommand;
    }

    /**
     * Add an action listener to the list of listeners to be notified
     * of actions.
     */
    public void addActionListener(ActionListener listener) {
        m_actionListenerList.add(listener);
    }


    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        if (!inEditMode) {
            e.rejectDrop();
            return;
        }
        try {
            Transferable tr = e.getTransferable();
            if (tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)) {
                Object obj =
                    tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
                if (obj instanceof VObjIF) {
                    e.acceptDrop(DnDConstants.ACTION_MOVE);
                    JComponent comp = (JComponent)obj;
                    Container cont = (Container)comp.getParent();
                    while (cont != null) {
                        if (cont instanceof ParamPanel)
                                break;
                        cont = cont.getParent();
                    }
                    e.getDropTargetContext().dropComplete(true);

                    if (cont != null) {
                        Point pt1 = cont.getLocationOnScreen();
                        Point pt2 = this.getLocationOnScreen();
                        Point pte = e.getLocation();
                        int x = pt2.x + pte.x - 10;
                        int y = pt2.y + pte.y - 10;
                        if (x < pt1.x) x = pt1.x;
                        if (y < pt1.y) y = pt1.y;
                        comp.setLocation(x, y);
                        ParamEditUtil.relocateObj((VObjIF) comp);
                    }
                    return;
                }
            }
        } catch (IOException io) {}
          catch (UnsupportedFlavorException ufe) { }
        e.rejectDrop();
    } // drop

    public Object[][] getAttributes() {
        VnmrjIF vif = Util.getVjIF();
        if (vif != null)
            rightsList = vif.getRightsList();
      
        if(rightsList != null && rightsList.isApproved("mayedithelpfield"))
            return attributes_H;
        else
            return attributes;
    }

    private final static Object[][] attributes = {
            { new Integer(LABEL), Util.getLabel(LABEL) },
            { new Integer(VARIABLE), Util.getLabel(VARIABLE) },
            { new Integer(SETVAL), Util.getLabel(SETVAL) },
            { new Integer(SHOW), Util.getLabel(SHOW) },
            { new Integer(CMD), Util.getLabel(CMD) },
            { new Integer(CMD2), Util.getLabel(CMD2) },
            { new Integer(STATPAR), Util.getLabel(STATPAR) }, 
            { new Integer(TOOLTIP), Util.getLabel(TOOLTIP) },
            };
    private final static Object[][] attributes_H = {
        { new Integer(LABEL), Util.getLabel(LABEL) },
        { new Integer(VARIABLE), Util.getLabel(VARIABLE) },
        { new Integer(SETVAL), Util.getLabel(SETVAL) },
        { new Integer(SHOW), Util.getLabel(SHOW) },
        { new Integer(CMD), Util.getLabel(CMD) },
        { new Integer(CMD2), Util.getLabel(CMD2) },
        { new Integer(STATPAR), Util.getLabel(STATPAR) }, 
        { new Integer(TOOLTIP), Util.getLabel(TOOLTIP) },
        {new Integer(HELPLINK), Util.getLabel("blHelp")}
        };

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if (vnmrIf == null)
            return;
        if (this.isSelected()) {
            if (vnmrCmd != null)
               vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
        else {
            if (vnmrCmd2 != null)
               vnmrIf.sendVnmrCmd(this, vnmrCmd2);
        }
    }

    public void setSizeRatio(double x, double y) {
        double xRatio = x;
        double yRatio = y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
        if (defDim.width <= 0)
            defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double) defDim.width * xRatio);
        curDim.height = (int) ((double) defDim.height * yRatio);
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

    public void calSize() {
        if (fm == null) {
            font = getFont();
            fm = getFontMetrics(font);
        }
        rHeight = font.getSize();
        if (iconWidth == 0) {
            Icon icon = UIManager.getIcon("CheckBox.icon");
            if (icon != null)
                iconWidth = icon.getIconWidth() + getIconTextGap() + 2;
            else
                iconWidth = 18;
            icon = null;
        }
        rWidth = iconWidth;
        if (label != null)
            rWidth += fm.stringWidth(label);
        rWidth2 = rWidth;
    }

    public void setBounds(int x, int y, int w, int h) {
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

    public void adjustFont() {
        Font curFont = null;

        if (curDim.width <= 0)
            return;
        int w = curDim.width;
        int h = curDim.height;
        nWidth = w;
        if (label == null || (label.length() <= 0)) {
            return;
        }
        if (w > rWidth2) {
            if (fontRatio >= 1.0f)
                return;
        }
        float s = (float) (w - iconWidth) / (float) (rWidth - iconWidth);
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
                    font2 = DisplayOptions.getFont("Dialog", font.getStyle(),
                            rHeight);
                else
                    font2 = font;
            }
            if (s < (float) rHeight)
                s++;
            //curFont = font2.deriveFont(s);
            curFont = DisplayOptions.getFont(font2.getName(), font2.getStyle(),
                    (int) s);
        } else
            curFont = font;
        int fh = 0;
        String strfont = curFont.getName();
        String strstyle = String.valueOf(curFont.getStyle());
        while (s >= 7) {
            FontMetrics fm2 = getFontMetrics(curFont);
            rWidth2 = iconWidth;
            if (label != null)
                rWidth2 += fm2.stringWidth(label);
            fh = curFont.getSize();
            if ((rWidth2 < nWidth) && (fh < h))
                break;
            s = s - 0.5f;
            if (s < 7)
                break;
            //curFont = curFont.deriveFont(s);
            curFont = DisplayOptions.getFont(curFont.getName(), curFont
                    .getStyle(), (int) s);
        }
        if (rWidth2 > w)
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
                // If helpstr is not set, see if there is a higher
                // level VGroup that has a helplink set.  If so, use it.
                // Try up to 3 levels of group above this.
                if(helpstr==null){
                    Container group = getParent();
                    helpstr = CSH_Util.getHelpFromGroupParent(group, label, LABEL, HELPLINK);
                    
                }

                // If no help available, don't put up the menu.
                if(!CSH_Util.haveTopic(helpstr))
                    return;
                
                // Create the menu and show it
                // If no help is found, don't put up the menu, just abort
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
                                helpstr = CSH_Util.getHelpFromGroupParent((VGroup)group, label, LABEL, HELPLINK);           
                            }

                            // Get the ID and display the help content
                            CSH_Util.displayCSHelp(helpstr);
                        }

                    };
                helpMenuItem.addActionListener(alMenuItem);
                    
                Point pt = evt.getPoint();
                helpMenu.show(VCheck.this, (int)pt.getX(), (int)pt.getY());

            }
             
        }
    }  /* End CSHMouseAdapter class */

}

