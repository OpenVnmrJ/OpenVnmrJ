/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.beans.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VCheckBoxMenuItem extends JCheckBoxMenuItem implements VObjIF, VObjDef,
                                    ExpListenerIF, VMenuitemIF, ActionListener, PropertyChangeListener
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
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String hotkey = null;
    public Color  fgColor = null;
    public Color  bgColor, orgBg;
    public Font   font = null;
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
    protected ButtonIF vnmrIf;
    protected Container vparent;
    protected SessionShare sshare;
    private boolean inModalMode = false;
    private JSeparator separator;
    private VSubMenu shownListener = null;


    public VCheckBoxMenuItem(SessionShare ss, ButtonIF vif, String typ)
    {
        this.sshare = ss;
        this.type = typ;
        this.vnmrIf = vif;
        orgBg = getBackground();
        bgColor = Util.getBgColor();
        setBackground(bgColor);
        addActionListener(this);

        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                if (inEditMode) {
                    int clicks = evt.getClickCount();
                    int modifier = evt.getModifiers();
                    if ((modifier & (1 << 4)) != 0) {
                        if (clicks >= 2) {
                            ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                        }
                    }
                }
            }
        };
        DisplayOptions.addChangeListener(this);

    }

    // PropertyChangeListener interface
    public void propertyChange(PropertyChangeEvent evt)
    {
        if (fg != null)
        {
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        changeFont();
        if (bMainMenuBar)
            bgColor = Util.getMenuBarBg();
        else
            bgColor = Util.getBgColor();
        setBackground(bgColor);
    }

    public void setVParent(Container p) {
        vparent = p;
    }

    public void destroy() {}

    public void setSeparator(JSeparator obj) {
        separator = obj;
    }

    public Container getVParent() {
        return vparent;
    }

    public void changeLook() {
        fgColor=DisplayOptions.getColor(fg);
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
        if (s) {
            addMouseListener(ml);
        }
        else {
            removeMouseListener(ml);
        }
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        repaint();
    }

    public void changeFocus(boolean s) {
        repaint();
    }

    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
        case KEYSTR:
            return keyStr;
        case LABEL:
            return label;
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
        case CMD:
            return vnmrCmd;
        case CMD2:
            return vnmrCmd2;
        case SETVAL:
            return setVal;
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
         default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case KEYSTR:
              keyStr=c;
            break;
        case LABEL:
            label = c;
/*
        if (ExperimentIF.isKeyBinded(label))
            setAccelerator(KeyStroke.getKeyStroke(ExperimentIF.getKeyBinded(label)));
*/
            VnmrjIF  vjIf = Util.getVjIF();
            if (vjIf != null) {
                if (vjIf.isKeyBinded(label))
                   setAccelerator(KeyStroke.getKeyStroke(vjIf.getKeyBindedForMenu(label)));
            }
        setText(c);
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            repaint();
            break;
        case BGCOLOR:
            bg = c;
            if (c == null || c.length()>0 || c.equals("default")){
                if (bMainMenuBar)
                    bgColor = Util.getMenuBarBg();
                else
                    bgColor = Util.getBgColor();
            }
            else {
               bgColor = DisplayOptions.getColor(c);
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
        case VARIABLE:
            vnmrVar = c;
            break;
        case SETVAL:
            setVal = c;
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case CMD2:
            vnmrCmd2 = c;
            break;
        case HOTKEY:
            hotkey = c;
            char a = hotkey.charAt( 0 );
            setMnemonic(a);
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
        if (pf != null && pf.value != null) {
            inChangeMode = true;
            int check = 0;
            try {
                check = Integer.parseInt(pf.value);
            } catch (NumberFormatException nfe) {
                Messages.postDebug("Bad int in VCheckBoxMenuItem.setValue(): "
                                   + pf.value);
                Messages.writeStackTrace(nfe);
            }
            if (check > 0) {
                setState(true);
            }
            else
                setState(false);
            inChangeMode = false;
        }
    }

    public void setShowValue(ParamIF pf) {
        if (pf == null || pf.value == null)
           return;

        String  s = pf.value.trim();
        boolean bVis = isVisible();
            isActive = Integer.parseInt(s);
            if (isActive > 0) {
                setEnabled(true);
                setBackground(bgColor);
            }
            else {
                setEnabled(false);
                setBackground(Global.NPCOLOR);
            }
            if (isActive >= 0) {
                setVisible(true);
                if (separator != null)
                   separator.setVisible(true);
            }
            else {
                setVisible(false);
                if (separator != null)
                   separator.setVisible(false);
            }

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
    }

    public void updateShow() {
        if ((vnmrIf != null) && (showVal != null))
            vnmrIf.asyncQueryShow(this, showVal);
    }

    public void updateMe() {
        if ((!toUpdate) || (vnmrIf == null))
            return;
        toUpdate = false;
        if (setVal != null)
            vnmrIf.asyncQueryParam(this, setVal);
    }

    public void initItem() {
        updateShow();
        toUpdate = true;
    }

    public void updateValue (Vector params) {
        StringTokenizer tok;
        String          vars, v;
        int             pnum = params.size();

        if (vnmrVar == null)
            return;
        tok = new StringTokenizer(vnmrVar, " ,\n");
        while (tok.hasMoreTokens()) {
            v = tok.nextToken();
            for (int k = 0; k < pnum; k++) {
                if (v.equals(params.elementAt(k))) {
                    updateShow();
                    toUpdate = true;
                    if (vparent != null && (vparent instanceof VSubMenu)) {
                        VSubMenu sm = (VSubMenu) vparent;
                        sm.updateLater();
                    }
                    else {
                        updateMe();
                    }
                    tok = null;
                    return;
                }
            }
        }
        tok = null;
    }

    public void mainMenuBarItem(boolean b) {
        bMainMenuBar = b;
        if (bMainMenuBar)
            bgColor = Util.getMenuBarBg();
        else
            bgColor = Util.getBgColor();
        setBackground(bgColor);
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
       if (inChangeMode ||  inEditMode)
           return;
       if (isActive < 0)
           return;
       String cmd = getState() ? vnmrCmd : vnmrCmd2;
       if (cmd != null)
           vnmrIf.sendVnmrCmd(this, cmd);
   }

   public void setDefLoc(int x, int y) {}

   public void writeValue(PrintWriter fd, int gap) {
   }

   public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    public Point getDefLoc() {
        return getLocation();
    }

    public void setSizeRatio(double x, double y) {}


}
