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
import java.awt.event.*;
import java.util.*;
import java.beans.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.border.*;
import javax.swing.plaf.basic.*;
import javax.swing.plaf.metal.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VSubMenu extends JMenu
    implements VObjIF, VObjDef, ExpListenerIF, VMenuitemIF, PropertyChangeListener
{
    public String type = null;
    public String label = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String showVal = null;
    public String setVal = null;
    public String seperatorStr = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String hotkey = null;
    public String iconName = null;
    public String tipStr = null;
    public Color  fgColor = Color.black;
    public Color  bgColor, orgBg;
    public Color  inColor = null;
    public Color  disabledFg = null;
    public Font   font = null;
    protected String keyStr = null;
    protected String objName = null;
    protected boolean isEditing = false;
    protected boolean inEditMode = false;
    protected boolean isFocused = false;
    protected boolean toUpdate = true;
    protected boolean updateChild = true;
    protected boolean  bMainMenuBar = false;
    protected boolean  bBorder = false;
    protected boolean  bPopup = false;
    protected boolean  bButtonMenu = false;
    protected boolean  bEntered = false;
    protected boolean  bPressed = false;
    protected boolean  bMenuUp = false;
    protected boolean  bVerbose = false;
    protected boolean  bSubMenuParent = false;
    protected boolean  bMenuBarParent = false;
    protected boolean  bHiFiUI = false;
    protected boolean  bLastWatchItem = false;
    protected boolean  bShownEvent = false;
    protected int  bMouseIn = 0;
    protected int isActive = 1;
    protected int iconW = 0;
    protected int iconH = 0;
    protected int labelW = 0;
    protected int labelH = 0;
    protected int mouseX = 0;
    protected int fontH = 10;
    protected int fontAscent = 10;
    protected int markXpnts[] = null;
    protected int markYpnts[] = null;
    protected double lastCmdTime = 0;
    protected ImageIcon iconImg = null;
    protected FontMetrics fm = null;
    protected ButtonIF vnmrIf;
    protected SessionShare sshare;
    protected Container vparent = null;
    // protected XMenuButton menuButton = null;
    protected VButton menuButton = null;
    // protected MetalComboBoxIcon menuIcon = null;
    protected Icon menuIcon = null;
    private JSeparator separator;
    private VSubMenu shownListener;

    public VSubMenu(SessionShare ss, ButtonIF vif, String typ) {
	this.sshare = ss;
	this.type = typ;
	this.vnmrIf = vif;
	orgBg = getBackground();
	bgColor = Util.getBgColor();
        inColor = Util.changeBrightness(bgColor, 10);
	setBackground(bgColor);
        // setUI(new VSubMenuUI());

	JPopupMenu popup = getPopupMenu();
	popup.addPopupMenuListener(new PopupMenuListener() {
                public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
		    helpRedraw((JComponent)e.getSource());
                    bPopup = true;
                    if (isActive > 0)
                        bPressed = true;
                    if (bVerbose)
                        System.out.println(objName+" popup ");
		    if (toUpdate) {
			updateMe();
		    }
		    if (updateChild) {
			updateChildren();
		    }
                }
                public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
                    bPopup = false;
                    bPressed = false;
                    if (!bEntered) {
                        bBorder = false;
                        bMouseIn = 0;
                        setBackground(bgColor);
/*
                        if (menuButton != null)
                           menuButton.setBackground(bgColor);
*/
                    }
		    if (vparent == null || (!(vparent instanceof VSubMenu)))
                    	Util.setMenuUp(false);
                }
                public void popupMenuCanceled(PopupMenuEvent e) {
                    bPressed = false;
                }
         });

	 DisplayOptions.addChangeListener(this);
    }

    public void setSeparator(JSeparator s) {
         separator = s;
    }

    private void addItemSeparator(Component comp) {
         JPopupMenu pop = getPopupMenu();
         if (pop == null) {
              addSeparator();
              return;
         }

         JSeparator obj = new JPopupMenu.Separator();
         pop.add(obj);
         ((VMenuitemIF)comp).setSeparator(obj);
    }

    public Component add(Component comp)
    {
        String str = null;
        if (comp instanceof VMenuitemIF) {
            str = ((VMenuitemIF)comp).getAttribute(SEPERATOR);
            ((VMenuitemIF)comp).mainMenuBarItem(bMainMenuBar);
        }
        if (str != null && str.equalsIgnoreCase("yes"))
        {
            addItemSeparator(comp);
        }
        return super.add(comp);
    }


    public void propertyChange(PropertyChangeEvent evt)
    {
        if (fg != null)
	{
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
	}
	changeFont();
        if (bButtonMenu) {
            bgColor = Util.getToolBarBg();
        }
        else {
            if (bMainMenuBar)
                bgColor = Util.getMenuBarBg();
            else
                bgColor = Util.getBgColor();
        }
        inColor = Util.changeBrightness(bgColor, 10);
	setBackground(bgColor);
        if (bButtonMenu && DisplayOptions.isUpdateUIEvent(evt)) {
            JButton btn = Util.getArrowButton();
            if (btn != null) {
                Icon icon = btn.getIcon();
                if (icon != null)
                     menuIcon = icon;
            }
            bHiFiUI = VLookAndFeelFactory.isHiFiLAF();
        }
    }

    private void helpRedraw(JComponent c) {
	Container p = getParent();
	if (p != null && (p instanceof JMenuBar))
             Util.setMenuUp(true, c);
	else
             Util.setMenuUp(true);
    }

    private void setMenuChoice(String s) {
    }

    public void addDefChoice(String s) {}

    public void addDefValue(String s) {}

    public void setVParent(Container p) {
        vparent = p;
        bMenuBarParent = false;
        bSubMenuParent = false;
        if (vparent != null) {
            if (vparent instanceof JMenuBar)
                bMenuBarParent = true;
            else if (vparent instanceof VSubMenu)
                bSubMenuParent = true;
        }
    }

    public Container getVParent() {
        return vparent;
    }

    public void updateLater() {
	updateChild = true;
	if (bSubMenuParent) {
	    VSubMenu sm = (VSubMenu) vparent;
	    sm.updateLater();
	}
    }

    // PropertyChangeListener interface

    public void changeLook() {
        int m = getMenuComponentCount();
	for (int k = 0; k < m; k++) {
	    Component comp = getMenuComponent(k);
	    if (comp instanceof VMenuitemIF) {
		VMenuitemIF obj = (VMenuitemIF) comp;
		obj.mainMenuBarItem(bMainMenuBar);
		obj.changeLook();
	    }
	}
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
        // setOpaque(s);
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        if (menuButton != null) {
            menuButton.setFont(font);
            menuButton.setForeground(fgColor);
        }
        fm = getFontMetrics(font);
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
        case SETVAL:
            return setVal;
        case SEPERATOR:
            return seperatorStr;
        case VARIABLE:
            return vnmrVar;
        case KEYVAL:
             return keyStr;
        case SETCHOICE:
            return null;
        case SETCHVAL:
            return null;
        case HOTKEY:
            return hotkey;
        case ICON:
            return iconName;
        case TOOL_TIP:
            return null;
        case TOOLTIP:
            return tipStr;
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
            if (bButtonMenu)
               return;
            if (c == null || c.length()>0 || c.equals("default")){
                   if (bMainMenuBar)
                       bgColor = Util.getMenuBarBg();
                   else
 		       bgColor = Util.getBgColor();
            }
            else {
               bgColor = DisplayOptions.getColor(c);
            }
            inColor = Util.changeBrightness(bgColor, 10);
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
        case SEPERATOR:
            seperatorStr = c;
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case SETCHOICE:
            break;
        case SETCHVAL:
            break;
        case VALUE:
            break;
        case HOTKEY:
            hotkey = c;
	    char a = hotkey.charAt( 0 );
	    setMnemonic(a);
            break;
        case ICON:
            if (c != null)
               iconName = c.trim();
            else
               iconName = null;
            iconImg = null;
            if (iconName == null || iconName.length() <= 0) {
               iconName = null;
               return;
            }
            iconW = 0;
            iconImg = Util.getGeneralIcon(iconName);
            if (iconImg != null) {
               iconW = iconImg.getIconWidth();
               iconH = iconImg.getIconHeight();
               setIcon(iconImg);
            }
            break;
        case TOOL_TIP:
        case TOOLTIP:
            tipStr = c;
            if (c != null && c.length() > 0)
               setToolTipText(Util.getLabel(c));
            else
               setToolTipText(null);
            break;
        case PANEL_NAME:
            objName = c;
            bVerbose = false;
            if (c != null)
                bVerbose = DebugOutput.isSetFor("traceXML");
            break;
        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
        int m = getMenuComponentCount();
	for (int k = 0; k < m; k++) {
	    Component comp = getMenuComponent(k);
	    if (comp instanceof VObjIF)
               ((VObjIF)comp).setVnmrIF(vif); 
        }
    }

    public void setValue(ParamIF pf) {
    }

    public void setShowValue(ParamIF pf) {
        if (pf == null || pf.value == null)
            return;

        String  s = pf.value.trim();
        boolean bVis = isVisible();
	    isActive = Integer.parseInt(s);
            if (isActive > 0) {
                setBackground(bgColor);
                setEnabled(true);
                if (menuButton != null) {
                    menuButton.setBackground(bgColor);
                    menuButton.setEnabled(true);
                }
            }
            else {
                // setBackground(Global.NPCOLOR);
                setEnabled(false);
                if (menuButton != null) {
                   //  menuButton.setBackground(Global.NPCOLOR);
                    menuButton.setEnabled(false);
                }
            }
            if (isActive >= 0) {
                setVisible(true);
                if (separator != null)
                    separator.setVisible(true);
                if (menuButton != null)
                    menuButton.setVisible(true);
            }
            else {
                setVisible(false);
                if (separator != null)
                    separator.setVisible(false);
                if (menuButton != null)
                    menuButton.setVisible(false);
            }
        if (shownListener != null) {
            if (bVis != isVisible())
               shownListener.childShownEvent(true);
            if (bLastWatchItem)
               shownListener.endShownEvent();
        }
    }

    public void updateShow() {
        if (showVal == null || vnmrIf == null)
            return;
        vnmrIf.asyncQueryShow(this, showVal);
    }

    public void updateMe() {
        if (vnmrIf == null)
            return;
        if (!toUpdate)
            return;
	toUpdate = false;
        if (setVal != null)
            vnmrIf.asyncQueryParam(this, setVal);
    }

    public void initItem() {
	updateShow();
	toUpdate = true;
	updateChild = true;
        int m = getMenuComponentCount();
	for (int k = 0; k < m; k++) {
	    Component comp = getMenuComponent(k);
	    if (comp instanceof VMenuitemIF) {
		VMenuitemIF xobj = (VMenuitemIF) comp;
		xobj.initItem();
	    }
	    else if (comp instanceof ExpListenerIF) {
		ExpListenerIF obj = (ExpListenerIF) comp;
		obj.updateValue();
	    }
	}
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
         
    public void childShownEvent(boolean b) {
        if (b)
            bShownEvent = true; 
    }

    public void endShownEvent() {
         if (!bShownEvent)
             return;
         bShownEvent = false;
         JPopupMenu pop = getPopupMenu();
         if (pop == null || !pop.isVisible())
             return;
         pop.setVisible(false);
         pop.revalidate();
         pop.setVisible(true);
    }

    public void updateChildren() {
	updateChild = false;
        if (bVerbose)
            System.out.println(objName+" updateChildren");
        bShownEvent = false; 
        int m = getMenuComponentCount();
        VMenuitemIF vobj = null;
	for (int k = 0; k < m; k++) {
	    Component comp = getMenuComponent(k);
	    if (comp instanceof ExpListenerIF) {
		ExpListenerIF obj = (ExpListenerIF) comp;
		obj.updateValue();
                if (bMenuBarParent) {
                   if (comp instanceof VMenuitemIF) {
                       if (((VMenuitemIF)comp).setShownListener(this))
                           vobj = (VMenuitemIF) comp;
                   }
                }
	    }
	    else if (comp instanceof VMenuitemIF) {
		VMenuitemIF xobj = (VMenuitemIF) comp;
		xobj.updateMe();
	    }
	}
        if (vobj != null)
            vobj.setLastShownObj(true);
    }

    public void updateValue() {
	toUpdate = true;
	updateShow();
	updateMe();
        if (bVerbose)
            System.out.println(objName+" update all");
        if (bMenuBarParent) {
            updateChild = true;
            return;
        }
        int m = getMenuComponentCount();
	for (int k = 0; k < m; k++) {
	    Component comp = getMenuComponent(k);
	    if (comp instanceof ExpListenerIF) {
		ExpListenerIF obj = (ExpListenerIF) comp;
		obj.updateValue();
	    }
	}
    }


    public void updateValue (Vector params) {
        StringTokenizer tok;
        String          v;
        int             pnum = params.size();
        int             k;

        int m = getMenuComponentCount();

        if (bVerbose)
           System.out.println(objName+" param update");
        if (bMenuBarParent)
           updateChild = true;
        else if (!updateChild) {
	   for (k = 0; k < m; k++) {
	       Component comp = getMenuComponent(k);
               if (bVerbose)
                   System.out.println(objName+" update child "+k);
	       if (comp instanceof ExpListenerIF) {
	           ExpListenerIF obj = (ExpListenerIF) comp;
		   obj.updateValue(params);
	       }
               if (updateChild)
                  break;
	   }
	}
        if (vnmrVar == null)
            return;
        tok = new StringTokenizer(vnmrVar, " ,\n");
        while (tok.hasMoreTokens()) {
            v = tok.nextToken();
            for (k = 0; k < pnum; k++) {
                if (v.equals(params.elementAt(k))) {
		    updateShow();
		    toUpdate = true;
		    if (bSubMenuParent) {
			updateLater();
                    }
		    else {
			updateMe();
		    }
                    return;
                }
            }
        }
    }

    public void refresh() {
    }

    public void destroy() {
    }

    public void setDefLoc(int x, int y) {}

    public void setModalMode(boolean s) {
    }

    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
        double newTime = System.currentTimeMillis();
        if (newTime - lastCmdTime < 1000) // less than 1 seconds
            return;
        lastCmdTime = newTime;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
        return;
    }

    public void mainMenuBarItem(boolean b) {
        bMainMenuBar = b;
        if (bButtonMenu)
            bgColor = Util.getToolBarBg();
        else {
            if (bMainMenuBar) {
                if (vparent == null) {
                    Container c = getParent();
                    if (c != null)
                        setVParent(c);
                }
                bgColor = Util.getMenuBarBg();
            }
            else
                bgColor = Util.getBgColor();
        }
	setBackground(bgColor);
    }

    public void setButtonMenu(boolean b) {
        if ( b && menuButton == null) {
            // menuButton = new XMenuButton(label, iconImg);
            // menuButton.setBorder(null);
	    // menuButton.setIcon(iconImg);
            menuButton = new VButton(sshare, vnmrIf, "button");
	    menuButton.setBackground(bgColor);
            if (iconName != null)
               menuButton.setAttribute(ICON, iconName);
	    menuButton.setFont(font);
            menuButton.setForeground(fgColor);
            menuButton.setToolBarButton(true);
            if (vnmrCmd != null) {
               menuButton.setAttribute(CMD, vnmrCmd);
            }
            menuButton.setBorderPainted(false);
            Border br = menuButton.getBorder();
            if (br != null && (br instanceof VButtonBorder)) {
               ((VButtonBorder) br).setHalfWay(true);
            }
/*
            menuButton.addActionListener(new ActionListener() { 
               public void actionPerformed(ActionEvent e) {
                      sendVnmrCmd();
               }
            });
*/
            MouseAdapter ma = new MouseAdapter() {
              public void mousePressed(MouseEvent evt) {
                  if (isActive > 0)
                     bPressed = true;
              }

              public void mouseReleased(MouseEvent evt) {
                  bPressed = false;
                  repaint();
              }

              public void mouseEntered(MouseEvent evt) {
                  if (isActive > 0) {
                     bMouseIn = 1;
                     bEntered = true;
                     bBorder = true;
                     setBackground(inColor);
                     menuButton.setBackground(inColor);
                     menuButton.setBorderPainted(true);
                     repaint();
                  }
              }

              public void mouseExited(MouseEvent evt) {
                  bEntered = false;
                  if (!bPopup) {
                      bMouseIn = 0;
                      bBorder = false;
                      if (isActive > 0) {
                         setBackground(bgColor);
                         menuButton.setBackground(bgColor);
                         menuButton.setBorderPainted(false);
                         repaint();
                      }
                  }
              }
            };
            addMouseListener(ma);
            menuButton.addMouseListener(ma);
            if (tipStr != null) {
               menuButton.setToolTipText(Util.getLabel(tipStr));
               // Kludge Alert:  While calling setToolTipText() works for
               // displaying the tooltip, it does not work for actually
               // setting menuButton.tipStr.  This variable is used
               // for CSH so I am setting it manually.
               menuButton.tipStr = Util.getLabel(tipStr);
            }
            if (markXpnts == null) {
               markXpnts = new int[3];
               markYpnts = new int[3];
            }
            menuIcon = (Icon) (new MetalComboBoxIcon());
            disabledFg = UIManager.getColor("MenuItem.disabledForeground");
        }
 
        bButtonMenu = b;
        if (bButtonMenu) {
            bgColor = Util.getToolBarBg();
            inColor = Util.changeBrightness(bgColor, 10);
	    setBackground(bgColor);
        }
    }

    public Component getMenuButton() {
        return (Component) menuButton;
    }

    public Dimension getPreferredSize() {
        if (!bButtonMenu || menuIcon == null)
            return super.getPreferredSize();
        int w = menuIcon.getIconWidth();
        int h = menuIcon.getIconHeight();
        return new Dimension(w+6, h + 4);
        // return new Dimension(12, 12);
    }

    public void paint(Graphics g) {
        if (!bButtonMenu || menuIcon == null) {
            super.paint(g);
            return;
        }
        if (!isEnabled())
            return;
        Dimension d = getSize();
        int x = 2;
        int y = 2;
        int w = menuIcon.getIconWidth();
        int h = menuIcon.getIconHeight();

        if (bEntered) {
            g.setColor(inColor);
            g.fillRect(0, 0, d.width, d.height);
        }
        else if (!bHiFiUI) {
            g.setColor(bgColor);
            g.fillRect(0, 0, d.width, d.height);
        }
        x = (d.width - w) / 2;
        y = (d.height - h) / 2;
        menuIcon.paintIcon(this, g, x, y);
        
/**
        if (bPressed)
            g.setColor(bgColor.darker().darker());
        else
            g.setColor(bgColor.brighter());
        x = d.width - 1;
        y = d.height - 1;
        g.drawLine(0, 0, x, 0);
        if (bPressed)
            g.drawLine(0, 0, 0, y);
        if (bPressed)
            g.setColor(bgColor.brighter());
        else
            g.setColor(bgColor.darker().darker());
        g.drawLine(x, 0, x, y);
        g.drawLine(0, y,  x, y);
**/
/*
        if (isActive > 0)
           g.setColor(fgColor);
        else {
           if ( disabledFg != null)
               g.setColor(disabledFg);
           else 
               g.setColor(getBackground().brighter());
        }
        x = d.width - 9;
        markXpnts[0] = x;
        markXpnts[1] = x + 6;
        markXpnts[2] = x + 3;
        y = (d.height - 3) / 2;
        markYpnts[0] = y;
        markYpnts[1] = y;
        markYpnts[2] = y + 3;
        g.fillPolygon(markXpnts, markYpnts, 3);
        if (bBorder) {
            // g.setColor(bgColor.darker());
            g.setColor(Util.getActiveBg());
            g.drawRect(0, 0, d.width-1, d.height-1);
        }
*/
    }


    public Point getDefLoc() { return getLocation(); }

    public void setSizeRatio(double x, double y) {}

    class VSubMenuUI extends BasicMenuUI
    {

        protected ChangeListener createChangeListener(JComponent c) {
            return new MenuChangeHandler((JMenu)c, this);
        }

        public class MenuChangeHandler extends ChangeHandler {
            public MenuChangeHandler(JMenu m, VSubMenuUI ui) {
                super(m, ui);
            }

            public void stateChanged(ChangeEvent e) {
                JMenuItem c = (JMenuItem)e.getSource();
                if (c.isArmed() || c.isSelected()) {
                    c.setBorderPainted(true);
                    // c.repaint();
                } else {
                    c.setBorderPainted(false);
                }

                super.stateChanged(e);
            }
       }
    }

    private class XMenuButton extends JButton {
       public XMenuButton(String text, Icon icon) {
           super(text, icon);
       }

       public void paint(Graphics g) {
            super.paint(g);
            if (!bBorder || bMouseIn < 1)
                return;
            Dimension d = getSize();
            g.setColor(Util.getActiveBg());
            g.drawLine(0, 0, 0, d.height-1);
            g.drawLine(0, 0, d.width, 0);
            g.drawLine(0, d.height-1, d.width, d.height-1);
            // g.drawRect(0, 0, d.width-1, d.height-1);
       }
    }

}

