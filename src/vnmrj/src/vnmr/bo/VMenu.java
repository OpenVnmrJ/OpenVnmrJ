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
import java.util.*;
import java.beans.*;
import java.awt.dnd.*;
import java.awt.event.*;

import javax.swing.border.BevelBorder;
import javax.swing.event.*;
import java.io.*;
import javax.swing.*;
import javax.swing.plaf.*;
import javax.accessibility.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.bo.*;

// public class VMenu extends XJComboBox implements VGroupSave, VObjIF, VObjDef,
public class VMenu extends JComboBox implements VGroupSave, VObjIF, VObjDef,
        ComboBoxTitleIF, DropTargetListener, ExpListenerIF, PropertyChangeListener, VEditIF,
        ActionComponent {
    public String type = null;
    protected String titleStr = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String vnmrCmd2 = null;
    public String showVal = null;
    public String setVal = null;
    public String tipStr = null;
    protected String strSubtype = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String objName = null;
    public static String lfStr = null;
    public Color fgColor = null;
    public Color bgColor = Util.getBgColor();
    public Font font = null;
    public Font font2 = null;
    protected String keyStr = null;
    private MouseAdapter mlEditor;
    private MouseAdapter mlNonEditor;
    private RightsList rightsList=null;
    protected String m_helplink = null;
    protected boolean isEditing = false;
    protected boolean inEditMode = false;
    protected boolean isFocused = false;
    protected boolean bFocused = false; // editor focus status
    protected boolean inChangeMode = false;
    protected boolean cmdOk = false;
    protected boolean bNeedSetDef = false;
    protected boolean bMenuUp = false;
    protected boolean m_bParameter = false;
    protected boolean m_bShow = true;
    protected boolean bVpMenu = false;
    protected boolean inAddMode = false;
    protected boolean bSelMenu = false;
    protected boolean bListBoxAdded = false;
    protected VComboBoxUI menuUi;
    protected String m_strEditable = "No";
    protected ActionListener m_editorActionListener;
    protected FocusListener m_editorFocusListener;
    protected int isActive = 1;
    protected ButtonIF vnmrIf;
    protected SessionShare sshare;
    protected VMenuLabel mlabel;
    protected boolean inModalMode = false;
    protected ArrayList<VMenuLabel> m_aListLabel;
    protected boolean m_bDefault = true;
    /** The default choice string in the menu. */
    protected String m_strDefault = "";

    private FontMetrics fm = null;
    private float fontRatio = 1.0f;
    private int rWidth = 0;
    private int rWidth2 = 0;
    private int nWidth = 0;
    private int nHeight = 0;
    private int rHeight = 10;
    private int fontH = 0;
    private int fontRH = 0; // the height of FontMetrics
    protected int numActive = 1;
    private Dimension defDim = new Dimension(0, 0);
    private Dimension curDim = new Dimension(0, 0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private Insets myInset = new Insets(2, 2, 2, 2);
    private boolean panel_enabled = true;
    private boolean bInPopup = false;
    public static boolean bMetalUI = true;
    protected PopupMenuListener popupL = null;
    protected String m_viewport = null;
    private ComboBoxEditor cbEditor;
    public static int uiMinWidth = 28;

    /**
     * Name of local parameter tracked through the ActionCommand.
     * Accessible through set/getAttribute(PANEL_PARAM).
     */
    private String m_parameter = null;

    /**
     * ActionCommand used to handle value changes locally; not through Vnmr.
     * Accessible through set/getAttribute(ACTIONCMD).
     */
    private String m_actionCmd = null;

    /**
     * Java listeners that get sent the ActionCommand on value changes.
     */
    private Set<ActionListener> m_actionListenerList = new TreeSet<ActionListener>();

    protected final static String[] m_aStrEditable = { "Yes", "No" };
    /** The array of the attributes that are displayed in the edit template.*/
    protected final static Object[][] m_attributes = {
            { new Integer(LABEL), "Label of item:" },
            { new Integer(VARIABLE), "Vnmr variables:    " },
            { new Integer(SETVAL), "Value of item:" },
            { new Integer(SHOW), "Enable condition:" },
            { new Integer(CMD), "Vnmr command:" },
            { new Integer(SETCHOICE), "Label of Choices:" },
            { new Integer(SETCHVAL), "Value of Choices:" },
            { new Integer(EDITABLE), "Editable:", "radio", m_aStrEditable },
            { new Integer(TOOLTIP),  Util.getLabel(TOOLTIP) }
    };
    
    protected final static Object[][] m_attributes_H = {
        { new Integer(LABEL), "Label of item:" },
        { new Integer(VARIABLE), "Vnmr variables:    " },
        { new Integer(SETVAL), "Value of item:" },
        { new Integer(SHOW), "Enable condition:" },
        { new Integer(CMD), "Vnmr command:" },
        { new Integer(SETCHOICE), "Label of Choices:" },
        { new Integer(SETCHVAL), "Value of Choices:" },
        { new Integer(EDITABLE), "Editable:", "radio", m_aStrEditable },
        { new Integer(TOOLTIP),  Util.getLabel(TOOLTIP) },
        { new Integer(HELPLINK), Util.getLabel("blHelp")} 
};

    public VMenu(SessionShare ss, ButtonIF vif, String typ) {
        super();
        this.sshare = ss;
        this.type = typ;
        this.vnmrIf = vif;
        m_aListLabel = new ArrayList<VMenuLabel>();
        // super.addActionListener(this);
        // setBorder(BorderFactory.createBevelBorder(BevelBorder.RAISED));
        // setOpaque(true);

        ComboBoxUI ui;
/**
        if (VLookAndFeelFactory.isVjLookAndFeel()) {
            ui = (ComboBoxUI) new VComboMetalUI(this);
            setUI(ui);
        }
**/
        ui = getUI();
        if (ui != null && (ui instanceof VComboBoxUI))
            menuUi = (VComboBoxUI) ui;

        setBgColor(bg);

        setMaximumRowCount(12);
        mlEditor = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                if(inEditMode) {
                    int clicks = evt.getClickCount();
                    int modifier = evt.getModifiers();
                    if((modifier & (1 << 4)) != 0) {
                        if(clicks >= 2) {
                            if(!m_bParameter) {
                                Component comp = (Component)evt.getSource();
                                if(!(comp instanceof VObjIF))
                                    comp = comp.getParent();
                                if(comp instanceof VObjIF)
                                    ParamEditUtil.setEditObj((VObjIF)comp);
                            } else {
                                Component comp = ((Component)evt.getSource())
                                        .getParent();
                                if(comp instanceof VParameter)
                                    ParamEditUtil.setEditObj((VObjIF)comp);
                            }
                        }
                    }
                }
            }
            public void mouseReleased(MouseEvent evt) {
                if(inEditMode) {
                    Component comp = (Component)evt.getSource();
                    if(!(comp instanceof VObjIF))
                        comp = comp.getParent();
                    if(comp instanceof VObjIF)
                        ParamEditUtil.setEditObj((VObjIF)comp);
                }
            }
        };

        
        mlNonEditor = new CSHMouseAdapter();
    
        // Start with the non editor listener.  If we change to Editor mode
        // it will be changed.
        addMouseListener(mlNonEditor);
  
        addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent e) {
                if(e.getKeyCode() == KeyEvent.VK_ENTER) {
                    cmdOk = true;
                    doAction();
                    hidePopup();
                }
            }
        });

        m_editorActionListener = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                performEditorAction();
            }
        };

        m_editorFocusListener = new FocusAdapter() {
            public void focusGained(FocusEvent e)
            {
                bFocused = true;
                setFocusedObj();
            }

            public void focusLost(FocusEvent e) {
                bFocused = false;
                removeFocusedObj();
                performEditorAction();
            }
        };

        popupL = new PopupMenuListener() {
            public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
                bMenuUp = true;
                if (bListBoxAdded)
                    bInPopup = false;
                else
                    bInPopup = true;
            }

            public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
                bMenuUp = false;
                if (bInPopup)
                    execCmd();
            }

            public void popupMenuCanceled(PopupMenuEvent e) {
                bInPopup = false; 
            }
        };

        addPopupMenuListener(popupL);

        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
    }

    @Override
    public void setEditor(ComboBoxEditor anEditor) {
        super.setEditor(anEditor);
        if (anEditor != null) {
           if (isEditable())
               initEditor();
        }
    }

    private void setFgColor(String c) {
        fg = c;
    	if (c == null || c.length() == 0 || c.equals("default"))
             fgColor = UIManager.getColor("ComboBox.foreground");
    	else
    	     fgColor = DisplayOptions.getColor(c);
        Component editField= getEditor().getEditorComponent();
        if (editField != null)
            editField.setForeground(fgColor);
        setForeground(fgColor);
    }

    /**
     * Set the background color.
     * @param bg The color to set.
     */
    private void setBgColor(String bg) {
    	if(bg == null || bg.length() == 0 || 
    			bg.equals("transparent")||bg.equals("VJBackground"))
              bgColor = UIManager.getColor("ComboBox.background");
    	else
    	      bgColor = DisplayOptions.getColor(bg);
        setBackground(bgColor);
        // if (menuUi != null)
        //     ((VComboBoxUI)menuUi).setBgColor(bgColor);
    }

    /*******
    public Insets getInsets() {
        //return myInset;
        return new Insets(1, 1, 1, 1);
    }
    *******/

    public void setSelMode(boolean b) {
        bSelMenu = b;
    }

    public void updateUI() {
        bListBoxAdded = false;
        cbEditor = null;
        super.updateUI();
        menuUi = null;
        /****
         ComboBoxUI ui = getUI();
         if (ui != null && (ui instanceof VComboBoxUI)) {
              menuUi = (VComboBoxUI) ui;
         }
        ****/
    }

    /**
     *  Handles Key Events.
     */
    public void processKeyEvent(KeyEvent e) {
        super.processKeyEvent(e);

        // Make sure that the selected index is visible.
        if(isPopupVisible()) {
            int nItemx = getSelectedIndex();
            Accessible a = this.getUI().getAccessibleChild(this, 0);
            if(a != null && a instanceof javax.swing.plaf.basic.ComboPopup) {
                // get the popup list
                JList list = ((javax.swing.plaf.basic.ComboPopup)a).getList();
                if(list != null)
                    list.ensureIndexIsVisible(nItemx);
            }
        }
    }

    public boolean selectWithKeyChar(char keyChar) {
        boolean bItemMatched = super.selectWithKeyChar(keyChar);

        if(bItemMatched) {
            sendVnmrCmd();
            sendActionCmd();
        }
        return bItemMatched;
    }

    public void initEditor() {
        if(editor != null) {
            if (cbEditor == editor)
                return;
            cbEditor = editor;
            Component comp = editor.getEditorComponent();
            if (comp == null)
                return;
            if (!(comp instanceof JTextField))
                return;
            JTextField txfEditor = (JTextField)comp;
            
            /***
            if(!isEditorListenerAdded(txfEditor.getActionListeners(),
                    m_editorActionListener))
                editor.addActionListener(m_editorActionListener);
            ***/

            if(!isEditorListenerAdded(txfEditor.getFocusListeners(),
                    m_editorFocusListener))
                txfEditor.addFocusListener(m_editorFocusListener);
        }
    }

    protected boolean isEditorListenerAdded(EventListener[] arrEl,
            EventListener eventListener) {
        boolean bListener = false;
        int nSize = arrEl.length;
        for(int i = 0; i < nSize; i++) {
            EventListener el = arrEl[i];
            if(el == eventListener) {
                bListener = true;
                break;
            }
        }
        return bListener;
    }

    //----------------------------------------------------------------
    /** set the menu choice. */
    //----------------------------------------------------------------
    protected void setMenuChoice(String s) {
        if (s == null)
            return;
        int k = m_aListLabel.size();
        String s2;
        String strLabel;
        String strValue;
        int len = s.length();
        int found = -1;

        m_bDefault = false;
        if (len == 1) {
            if (s.charAt(0) == ' ')
               len = 0;
        }
        
        for(int n = 0; n < k; n++) {
            mlabel = m_aListLabel.get(n);
            s2 = mlabel.getAttribute(CHVAL);
            if (s2 != null) {
                if (len == 0) {
                   if (s2.length() == 2) {
                      if ((s2.charAt(0) == '\'') && (s2.charAt(0) == '\'')) {
                         found = n;
                         break;
                      }
                   }
                }
                if (s2.equals(s)) {
                   found = n;
                   break;
                }
            }
        }
        if (found >= 0) {
            if (found < getItemCount()) {
                setSelectedIndex(found);
                repaint();
            }
            return;
        }

        m_bDefault = true;
        if (!bSelMenu)
             m_strDefault = s;
        if (isEditable() && (editor != null)) {
            JTextField txfEditor = (JTextField)editor.getEditorComponent();
            txfEditor.setText(s);
            m_bDefault = false;
        }
        repaint();
        // int nBlankIndex = getNullChoice();
        /* if (nBlankIndex >= 0 && nBlankIndex < getItemCount()) {
         setSelectedIndex(nBlankIndex);
         }*/
    }

    private int getNullChoice() {
        VMenuLabel objLabel;
        String strLabel;
        for(int n = 0; n < m_aListLabel.size(); n++) {
            objLabel = m_aListLabel.get(n);
            strLabel = objLabel.getAttribute(LABEL);
            if(strLabel.trim().equals("") || strLabel.equals(" "))
                return n;
        }
        //addChoice("");
        return (getComponentCount() - 1);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
        changeFont();
        setFgColor(fg);
        setBgColor(bg);
        SwingUtilities.updateComponentTreeUI(this);
    }

    public void setDefLabel(String s) {
        this.titleStr = s;
        rWidth = 0;
    }

    public void setDefColor(String c) {
        setFgColor(c);
    }

    public void setShow(boolean bShow) {
        m_bShow = bShow;
    }

    public void checkAndShow() {
        if(getItemCount() <= 1) {
            String strObj = (String)getItemAt(0);
            if(strObj == null || strObj.equals("") || strObj.equals(" ")) {
                setVisible(false);
            } else
                setVisible(true);
        } else if(m_bShow)
            setVisible(true);
    }

    public boolean isRequestFocusEnabled() {
        if(!isEditable())
            return Util.isFocusTraversal();
        return super.isRequestFocusEnabled();
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        boolean oldMode = inEditMode;
        inEditMode = s;
        for(VMenuLabel label : m_aListLabel) {
            label.setEditMode(s);
        }
        if(s) {
            setVisible(true);
            // Be sure both are cleared out
            removeMouseListener(mlNonEditor);
            removeMouseListener(mlEditor);
            addMouseListener(mlEditor);
/*
            if(editor != null)
                editor.getEditorComponent().addMouseListener(ml);
*/
            defDim = getPreferredSize();
            if(font != null) {
                setFont(font);
                fontH = rHeight;
                fontRH = fm.getHeight();
                myInset.top = (defDim.height - fontRH) / 2;
                if(myInset.top < 0)
                    myInset.top = 0;
            }
            fontRatio = 1.0f;
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
            rWidth2 = rWidth;
        } else {
            checkAndShow();
            
            // Be sure both are cleared out
            removeMouseListener(mlEditor);
            removeMouseListener(mlNonEditor);
            addMouseListener(mlNonEditor);

            if(editor != null)
                editor.getEditorComponent().removeMouseListener(mlEditor);
            if(oldMode)
                adjustFont(curDim.width, curDim.height);
        }
    }

    private void setFocusedObj() {
        VObjUtil.setFocusedObj((VObjIF)this);
    }

    private void removeFocusedObj() {
        VObjUtil.setFocusedObj((VObjIF)this);
    }

    private void setDefSize() {
        if(rWidth <= 0)
            calSize();
        if (isPreferredSizeSet()) {
            defDim = getPreferredSize();
            if (defDim != null) {
                bNeedSetDef = false;
                return;
            }
        }

        defDim.width = rWidth + 6;
        defDim.height = rHeight + 8;
        setPreferredSize(defDim);
    }

    public void changeFont() {
        font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
        setFont(font);
        if(bVpMenu) {
            Util.updateVpMenuLabel();
            return;
        }
        rHeight = font.getSize();
        fontH = rHeight;
        fm = getFontMetrics(font);
        fontRH = fm.getHeight();
        font2 = null;
        calSize();
        fontRatio = 1.0f;
        if(!inEditMode) {
            if((curDim.width > 0) && (rWidth > curDim.width)) {
                if(bNeedSetDef) {
                    rWidth = 0;
                    setDefSize();
                }
                adjustFont(curDim.width, curDim.height);
            }
        }

        repaint();
    }

    public void changeFocus(boolean s) {
        if (!inEditMode) {
            if (!s && bFocused) {
               performEditorAction();
               bFocused = false;
            }
            return;
        }
        isFocused = s;
        repaint();
    }

    public Object[][] getAttributes() {
        VnmrjIF vif = Util.getVjIF();
        if (vif != null)
            rightsList = vif.getRightsList();
        if(rightsList != null && rightsList.isApproved("mayedithelpfield"))
            return m_attributes_H;
        else 
            return m_attributes;
    }

    public String getAttribute(int attr) {
        int k;
        String s;
        switch(attr) {
        case TYPE:
            return type;
        case KEYSTR:
            return keyStr;
        case LABEL:
            return titleStr;
        case BGCOLOR:
            return bg;
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
        case SETVAL:
            return setVal;
        case SUBTYPE:
            return strSubtype;
        case VARIABLE:
            return vnmrVar;
        case KEYVAL:
            if(keyStr == null)
                return null;
        case EDITABLE:
            return m_strEditable;
        case VALUE:
            k = getSelectedIndex();
            if(k >= 0 && k < m_aListLabel.size()) {
                mlabel = m_aListLabel.get(k);
                return mlabel.getAttribute(CHVAL);
            } else {
                String v = "";
                if (isEditable() && editor != null)
                    v = (String)editor.getItem();
                return v;
            }
        case SETCHOICE:
            s = "";
            for(k = 0; k < m_aListLabel.size(); k++) {
                mlabel = m_aListLabel.get(k);
                s += "\"" + mlabel.getAttribute(LABEL) + "\" ";
            }
            return s;
        case SETCHVAL:
            s = "";
            for(k = 0; k < m_aListLabel.size(); k++) {
                mlabel = m_aListLabel.get(k);
                String chval = mlabel.getAttribute(CHVAL);
                if(chval != null)
                    s += "\"" + chval + "\" ";
            }
            return s;
        case TOOL_TIP:
            return null;
        case TOOLTIP:
            return tipStr;
        case ACTIONCMD:
            return m_actionCmd;
        case PANEL_PARAM:
            return m_parameter;
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
        Vector v;
        switch(attr) {
        case TYPE:
            type = c;
            break;
        case KEYSTR:
            keyStr = c;
            break;
        case LABEL:
            inChangeMode = true;
            titleStr = c;
            if (bSelMenu) {
                if (menuUi != null)
                    ((VComboBoxUI)menuUi).setTitle(c);
                else
                    setSelectedItem(c);
            }
            else {
                setSelectedItem(c);
                for(int n = 0; n < m_aListLabel.size(); n++) {
                    mlabel = m_aListLabel.get(n);
                    String s = mlabel.getAttribute(LABEL);
                    if(s.equals(titleStr)) {
                        setSelectedIndex(n);
                    }
                }
            }
            inChangeMode = false;
            break;
        case FGCOLOR:
            // fgColor = DisplayOptions.getColor(fg);
            // setForeground(fgColor);
            setFgColor(c);
            repaint();
            break;
        case BGCOLOR:
            bg = c;
            setBgColor(bg);
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
        case VARIABLE:
            vnmrVar = c;
            break;
        case SETVAL:
            setVal = c;
            break;
        case SUBTYPE:
            strSubtype = c;
            if(strSubtype != null && strSubtype.equals("parameter"))
                m_bParameter = true;
            else
                m_bParameter = false;
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case SETCHOICE:
            inAddMode = true;
            v = ParamInfo.parseChoiceStr(c);
            Component comp;
            int m = 0;
            if(v != null)
                m = v.size();

            if(getItemCount() > 0) {
                removeAllItems();
                m_aListLabel.clear();

                //removeAll();
                ArrayList<Component> aListComp = new ArrayList<Component>();
                for(int i = 0; i < getComponentCount(); i++) {
                    comp = getComponent(i);
                    if(comp instanceof VObjIF)
                        aListComp.add(comp);
                }

                // Delete the vobjif components from the menu
                for(int i = 0; i < aListComp.size(); i++) {
                    remove(aListComp.get(i));
                }
            }
            if(v == null || m < 1) {
                inAddMode = false;
                return;
            }
            for(int n = 0; n < m; n++) {
                String val = (String)v.elementAt(n);
                addChoice(val);
                if(titleStr != null && val.equals(titleStr)) {
                    setSelectedIndex(n);
                }
            }
            v = null;
            inAddMode = false;
            rWidth = 0;
            break;
        case SETCHVAL:
            v = ParamInfo.parseChoiceStr(c);
            if(v == null)
                return;

            for(int k = 0; k < v.size(); k++) {
                if(k < m_aListLabel.size()) {
                    mlabel = m_aListLabel.get(k);
                    mlabel.setAttribute(CHVAL, (String)v.elementAt(k));
                }
            }
            break;
        case KEYVAL:
            titleStr = c;
            break;
        case EDITABLE:
            if(c != null)
                m_strEditable = c;
            boolean bEditable = false;
            if(c != null
                    && (c.equalsIgnoreCase("true") || c.equalsIgnoreCase("yes")))
                bEditable = true;
            setEditable(bEditable);
            if (bEditable)
                initEditor();
            break;
        case VALUE:
            inChangeMode = true;
            setMenuChoice(c);
            inChangeMode = false;
            break;
        case ENABLED:
            panel_enabled = c.equals("false") ? false : true;
            setEnabled(c.equals("false") ? false : true);
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

    public ArrayList getCompList() {
        return m_aListLabel;
    }

    public int getActiveNum() {
        if(bVpMenu)
            return numActive;
        return m_aListLabel.size();
    }

    protected VMenuLabel addChoice(String val) {
        addItem(val);
        mlabel = new VMenuLabel(sshare, vnmrIf, "mlabel");
        mlabel.setAttribute(LABEL, val);
        m_aListLabel.add(mlabel);
        //System.out.println("Adding new label " + val);
        super.add(mlabel);
        return mlabel;
    }

    public Component add(Component jcomp) {
        Component comp;
        inAddMode = true;
        if(jcomp instanceof VObjIF) {
            VObjIF obj = (VObjIF)jcomp;
            String item = obj.getAttribute(LABEL);
            if(item != null) {
                if(jcomp instanceof VMenuLabel) {
                    mlabel = (VMenuLabel)jcomp;
                    addItem(item);
                    m_aListLabel.add(mlabel);
                    super.add(jcomp);
                } else {
                    mlabel = addChoice(item);
                    mlabel.setAttribute(CHVAL, item);
                }
                if(titleStr != null && item.equals(titleStr))
                    setSelectedIndex(getItemCount() - 1);
                item = obj.getAttribute(CHVAL);
                if(titleStr != null && item != null && item.equals(titleStr))
                    setSelectedIndex(getItemCount() - 1);
                if(item != null && item.length() > 0)
                    mlabel.setAttribute(CHVAL, item);
            }
            //System.out.println("Added new label " + item);
            inAddMode = false;
            rWidth = 0;
            return mlabel;
        } else {
            comp = super.add(jcomp);
            inAddMode = false;
            rWidth = 0;
            return comp;
        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        if(vif != null) {
            if(vif == vnmrIf)
                return;
        }
        vnmrIf = vif;
        int k = getComponentCount();
        for(int i = 0; i < k; i++) {
            Component m = getComponent(i);
            if(m instanceof VObjIF)
                ((VObjIF)m).setVnmrIF(vif);
        }
        // updateValue();
    }

    public void setValue(ParamIF pf) {
        if(pf != null && pf.value != null) {
            inChangeMode = true;
            setMenuChoice(pf.value);
            inChangeMode = false;
        }
    }

    public void setShowValue(ParamIF pf) {
        if(pf != null && pf.value != null) {
            String s = pf.value.trim();
            isActive = Integer.parseInt(s);

            if(bVpMenu) {
                if(isActive > 0) {
                    numActive = isActive;
                    Util.setVpNum(numActive);
                }
                return;
            }
            if(panel_enabled && isActive > 0) {
                setEnabled(true);
                if(setVal != null) {
                    vnmrIf.asyncQueryParam(this, setVal);
                }
            } else {
                if(!panel_enabled && setVal != null && isActive >= 0)
                    vnmrIf.asyncQueryParam(this, setVal);
                setEnabled(false);
            }
        }
    }

    public void updateValue() {
        if (m_viewport != null && m_viewport.toLowerCase().startsWith("y")) {
            vnmrIf = Util.getActiveView();
        }
        if(vnmrIf == null)
            return;
        if(!inEditMode)
            checkAndShow();
        if(showVal != null)
            vnmrIf.asyncQueryShow(this, showVal);
        else if(setVal != null)
            vnmrIf.asyncQueryParam(this, setVal);
        if(bVpMenu) {
            int k = getComponentCount();
            for(int i = 0; i < k; i++) {
                Component m = getComponent(i);
                if(m instanceof VObjIF)
                    ((VObjIF)m).updateValue();
            }
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
    }

    public void setVpMenu(boolean b) {
        bVpMenu = b;
        int k = getComponentCount();
        for(int i = 0; i < k; i++) {
            Component m = getComponent(i);
            if(m instanceof VMenuLabel)
                ((VMenuLabel)m).setVpMenu(b);
        }
    }

    public void childChangeLabel() {
        if(bVpMenu) {
            Util.updateVpMenuLabel();
        }
    }

    public void setAdddMode(boolean b) {
        inAddMode = b;
    }

    public void childChangeStatus() {
        if(bVpMenu) {
            Util.updateVpMenuStatus();
        }
    }

    public void paint(Graphics g) {
        super.paint(g);
        if(!isEditing)
            return;
        Dimension psize = getSize();
        if(isFocused)
            g.setColor(Color.yellow);
        else
            g.setColor(Color.green);
        g.drawRect(0, 0, psize.width -1, psize.height -1);
        g.drawRect(1, 1, psize.width -3, psize.height -3);
    }

    public void refresh() {
    }

    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void addDefChoice(String c) {
        setAttribute(SETCHOICE, c);
    }

    public void addDefValue(String c) {
        setAttribute(SETCHVAL, c);
    }

    protected void doAction() {
        // sometimes this method will be called twice by one selection
        // use cmdOk to prohibit double action
        /*
         if (!cmdOk)
         return;
         */
        cmdOk = false;
        if(!bMenuUp)
            return;
        m_bDefault = false;
        /***************************
        if(inModalMode)
            return;
        if(inAddMode || inChangeMode || inEditMode || isLabelNull())
            return;
        if((vnmrCmd == null || vnmrIf == null) && m_actionCmd == null)
            return;
        if(isActive < 0)
            return;

        ***************************/

        if (!canDoAction())
            return;
        sendVnmrCmd();
        sendActionCmd();


        /*
         String strSelected = (String) getSelectedItem();
         String strChoice = null;
         if (mlabel != null)
         strChoice = mlabel.getAttribute(LABEL);
         vnmrIf.sendVnmrCmd(this, vnmrCmd);
         */
    }

    protected boolean canDoAction() {
        if (inModalMode)
            return false;
        if (inAddMode || inChangeMode || inEditMode || isLabelNull())
            return false;
        if ((vnmrCmd == null || vnmrIf == null) && m_actionCmd == null)
            return false;
        if (isActive < 0)
            return false;
        return true;
    }

    private void execCmd() {
        if (!bInPopup)
            return;
        bInPopup = false;

        if(inModalMode) { // select choice but not send to vnmr
            int k = getSelectedIndex();
            if(k >= 0 && k < m_aListLabel.size()) {
                mlabel = m_aListLabel.get(k);
		String choice = mlabel.getAttribute(CHVAL);
                setMenuChoice(choice);
	    }
            return;
	}

        if (!canDoAction()) 
            return;
        sendVnmrCmd();
        sendActionCmd();
    }


    public void actionPerformed(ActionEvent e) {
        bInPopup = true;
        execCmd();
    }


    public void enterPopup() {
         bInPopup = true;
    }

    public void exitPopup() {
         bInPopup = false;
    }

    public void listBoxMouseAdded() {
        bListBoxAdded = true;
    }

    protected void performEditorAction() {
        if(inModalMode || inAddMode || inEditMode)
            return;

        if (!isEditable() || editor == null)
            return;
        String strValue = (String)editor.getItem();
        String v = (String) getSelectedItem();
        if(strValue != null && !strValue.equals("")
                && !strValue.equals(v)) {
            int nLength = m_aListLabel.size();
            boolean bLabel = false;
            for(int i = 0; i < nLength; i++) {
                VMenuLabel menulabel = m_aListLabel.get(i);
                String strLabel = menulabel.getAttribute(LABEL);
                if(strValue.equals(strLabel)) {
                    bLabel = true;
                    break;
                }
            }
            if(!bLabel) {
                mlabel = addChoice(strValue);
                mlabel.setAttribute(CHVAL, strValue);
            }
            setSelectedItem(strValue);
            sendVnmrCmd();
            sendActionCmd();
        }
    }

    protected boolean isLabelNull() {
        String strLabel = (String)getSelectedItem();
        return (strLabel != null && strLabel.equals(""));
    }

    public void writeValue(PrintWriter fd, int gap) {
        int k, m;
        m = m_aListLabel.size();
        if(m <= 0)
            return;
        for(int i = 0; i < m; i++) {
            mlabel = m_aListLabel.get(i);
            fd.print("choice=\"" + mlabel.getAttribute(LABEL) + "\"");
            fd.println(" chval=\"" + mlabel.getAttribute(CHVAL) + "\"");
            for(k = 0; k < gap + 1; k++)
                fd.print("  ");
        }
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
    }

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if(vnmrCmd == null || vnmrIf == null)
            return;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    /**
     * Send an ActionEvent to all the action listeners, only if the
     * actionCmd has been set.
     */
    protected void sendActionCmd() {
        if(m_actionCmd != null) {
            String value = getAttribute(VALUE);
            ActionEvent event = new ActionEvent(this, hashCode(), m_actionCmd);
            for(ActionListener listener : m_actionListenerList) {
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

    public void setSizeRatio(double x, double y) {
        double xRatio = x;
        double yRatio = y;
        if(x > 1.0)
            xRatio = x - 1.0;
        if(y > 1.0)
            yRatio = y - 1.0;
        if(defDim.width <= 0) {
            if(!isPreferredSizeSet()) {
                bNeedSetDef = true;
                setDefSize();
            } else
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
        if(fm == null) {
            font = getFont();
            if (font == null) {
                font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
                if (font == null)
                    return;
            }
            fm = getFontMetrics(font);
            rHeight = font.getSize();
        }
        rWidth = 6;
        //	Dimension ss = getMinimumSize();
        int fw;
        for(int n = 0; n < m_aListLabel.size(); n++) {
            mlabel = m_aListLabel.get(n);
            String str = mlabel.getAttribute(LABEL);
            if(str != null) {
                fw = fm.stringWidth(str);
                if(fw > rWidth)
                    rWidth = fw;
            }
        }
        rWidth += uiMinWidth;
        rWidth2 = rWidth;
    }

    public void setBounds(int x, int y, int w, int h) {
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
        if(!inEditMode) {
            if(rWidth <= 0) {
                calSize();
            }
            if((w != nWidth) || (h != nHeight) || (w < rWidth2)) {
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

    /**
     *  Returns true if the value does not match the values in the combobox,
     *  false otherwise. If it's true then the ui would show the default titleStr
     *  as the title of the combobox.
     */
    public boolean getDefault() {
        return m_bDefault;
    }

    public void setDefault(boolean b) {
        m_bDefault = b;
    }

    public String getTitleLabel() {
        if (bSelMenu)
            return titleStr;
        return null;
    }

    /**
     *  Returns the default titleStr string to show as the title of the combobox.
     */
    public String getDefaultLabel() {
        return m_strDefault;
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

    public void adjustFont(int w, int h) {
        Font curFont = null;

        if(w <= 0)
            return;
        if(bVpMenu)
            return;
        if (font == null) {
            calSize();
            if (font == null)
                return;
        }
        nWidth = w;
        nHeight = h;
        if(fontRH > 0) {
            myInset.top = (h - fontRH) / 2;
            if(myInset.top < 0)
                myInset.top = 0;
        }
        h -= 4;
        if(w > rWidth2) {
            if((fontRatio >= 1.0f) && (h > fontH) && (fontH >= rHeight))
                return;
        }
        float s = (float)w / (float)rWidth;
        float fh = (float) h;
        if(rWidth > w) {
            if(s > 0.98f)
                s = 0.98f;
            if(s < 0.5f)
                s = 0.5f;
        }
        if(s > 1.0f)
            s = 1.0f;
        if((s == fontRatio) && (h > fontH) && (fontH >= rHeight))
            return;
        fontRatio = s;
        s = (float)rHeight * fontRatio;
        if(s >= fh)
            s = fh - 1.0f;
        if((s < 9.0f) && (rHeight > 9))
            s = 9.0f;
        if(fontRatio < 1.0f) {
            if(font2 == null) {
                String fname = font.getName();
                if(!fname.equals("Dialog"))
                    //font2 = new Font("Dialog", font.getStyle(), rHeight);
                    font2 = DisplayOptions.getFont("Dialog", font.getStyle(),
                            rHeight);
                else
                    font2 = font;
            }
            if(s < (float)rHeight)
                s += 1;
            //curFont = font2.deriveFont(s);
            curFont = DisplayOptions.getFont(font2.getName(), font2.getStyle(),
                    (int)s);
        } else {
            //curFont = font.deriveFont(s);
            curFont = DisplayOptions.getFont(font.getName(), font.getStyle(),
                    (int)s);
        }
        String strfont = curFont.getName();
        int nstyle = curFont.getStyle();
        FontMetrics fm2 = getFontMetrics(curFont);
        while(s > 9.0f) {
            rWidth2 = 0;
            int fw = 0;
            for(int n = 0; n < m_aListLabel.size(); n++) {
                mlabel = m_aListLabel.get(n);
                String str = mlabel.getAttribute(LABEL);
                if(str != null) {
                    fw = fm2.stringWidth(str);
                    if(fw > rWidth2)
                        rWidth2 = fw;
                }
            }
            rWidth2 += uiMinWidth;
            if(rWidth2 <= nWidth)
                break;
            if(s < 10.0f)
                break;
            s = s - 1.0f;
            //curFont = curFont.deriveFont(s);
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
            fm2 = getFontMetrics(curFont);
        }
        fontH = curFont.getSize();
        if(rWidth2 > w)
            rWidth2 = w;
        fontRH = fm2.getHeight();
        myInset.top = (nHeight - fontRH) / 2;
        if(myInset.top < 0)
            myInset.top = 0;
        setFont(curFont);
    }

    public static void setMinWidth(int n) {
         uiMinWidth = n;
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
                helpMenu.show(VMenu.this, (int)pt.getX(), (int)pt.getY());

            }
             
        }
    }  /* End CSHMouseAdapter class */

}
