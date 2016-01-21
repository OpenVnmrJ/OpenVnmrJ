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
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import javax.accessibility.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VPopup extends JLabel implements VEditIF,
                                              VGroupSave,
                                              VObjDef,
                                              VObjIF,
                                              DropTargetListener,
                                              ExpListenerIF,
                                              PropertyChangeListener {

    public Color  bgColor, orgBg;
    public Color  fgColor = null;
    public Font   font = null;
    public String bg = null;
    public String fg = null;
    public String fontName = null;
    public String fontSize = null;
    public String fontStyle = null;
    public String label = null;
    public String setVal = null;
    public String showVal = null;
    public String toolTip = null;
    public String type = null;
    public String vnmrCmd = null;
    public String vnmrCmd2 = null;
    public String vnmrVar = null;


    protected boolean inAddMode = false;
    protected boolean inChangeMode = false;
    protected boolean inEditMode = false;
    protected boolean inModalMode = false;
    protected boolean isEditing = false;
    protected boolean isFocused = false;
    protected int isActive = 1;
    protected ButtonIF             vnmrIf;
    protected JPopupMenu           jpm;
    protected MouseAdapter         ml;
    protected SessionShare         sshare;
    protected SingleSelectionModel jpmModel;
    protected String               keyStr = null;

    private int             itemCount = 0;
    private VMenuLabel	    mlabel;
    private JmiActionEar    jmiActionEar;

    public VPopup(SessionShare ss, ButtonIF vif, String typ) {
	super("Default Label");
        this.sshare = ss;
        this.type = typ;
        this.vnmrIf = vif;
        orgBg = getBackground();
        bgColor = Util.getBgColor();
        jpm = new JPopupMenu();
        jpmModel = jpm.getSelectionModel();
        jmiActionEar  = new JmiActionEar();

        ml = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                    }
                }
             }
        };


        addMouseListener(new PopupListener());

        addKeyListener(new KeyAdapter()
        {
            public void keyPressed(KeyEvent e)
            {
                if (e.getKeyCode() == KeyEvent.VK_ENTER)
                {
                    doAction((JMenuItem)e.getSource());
                }
            }
        });
        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
    }

    class PopupListener extends MouseAdapter {
        public void mouseReleased(MouseEvent e) {
            maybeShowPopup(e);
        }

        public void mousePressed(MouseEvent e) {
            maybeShowPopup(e);
        }

        private void maybeShowPopup(MouseEvent e) {
            if (e.isPopupTrigger()) {
                jpm.show(e.getComponent(),
                         e.getX(), e.getY());
            }
        }
    }

    public class JmiActionEar implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            doAction((JMenuItem)ae.getSource());
        }
    }
    /**
     *  Handles Key Events.
     */
/*    public void processKeyEvent(KeyEvent e)
/*    {
/*        super.processKeyEvent(e);
/*
/*        // Make sure that the selected index is visible.
/*        if (isPopupVisible())
/*        {
/*            int nIndex = getSelectedIndex();
/*            Accessible a = this.getUI().getAccessibleChild(this, 0);
/*            if (a != null &&
/*                a instanceof javax.swing.plaf.basic.ComboPopup)
/*            {
/*
/*                // get the popup list
/*                JList list = ((javax.swing.plaf.basic.ComboPopup)a).getList();
/*                if (list != null)
 /*                   list.ensureIndexIsVisible(nIndex);
 /*           }
 /*       }
/*    }
*/
    /** The array of the attributes that are displayed in the edit template.*/
    private final static Object[][] attributes = {
	{new Integer(LABEL), 	"Label of item:"},
	{new Integer(ICON), 	"Icon of item:"},
	{new Integer(SHOW),	"Enable condition:"},
	{new Integer(SETCHOICE),"Labels of choices:"},
	{new Integer(SETCHVAL),	"Values of choices:"},
        {new Integer(TOOL_TIP), "Tool tip:"},
    };

    public Object[][] getAttributes()  { return attributes; }
    public int getComponentCount() {
	return itemCount;
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
        fgColor=DisplayOptions.getColor(fg);
        setForeground(fgColor);
        changeFont();
        bgColor = Util.getBgColor();
        setBackground(bgColor);
    }

    public void setDefLabel(String s) {
        this.label = s;
    }

    public void setDefColor(String c) {
        this.fg = c;
        fgColor = VnmrRgb.getColorByName(c);
        setForeground(fgColor);
    }


    public void checkAndShow() {
        if (itemCount <= 1) {
            setVisible(false);
        }
        else {
            setVisible(true);
        }
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        inEditMode = s;
        for (int i = 0; i < itemCount; i++) {
            Component jcomp = getComponent(i);
            if (jcomp instanceof VObjIF) {
                VObjIF vobj = (VObjIF)jcomp;
                vobj.setEditMode(s);
            }
        }
        if (s) {
          setVisible(true);
          addMouseListener(ml);
        }
        else {
          checkAndShow();
          removeMouseListener(ml);
        }
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        repaint();
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    public String getAttribute(int attr) {
        int             k;
        String s;
        switch (attr) {
        case BGCOLOR:
            return bg;
        case CMD:
            return vnmrCmd;
        case FGCOLOR:
            return fg;
        case FONT_NAME:
            return fontName;
        case FONT_SIZE:
            return fontSize;
        case FONT_STYLE:
            return fontStyle;
        case KEYSTR:
            return keyStr;
        case KEYVAL:
                if(keyStr==null)
                        return null;
        case LABEL:
            return label;
        case SETCHOICE:
            s = "";
            for (k = 0; k < itemCount; k++) {
                mlabel=(VMenuLabel)getComponent(k);
                s+= "\"" + mlabel.getAttribute(LABEL) + "\" ";
            }
            return s;
        case SETCHVAL:
            s = "";
            for (k = 0; k < itemCount; k++) {
                mlabel=(VMenuLabel)getComponent(k);
                String chval=mlabel.getAttribute(CHVAL);
                if(chval !=null)
                    s+= "\"" + chval + "\" ";
            }
            return s;
        case SETVAL:
            return setVal;
        case SHOW:
            return showVal;
        case TOOL_TIP:
            return toolTip;
        case TYPE:
            return type;
        case VALUE:
                return null;
        case VARIABLE:
            return vnmrVar;
         default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        Vector v;
        switch (attr) {
        case BGCOLOR:
            bg = c;
            if (c == null || c.length()>0 || c.equals("default")){
                bgColor = Util.getBgColor();
            }
            else {
               bgColor = DisplayOptions.getColor(c);
            }
            setBackground(bgColor);
            repaint();
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            repaint();
            break;
        case FONT_NAME:
            fontName = c;
            break;
        case FONT_SIZE:
            fontSize = c;
            break;
        case FONT_STYLE:
            fontStyle = c;
            break;
        case KEYSTR:
                keyStr=c;
            break;
        case KEYVAL:
            label=c;
            break;
        case LABEL:
            label = c;
            setText(label);
            break;
        case SETCHOICE:
            inAddMode = true;
            v = ParamInfo.parseChoiceStr(c);
            int    m = 0;
            if (v != null)
                m = v.size();
            removeAll();
            if (itemCount > 0){
                for(int i=0; i<itemCount; i++) {
                    jpm.remove(0);
                }
            }
            itemCount=0;
            if (v == null || m<1) {
                inAddMode = false;
                return;
            }
            for (int n = 0; n < m; n++) {
                String val = (String)v.elementAt(n);
                addChoice(val);
            }
            v = null;
            inAddMode = false;
            break;
        case SETCHVAL:
            v = ParamInfo.parseChoiceStr(c);
            if (v == null)
                return;

            for (int k = 0; k < v.size(); k++) {
                if(k<itemCount){
                    mlabel=(VMenuLabel)getComponent(k);
                    mlabel.setAttribute(CHVAL,(String) v.elementAt(k));
                }
            }
            break;
        case SETVAL:
            setVal = c;
            break;
        case SHOW:
            showVal = c;
            break;
        case TOOL_TIP:
        case TOOLTIP:
            toolTip = c;
            if (c != null && c.length() > 0)
               setToolTipText(Util.getLabel(c));
            else
               setToolTipText(null);
            break;
        case TYPE:
            type = c;
            break;
        case VALUE:
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        }
    }

    private VMenuLabel addChoice(String val) {
        JMenuItem tmpJMI = new JMenuItem(val);
        tmpJMI.addActionListener(jmiActionEar);
        jpm.add(tmpJMI);
        itemCount++;
        mlabel = new VMenuLabel(sshare, vnmrIf, "mlabel");
        mlabel.setAttribute(LABEL,val);
        super.add(mlabel);
        return (mlabel);
    }

    public Component add(Component jcomp) {
        inAddMode = true;
        if (jcomp instanceof VObjIF) {
            VObjIF obj=(VObjIF)jcomp;
            String item=obj.getAttribute(LABEL);
            if (item !=null){
                mlabel=addChoice(item);
                mlabel.setAttribute(CHVAL,obj.getAttribute(CHVAL));
            }
        }
        inAddMode = false;
        return mlabel;
   }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void setValue(ParamIF pif) {
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            isActive = Integer.parseInt(s);
            if (isActive > 0) {
                setBackground(bgColor);
            }
            else {
                if (isActive == 0)
                    setBackground(Global.IDLECOLOR);
                else
                    setBackground(Global.NPCOLOR);
            }
            if (isActive >= 0) {
                setEnabled(true);
                if (setVal != null) {
                    vnmrIf.asyncQueryParam(this, setVal);
                }
            }
            else {
                setEnabled(false);
            }
        }
    }

    public void updateValue() {
        if (vnmrIf == null)
            return;
        if (!inEditMode)
            checkAndShow();
        if (showVal != null)
            vnmrIf.asyncQueryShow(this, showVal);
        else if (setVal != null)
            vnmrIf.asyncQueryParam(this, setVal);
    }

    public void updateValue (Vector params) {
        StringTokenizer tok;
        String          vars, v;
        int             pnum = params.size();

        vars = getAttribute(VARIABLE);
        if (vars == null)
            return;
        tok = new StringTokenizer(vars, " ,\n");
        while (tok.hasMoreTokens()) {
            v = tok.nextToken();
            for (int k = 0; k < pnum; k++) {
                if (v.equals(params.elementAt(k))) {
                    updateValue();
                    return;
                }
            }
        }
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

    protected void doAction(JMenuItem jmi) {
        if(inModalMode || vnmrIf == null)
            return;
        if (inAddMode || inChangeMode ||  inEditMode )
            return;
        if (isActive < 0)
            return;

        String strChoice = null;
	String txt = jmi.getText();
	for (int i=0; i<itemCount; i++) {
            mlabel = (VMenuLabel)getComponent(i);
            if ( txt.equals( mlabel.getAttribute(LABEL) ) ) {
                strChoice = mlabel.getAttribute(CHVAL);
                break;
            }
        }
        if (strChoice == null) 
            System.out.println("Error, no such selection");
        vnmrIf.sendVnmrCmd(this, strChoice);
    }

    public void setDefLoc(int x, int y) {}

/*    public void writeValue(PrintWriter fd, int gap) {
/*        int             k;
/*        if (itemCount <= 0)
/*            return;
/*        for (int i = 0; i < itemCount; i++) {
/*            mlabel=(VMenuLabel)getComponent(i);
/*            fd.print("choice=\"" + mlabel.getText() + "\"");
/*            fd.println(" chval=\"" + mlabel.getName() + "\"");
/*            for (k = 0; k < gap + 1; k++)
/*                fd.print("  ");
/*        }
/*    }
/* */

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


