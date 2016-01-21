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
import java.beans.*;
import java.util.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import vnmr.util.*;
import vnmr.ui.*;

/**
 * Title: VToolBarButton
 * Description: Button object that appear in the toolbar.
 */

public class VToolBarButton extends VButton implements DragGestureListener, DragSourceListener,
                                                       PropertyChangeListener
{
    /** String for the setVC command */
    public String m_strSetVC = null;

    protected String m_strSubType = null;

    protected ModelessPopup m_objVpEditor;

    protected boolean m_bInitialize = false;

    protected ActionListener m_alvp;

    protected static boolean m_bInitVis = false;

    /** ToolBarButton object */
    private VToolBarButton m_btn;

    /** if the tool has been dragged and dropped.  */
    private boolean m_bDndTool = false;

    /** Timer object */
    private javax.swing.Timer m_objTimer;

    /** Used for storing the previous position of the button, for undo. */
    private int m_nPreviousPos = 0;

    /** DragSource object */
    private DragSource m_objDragSource = new DragSource();

    private final static int THREE_SEC = 3000;

    /**
     *  Constructor
     *  @sshare SessionShare
     *  @vif    ButtonIF object
     *  @typ    Specified type
     */
    public VToolBarButton(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
        // Mouse Listener to listen to addtoolsdialog.
        setMouseListener();

        this.fg = "black";
        this.fontSize = "12";
        this.fontStyle = "PlainText";

        m_alvp = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                String cmd = e.getActionCommand();
                setBtn(cmd);
            }
        };
        // Recognizer for movements of this object.
        DragGestureRecognizer objDragRecognizer
                = m_objDragSource.createDefaultDragGestureRecognizer
                (null,
                 DnDConstants.ACTION_COPY_OR_MOVE,
                 this);
        objDragRecognizer.setComponent(this);
        DisplayOptions.addChangeListener(this);
    }

    public void propertyChange(PropertyChangeEvent evt) {
        if(fg!=null){
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        // bgColor = Util.getBgColor();
        if (bToolButton) {
           bgColor = Util.getToolBarBg();
           setBackground(bgColor);
        }
        changeFont();
    }

    public void actionPerformed(ActionEvent e) {
        // overwriting the superclass method
        // the actions are done with the mouse listeners
    }

    /**
     *  Gets the value of the specified attribute.
     */
    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
            /*case LABEL:
              case VALUE:
              return label;*/
        case FGCOLOR:
            return fg;
        case BGCOLOR:
            return null;
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
        case SETVAL:
            return setVal;
        case ENABLED:
            if (isEnabled())
                return "yes";
            else
                return "no";
        case SUBTYPE:
            return m_strSubType;
        case NUMDIGIT:
            return precision;
        case ICON:
            return iconName;
        case SET_VC:
            return m_strSetVC;
        case TOOL_TIP:
        case TOOLTIP:
            return tipStr;
        default:
            return super.getAttribute(attr);
        }
    }

    /**
     *  Sets the various attributes of the object.
     *  @attr   Specified attribute that needs to be set.
     *  @c      String that should be set for the specified attribute.
     */
    public void setAttribute(int attr, String c) {
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case LABEL:
        case VALUE:
            label = c;
            setText(c.replace('"',' '));
            setToolBarButton(bToolButton);
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            repaint();
            break;
        case BGCOLOR:
           /***
            bg = c;
            if (c != null) {
                bgColor = DisplayOptions.getColor(c);
            } else {
                bgColor = Util.getBgColor();
            }
            setBackground(bgColor);
            repaint();
           ***/
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
        case NUMDIGIT:
            precision = c;
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case ENABLED:
            if(c.equals("yes"))
                setEnabled(true);
            else
                setEnabled(false);
            break;
        case SUBTYPE:
            m_strSubType = c;
            if (m_strSubType != null
                && m_strSubType.indexOf(Global.VIEWPORT) >= 0
                && !m_bInitialize)
            {
                addListener(m_alvp);
                setVisible(false);
                m_bInitialize = true;
            }
            break;
        case ICON:
            iconName = c;
            Icon icon = null;
            icon = Util.getVnmrImageIcon(iconName);
            if (icon != null
                && icon.getIconWidth() > 0 && icon.getIconHeight() > 0)
            {
                setIcon(icon);
                setText("");
            } else {
                if (c == null || c.length() < 1)
                     return;
                label = c;
                setText(c);
                if (Integer.parseInt(fontSize) < 12)
                    fontSize = "12";
                changeFont();
                setIcon(null);
                setToolBarButton(bToolButton);
                repaint();
            }
            break;
        case SET_VC:
            m_strSetVC = c;
            break;
        case TOOL_TIP:
        case TOOLTIP:
            tipStr = c;
            if (c != null && c.length() > 0)
               setToolTipText(Util.getLabel(c));
            else
               setToolTipText(null);
            break;
        default:
            super.setAttribute(attr, c);
            break;
        }
    }

    public void updateValue() {
        if (vnmrIf == null)
        {
            VjToolBar toolbar = getToolBar();
            if (toolbar != null)
                vnmrIf = toolbar.getExpIf();
            if (vnmrIf == null)
                return;
        }
        if (showVal != null)
            vnmrIf.asyncQueryShow(this, showVal);
        else if (setVal != null)
            vnmrIf.asyncQueryParam(this, setVal, precision);

    }

    public void setValue(ParamIF pf) {
        String strSet = null;
        if (pf != null && pf.value != null) {
            String strValue = pf.value.trim().replace('"',' ');
            rWidth = 0;
            label = strValue;
            setText(label);
            setToolBarButton(bToolButton);
        }
    }

    public void setShowValue(ParamIF pf) {
        if (isVpEditorShowing())
            return;

        VjToolBar toolbar = getToolBar();
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            isActive = Integer.parseInt(s);
            if (isActive > 0) {
                setVisible(true);
                if (setVal != null)
                    vnmrIf.asyncQueryParam(this, setVal);
                if (toolbar != null)
                    toolbar.validateToolbar();
                if (!m_bInitVis)
                {
                    m_bInitVis = true;
                    // sendVnmrCmd("vnmrjcmd('window', jviewports[3], jviewports[4])");
                }
            }
            else {
                setVisible(false);
                toolbar.setVpVisible();
            }
        }

    }

    public void addListener(ActionListener alvp) {
        ActionHandler vpactionHandler = Util.getVpActionHandler();
        ArrayList aListActionListener = new ArrayList();
        boolean bListenerAdded = false;
        if (vpactionHandler != null)
            aListActionListener = vpactionHandler.getListener();

        ActionListener a;
        for (int i = 0; i < aListActionListener.size(); i++)
        {
            a = (ActionListener)aListActionListener.get(i);
            if (a.equals(alvp))
            {
                bListenerAdded = true;
                break;
            }
        }
        if (!bListenerAdded)
            Util.addVpListener(alvp);
    }

    public void setDnDWithinToolBar( boolean flag ) {
        m_bDndTool = flag;
    }

    public boolean getDnDWithinToolBar() {
        return m_bDndTool;
    }

    /**
     * Returns the toolsdialog.
     */
    protected AddToolsDialog getToolsDialog() {
        return ((getToolBar() != null)
                ? getToolBar().getToolsEditor() : null);
    }

    protected boolean isVpEditorShowing() {
        boolean bShowing = false;
        if (m_objVpEditor == null)
        {
            VjToolBar objToolBar = getToolBar();
            if (objToolBar != null)
                m_objVpEditor = objToolBar.getVpEditor();
        }

        if (m_objVpEditor != null && m_objVpEditor.isShowing())
            bShowing = true;

        return bShowing;
    }

    /**
     * Returns the toolbar.
     */
    protected VjToolBar getToolBar() {
/*
        AppIF appIf = Util.getAppIF();
        VjToolBar objToolBar = null;
        if (appIf instanceof ExperimentIF)
            objToolBar = ((ExperimentIF)appIf).getToolBar();
        return objToolBar;
*/

        return Util.getToolBar();
    }

    /**
     * Returns the previous position of the button.
     */
    public int getPreviousPos() {
        return m_nPreviousPos;
    }

    /**
     * Sets the previous position of the button.
     */
    public void setPreviousPos(int nPos) {
        m_nPreviousPos = nPos;
    }

    int dragParentX = 0;
    int dragParentY = 0;
    int dragParentY2 = 0;
    Point startLoc = new Point(0, 0);

    //=========================================================
    //   DragGestureListener implementation follows ...
    //=========================================================
    public void dragGestureRecognized( DragGestureEvent e ) {
        VjToolBar objToolBar = getToolBar();
        if (objToolBar != null)
        {
            if(( getToolsDialog() != null )
               && getToolsDialog().getToolEditStatus() )
            {
                objToolBar.recordCurrentToolBar("tmpFile1.xml");
                resetFocus(objToolBar.getToolBar());
                resetFocus(objToolBar.getVPToolBar());
                changeFocus(false);
                Component comp = e.getComponent();
                if (comp instanceof VToolBarButton)
                {
                    if (m_strSubType == null
                        || m_strSubType.indexOf(Global.VIEWPORT) < 0)
                    {
                        LocalRefSelection ref
                                = new LocalRefSelection( comp );
                        startLoc = getLocation();
                        Point pt1 = e.getDragOrigin();
                        Point pt2 = getLocationOnScreen();
                        dragParentX = pt2.x - startLoc.x +pt1.x;
                        dragParentY = pt2.y - startLoc.y - 4;
                        dragParentY2 = dragParentY + 50;
                        m_objDragSource.startDrag
                                (e, DragSource.DefaultMoveDrop, ref, this);
                    }
                }
            }
        }
    }

    //=================================================================
    //   DragSourceListener implementation follows ...
    //=================================================================

    public void dragDropEnd( DragSourceDropEvent e ) {}
    public void dragEnter( DragSourceDragEvent e ) {}
    public void dragExit( DragSourceEvent e ) {}
    public void dropActionChanged( DragSourceDragEvent e ) {}
    public void dragOver( DragSourceDragEvent e ) {
          Point pt = e.getLocation();
          int x = pt.x - dragParentX;
          if (x < 1 || pt.y < dragParentY || pt.y > dragParentY2)
               return;
          setLocation(x, startLoc.y);
    }

    /**
     * Sets the mouse listener for the object.
     */
    private void setMouseListener() {
        this.addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent e) {
                m_btn = (VToolBarButton) e.getSource();
                m_btn.setSelected(true);
                if(isToolDialogEditMode())
                {
                    if( e.getClickCount() == 1   )
                    {
                        JComponent comp = ( JComponent ) e.getSource();
                        VjToolBar objToolBar = getToolBar();
                        if (objToolBar != null)
                        {
                            objToolBar.resetToolsInfo( comp );
                            objToolBar.setComponent(comp);
                            objToolBar.getToolsEditor().toolChanged(comp);
                        }
                        m_btn.setBorder(BorderFactory.createLineBorder
                                        (Color.pink));
                        e.consume();
                    }
                }
                else
                {
                    // Start the timer, and if time >= 3sec.,
                    // then execute the setVC command.
                    m_objTimer = new javax.swing.Timer(THREE_SEC, new ActionListener() {
                        public void actionPerformed(ActionEvent e) {
                            if (!isToolDialogEditMode() && m_btn.isSelected())
                            {
                                String strSetCmd = m_btn.getAttribute(VObjDef.SET_VC);
                                if (strSetCmd != null)
                                    sendVnmrCmd(strSetCmd);
                                m_objTimer.stop();
                            }
                        }
                    });
                    m_objTimer.start();
                    /*if (m_strSubType != null && m_strSubType.equals(Global.VIEWPORT))
                      m_btn.setBorder(BorderFactory.createLoweredBevelBorder());*/
                }
            }

            public void mouseReleased(MouseEvent e) {
                if (m_objTimer != null)
                    m_objTimer.stop();
                VToolBarButton btn = (VToolBarButton) e.getSource();
                String cmd = btn.getAttribute(VObjDef.CMD);
                try
                {
                    if (!isToolDialogEditMode() && btn.isSelected() )
                    {
                        if (m_strSubType != null
                            && m_strSubType.indexOf(Global.VIEWPORT) < 0)
                        {
                            btn.setBorder(BorderFactory.createRaisedBevelBorder());
                        }
                        else if (m_strSubType != null)
                        {
                            String strIndex = m_strSubType.substring(Global.VIEWPORT.length());
                            int a = 0;
                            if (strIndex != null && strIndex.trim().length() > 0)
                            {
                                //Util.getVpActionHandler().fireAction(new ActionEvent(this, 0, strIndex));
                                a = Integer.parseInt(strIndex);
                            }
                            // the command should set viewport
                            if (cmd == null) {
                                if (a > 0)
                                    Util.getAppIF().setViewPort(a-1);
                            }

                        }
                        btn.setSelected(false);
                        if (cmd != null)
                            sendVnmrCmd(cmd);
                    }

                }
                catch (Exception ex)
                {
                    Messages.writeStackTrace(ex);
                    //ex.printStackTrace();
                    //Messages.postError(ex.toString());
                    sendVnmrCmd(cmd);
                }
            }

            public void mouseEntered(MouseEvent e) {
                if (!isToolDialogEditMode())
                {
                    VButton btn = (VButton) e.getSource();
                    btn.setSelected(true);
                }
            }

            public void mouseExited(MouseEvent e) {
                if (m_objTimer != null)
                    m_objTimer.stop();
                VButton btn = (VButton) e.getSource();
                if (!isToolDialogEditMode())
                {
                    btn.setSelected(false);
                    //btn.setFocusPainted(false);
                }
            }

        });
    }

    protected void sendVnmrCmd(String cmd) {
        ExpViewArea expViewArea = Util.getAppIF().expViewArea;
        if (cmd == null || expViewArea == null)
            return;

        expViewArea.sendToVnmr(cmd);
    }


    /**
     * Checks if the tools dialog is in edit mode.
     */
    protected boolean isToolDialogEditMode() {
        return (getToolsDialog() != null
                && getToolsDialog().getToolEditStatus());
    }

    public void setBtn()
    {
        String cmd = String.valueOf(Util.getActiveView().getViewId()+1);
        setBtn(cmd);
    }

    protected void setBtn(String cmd)
    {
        boolean bSubtype = false;
        if (m_strSubType != null && !m_strSubType.equals(""))
        {
            String strvp = m_strSubType.substring(Global.VIEWPORT.length());
            if (cmd != null && cmd.trim().length() > 0 &&
                cmd.indexOf(strvp) >= 0)
                bSubtype = true;
        }
        if (bSubtype)
            setBorder(BorderFactory.createLoweredBevelBorder());
        else
            setBorder(BorderFactory.createRaisedBevelBorder());
    }

    /**
     *  Resets the focus.
     */
    protected void resetFocus(JComponent objToolBar) {
        if (objToolBar == null)
            return;

        int nCount = objToolBar.getComponentCount();
        for (int i = 0; i < nCount; i++)
        {
            VToolBarButton objBtn
                    = (VToolBarButton)objToolBar.getComponent(i);
            objBtn.setFocusPainted(true);
            if (objBtn.isSelected())
                objBtn.setSelected(false);

        }
    }
}
