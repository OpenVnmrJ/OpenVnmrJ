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
import java.awt.event.*;
import java.util.*;
import java.io.*;
import java.beans.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.plaf.basic.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VSubMenuItem extends JMenuItem implements VObjIF, VObjDef,
        ExpListenerIF, VMenuitemIF, ActionListener, PropertyChangeListener {

    public String type = null;
    public String label = null;
    public String fg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String showVal = null;
    public String setVal = null;
    public String seperatorStr = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String hotkey = null;
    public String watchObjName = null;
    public String showObjName = null;
    public String tipStr = null;
    public Color fgColor = null;
    public Color bgColor=null;
    public Font font = null;
    public String iconName = null;

    protected ImageIcon vicon = null;
    protected String keyStr = null;
    protected MouseAdapter ml;
    protected boolean isEditing = false;
    protected boolean inEditMode = false;
    protected boolean isFocused = false;
    protected boolean inChangeMode = false;
    protected boolean toUpdate = false;
    protected boolean bMainMenuBar = false;
    protected boolean bLastWatchItem = false;
    protected int isActive = 1;
    protected int oldActive = -99;
    protected ButtonIF vnmrIf;
    protected Container vparent;
    protected SessionShare sshare;
    private boolean inModalMode = false;
    private boolean bCheckMark = false;
    private boolean bShowCheckMark = false;
    private int iconW = 0;
    private int iconH = 0;
    private ImageIcon checkIcon = null;
    private JSeparator separator;
    private VnmrjIF vjIf = null;
    private VSubMenu shownListener = null;

    public VSubMenuItem(SessionShare ss, ButtonIF vif, String typ) {
        this.sshare = ss;
        this.type = typ;
        this.vnmrIf = vif;
        bgColor = Util.getMenuBg();
        setBackground(bgColor);
        addActionListener(this);

        setUI(new VSubMenuItemUI());

        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                if(inEditMode) {
                    int clicks = evt.getClickCount();
                    int modifier = evt.getModifiers();
                    if((modifier & (1 << 4)) != 0) {
                        if(clicks >= 2) {
                            ParamEditUtil.setEditObj((VObjIF)evt.getSource());
                        }
                    }
                }
            }
        };
        DisplayOptions.addChangeListener(this);
    }

    // PropertyChangeListener interface
    public void propertyChange(PropertyChangeEvent evt) {
    	if(fontStyle != null)
    		fg=fontStyle;
        if(fg != null) {
            fgColor = DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        if(bMainMenuBar)
            bgColor = Util.getMenuBarBg();
        else
            bgColor = Util.getMenuBg();
        setBackground(bgColor);
        changeFont();
    }

    public void setVParent(Container p) {
        vparent = p;
    }

    public void destroy() {
    }

    public Container getVParent() {
        return vparent;
    }

    public void changeLook() {
        fgColor = DisplayOptions.getColor(fg);
        setForeground(fgColor);
        changeFont();
    }

    public void setDefLabel(String s) {
        this.label = s;
        setText(s);
    }

    public void setDefColor(String c) {
        this.fg = c;
        fgColor = VnmrRgb.getColorByName(c);
        setForeground(fgColor);
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        inEditMode = s;
        if(s) {
            addMouseListener(ml);
        } else {
            removeMouseListener(ml);
        }
    }

    public void changeFont() {
        font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
        setFont(font);
        repaint();
    }

    public void changeFocus(boolean s) {
        repaint();
    }

    public String getAttribute(int attr) {
        switch(attr) {
        case TYPE:
            return type;
        case KEYSTR:
            return keyStr;
        case LABEL:
            return label;
        case ICON:
            return iconName;
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
        case SEPERATOR:
            return seperatorStr;
        case VARIABLE:
            return vnmrVar;
        case KEYVAL:
            return keyStr;
        case VALUE:
            return null;
        case SETCHOICE:
            return null;
        case SETCHVAL:
            return null;
        case HOTKEY:
            return hotkey;
        case CHECKMARK:
            if(bCheckMark)
                return "yes";
            else
                return "no";
        case CHECKOBJ:
            return watchObjName;
        case SHOWOBJ:
            return showObjName;
        case TOOL_TIP:
            return tipStr;
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        switch(attr) {
        case ICON:
            if(c != null)
                iconName = c.trim();
            else
                iconName = null;
            if(c == null || iconName.length() <= 0) {
                iconName = null;
                setIcon(null);
                return;
            }
            vicon = Util.getGeneralIcon(iconName);
            if(vicon != null)
                setIcon(vicon);
            break;

        case TYPE:
            type = c;
            break;
        case KEYSTR:
            keyStr = c;
            break;
        case LABEL:
            label = c;
            /*
             * if (ExperimentIF.isKeyBinded(label))
             * setAccelerator(KeyStroke.getKeyStroke(ExperimentIF.getKeyBinded(label)));
             */
            vjIf = Util.getVjIF();
            if(vjIf != null) {
                if(vjIf.isKeyBinded(label)) {
                    setAccelerator(KeyStroke.getKeyStroke(vjIf
                            .getKeyBindedForMenu(label)));
                }
            }
            setText(c);
            break;
        case FGCOLOR:
        	if(fontStyle==null){
	            fg = c;
	            fgColor = DisplayOptions.getColor(fg);
	            setForeground(fgColor);
	            //repaint();
            }
            break;
        case SHOW:
            showVal = c;
            break;
        case FONT_NAME:
            fontName = c;
            break;
        case FONT_STYLE:
            fg = c;
            fgColor = DisplayOptions.getColor(fg);
            setForeground(fgColor);
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
        case SEPERATOR:
            seperatorStr = c;
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case HOTKEY:
            hotkey = c;
            char a = hotkey.charAt(0);
            setMnemonic(a);
            break;
        case CHECKMARK:
            bCheckMark = false;
            if(c != null) {
                if(c.equals("yes") || c.equals("Yes"))
                    bCheckMark = true;
            }
            if(bCheckMark && checkIcon == null) {
                // checkIcon = Util.getGeneralIcon("check12.gif");
                checkIcon = Util.getGeneralIcon("blueCheck.gif");
                if(checkIcon == null)
                    bCheckMark = false;
                else {
                    iconW = checkIcon.getIconWidth();
                    iconH = checkIcon.getIconHeight();
                }
            }
            break;
        case CHECKOBJ:
            watchObjName = c;
            if(vjIf == null)
                vjIf = Util.getVjIF();
            if(vjIf != null) {
                if(c != null)
                    vjIf.addToolbarListener((Component)this);
                else
                    vjIf.removeToolbarListener((Component)this);
            }
            break;
        case SHOWOBJ:
            showObjName = c;
            break;
        case TOOL_TIP:
        case TOOLTIP:
            tipStr = c;
            if (c != null && c.length() > 0)
               setToolTipText(Util.getLabel(c));
            else
               setToolTipText(null);
            break;
        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public boolean setShownListener(VSubMenu obj) {
        if (showVal == null)
            return false;
        shownListener = obj;
        bLastWatchItem = false;
        return true;
    }

    public void setLastShownObj(boolean b) {
        bLastWatchItem = b;
    }

    public void setValue(ParamIF pf) {
        if(pf != null && pf.value != null) {
            inChangeMode = true;
            int check = Integer.parseInt(pf.value);
            if(check > 0) {
                setHorizontalTextPosition(SwingConstants.TRAILING);
                setHorizontalAlignment(SwingConstants.LEFT);
                // setIcon(Util.getImageIcon("vcheck8.gif"));
                setIcon(Util.getImageIcon("blueCheck.gif"));
            } else
                setIcon(null);
            inChangeMode = false;
        }
    }

    public void setSeparator(JSeparator obj) {
        separator = obj;
    }

    public void setShowValue(ParamIF pf) {
        String cmd = null;

        if (pf == null || pf.value == null)
           return;

        String s = pf.value.trim();
        boolean bVis = isVisible();

            isActive = Integer.parseInt(s);
            if(isActive > 0) {
                setEnabled(true);
            } else {
                setEnabled(false);
            }
            if(isActive >= 0) {
              setVisible(true);
              if (separator != null)
                 separator.setVisible(true);
            } else {
              setVisible(false);
              if (separator != null)
                 separator.setVisible(false);
            }
	    if(showObjName != null && (isActive != oldActive)) {
               if (isActive > 0)    
	             cmd = "vnmrjcmd('toolpanel','"+showObjName+"','show')";
               else
	             cmd = "vnmrjcmd('toolpanel','"+showObjName+"','close')";
	       vnmrIf.sendVnmrCmd(this,cmd);
            }
            oldActive = isActive;

        if (shownListener != null) {
            if (bVis != isVisible())
               shownListener.childShownEvent(true);
            if (bLastWatchItem)
               shownListener.endShownEvent();
        }
    }

    public void updateValue() {
        toUpdate = true;
        updateShow();
        updateMe();
        if(watchObjName != null || showObjName != null) {
            if(vjIf == null)
                vjIf = Util.getVjIF();
            if(vjIf != null) {
                if(watchObjName != null)
                    bShowCheckMark = vjIf.checkObject(watchObjName, this);
                if(showObjName != null) {
                    boolean bShow = vjIf.checkObjectExist(showObjName, this);
                    setVisible(bShow);
                    if (separator != null)
                       separator.setVisible(bShow);
                }
            }
        }
    }

    public void updateShow() {
        if((vnmrIf != null) && (showVal != null))
            vnmrIf.asyncQueryShow(this, showVal);
    }

    public void updateMe() {
        if((!toUpdate) || (vnmrIf == null))
            return;
        toUpdate = false;
        if(setVal != null)
            vnmrIf.asyncQueryParam(this, setVal);
    }

    public void initItem() {
        updateShow();
        toUpdate = true;
    }

    public void updateValue(Vector params) {
        StringTokenizer tok;
        String vars, v;
        int pnum = params.size();

        if(vnmrVar == null)
            return;
        tok = new StringTokenizer(vnmrVar, " ,\n");
        while(tok.hasMoreTokens()) {
            v = tok.nextToken();
            for(int k = 0; k < pnum; k++) {
                if(v.equals(params.elementAt(k))) {
                    updateShow();
                    toUpdate = true;
                    if(vparent != null && (vparent instanceof VSubMenu)) {
                        VSubMenu sm = (VSubMenu)vparent;
                        sm.updateLater();
                    } else {
                        updateMe();
                    }
                    tok = null;
                    return;
                }
            }
        }
        tok = null;
    }

    public void refresh() {
    }

    public void addDefChoice(String c) {
        setAttribute(SETCHOICE, c);
    }

    public void addDefValue(String c) {
        setAttribute(SETCHVAL, c);
    }

    public void actionPerformed(ActionEvent e) {
        if(inModalMode || vnmrIf == null)
            return;
        if(inChangeMode || inEditMode || vnmrCmd == null)
            return;
        if(isActive < 0)
            return;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    public void setDefLoc(int x, int y) {
    }

    public void writeValue(PrintWriter fd, int gap) {
    }

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if(vnmrCmd == null || vnmrIf == null)
            return;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    public void mainMenuBarItem(boolean b) {
        bMainMenuBar = b;
        if(bMainMenuBar)
            bgColor = Util.getMenuBarBg();
        else
            bgColor = Util.getMenuBg();
        setBackground(bgColor);
    }

    public Point getDefLoc() {
        return getLocation();
    }

    public Dimension getPreferredSize() {
        Dimension d = super.getPreferredSize();
        if(!bCheckMark)
            return d;
        if(checkIcon != null)
            d.width = d.width + checkIcon.getIconWidth() + 4;
        return d;
    }

    public void paint(Graphics g) {
        int x = 0;
        if(bCheckMark) {
            x = iconW + 4;
            g.translate(x, 0);
        }
        super.paint(g);
        if(!bCheckMark)
            return;
        Dimension d = getSize();
        int y = (d.height - iconH) / 2;
        g.translate(-x, 0);
        g.setColor(getBackground());
        g.fillRect(0, 0, x + 4, d.height);
        /*
         * if (isArmed()) { if (selectBg == null) selectBg =
         * UIManager.getColor("MenuItem.selectionBackground"); if (selectBg ==
         * null) selectBg = getBackground().darker(); g.setColor(selectBg);
         * g.fillRect(0,0, iconW+5, d.height-1); }
         */
        if(bShowCheckMark)
            g.drawImage(checkIcon.getImage(), 2, y, iconW + 2, iconH + y, 0, 0,
                    iconW, iconH, null);
    }

    public void setSizeRatio(double x, double y) {
    }

    class VSubMenuItemUI extends BasicMenuItemUI {
        protected ChangeListener createChangeListener(JComponent c) {
            return new MenuItemChangeHandler();
        }

        protected void installListeners() {
            super.installListeners();
            ChangeListener changeListener = createChangeListener(menuItem);
            menuItem.addChangeListener(changeListener);
        }

        protected class MenuItemChangeHandler implements ChangeListener {

            public void stateChanged(ChangeEvent e) {
                JMenuItem c = (JMenuItem)e.getSource();
                if(c.isArmed() || c.isSelected()) {
                    c.setBorderPainted(true);
                } else {
                    c.setBorderPainted(false);
                }
            }
        }
    }

}
