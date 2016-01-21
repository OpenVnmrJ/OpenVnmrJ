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
import java.awt.dnd.*;
import java.awt.event.*;
import java.beans.*;
import java.util.*;
import javax.swing.*;

import vnmr.util.*;
import vnmr.ui.*;

/**
 * A base class for objects implementing VObjIF that provides
 * some of the basic functionality.
 * <p>
 */
public class VObj extends JPanel
    implements VObjIF, VEditIF, VObjDef, DropTargetListener,
     PropertyChangeListener {

    private String type = null;
    private String fg = null;
    private String bg = "VJBackground";
    private String fontName = null;
    private String fontStyle = null;
    private String fontSize = null;
    protected Color  fgColor = null;
    protected Color  bgColor, orgBg;
    protected Font   font = null;

    protected String label = null;
    private MouseAdapter editML;
    private java.util.List<ComponentListener> componentList;

    protected boolean inEditMode = false;

    protected String vnmrVar = null;
    protected String valueExpr = null;
    protected String showExpr = null;
    protected String vnmrCmd = null;
    protected String vnmrCmd2 = null;
    protected String menuLabels = null;
    protected String menuValues = null;
    protected String statusParam = null;
    protected VMenuLabel mlabel;

    protected String precision = null;
    protected String value = null;
    protected int isActive = 1;
    protected boolean isEnabled = true;

    protected boolean isEditing = false;
    protected boolean isFocused = false;
    protected ButtonIF vnmrIf;
    protected VObjIF myself;

    protected boolean inModalMode = false; // Needed?

    protected double xRatio = 1.0;
    protected double yRatio = 1.0;
    protected Dimension defDim = new Dimension(0,0);
    protected Dimension curDim = new Dimension(0,0);
    protected Point defLoc = new Point(0, 0);
    protected Point curLoc = new Point(0, 0);
    protected Point tmpLoc = new Point(0, 0);


    /**
     * Dummy constructor for test programs.
     */
    public VObj() {
    }

    /**
     * Normal constructor.
     */
    public VObj(SessionShare sshare, ButtonIF vif, String typ) {
        myself = this;
        this.type = typ;
        this.vnmrIf = vif;
        this.fg = "PlainText";
        this.fontSize = "8";
        this.fontStyle = "Bold";
        setOpaque(false);
        bgColor = orgBg = DisplayOptions.getColor(bg);
        setOpaque(true);
        setBackground(bgColor);
        componentList = new LinkedList<ComponentListener>();

        editML = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0 && clicks == 2) {
                    ParamEditUtil.setEditObj(myself);
                }
            }
        };

        DisplayOptions.addChangeListener(this);
        new DropTarget(this, this);
    }

    // PropertyChangeListener interface
    public void propertyChange(PropertyChangeEvent evt) {
        if (fg != null) {
            fgColor = DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        if (bg != null) {
            bgColor = DisplayOptions.getColor(bg);
            setBackground(bgColor);
        }
        changeFont();
    }

    public void setDefLabel(String s) {
        this.label = s;
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    /**
     * Indicate whether this particular component is currently
     * being edited.
     * @param b Is this object being edited?
     */
    public void setEditStatus(boolean b) {
        isEditing = b;
        repaint();
    }

    /**
     * Add mouse listeners to this component and all components it may
     * contain.  It is harmless to pass a null listener.
     * @param ml The MouseListener to add.
     * @param mml The MouseMotionListener to add.
     */
    public void addMouseListeners(MouseListener ml, MouseMotionListener mml) {
        addMouseListeners(this, ml, mml);
    }

    /**
     * Add mouse listeners to a specified component and all components
     * it may contain.  It is harmless to pass a null listener.
     * @param com The component.
     * @param ml The MouseListener to add.
     * @param mml The MouseMotionListener to add.
     */
    public void addMouseListeners(Component com,
                                  MouseListener ml, MouseMotionListener mml) {
        com.addMouseListener(ml);
        com.addMouseMotionListener(mml);
        synchronized(getTreeLock()) {
            if (com instanceof Container) {
                Container con = (Container)com;
                int count = con.getComponentCount();
                Component[] component = con.getComponents();
                for (int i=0; i<count; ++i) {
                    addMouseListeners(component[i], ml, mml);
                }
            }
        }
    }

    /**
     * Remove mouse listeners from this component and all components
     * it may contain.  It is harmless to pass a null listener.
     * @param ml The MouseListener to remove.
     * @param mml The MouseMotionListener to remove.
     */
    public void removeMouseListeners(MouseListener ml,
                                     MouseMotionListener mml) {
        removeMouseListeners(this, ml, mml);
    }

    /**
     * Remove mouse listeners from a specified component and all
     * components it may contain.  It is harmless to pass a null
     * listener.
     * @param com The component.
     * @param ml The MouseListener to remove.
     * @param mml The MouseMotionListener to remove.
     */
    public void removeMouseListeners(Component com,
                                     MouseListener ml,
                                     MouseMotionListener mml) {
        com.removeMouseListener(ml);
        com.removeMouseMotionListener(mml);
        synchronized(getTreeLock()) {
            if (com instanceof Container) {
                Container con = (Container)com;
                int count = con.getComponentCount();
                Component[] component = con.getComponents();
                for (int i=0; i<count; ++i) {
                    removeMouseListeners(component[i], ml, mml);
                }
            }
        }
    }

   private void fillComponentList(Container parent) {
        int count = parent.getComponentCount();
        for (int i=-1; i<count; ++i) {
            Component com;
            Container con;
            if (i < 0) {
                com = parent;
            } else {
                com = parent.getComponent(i);
                if (com instanceof Container &&
                    (con=(Container)com).getComponentCount() > 0)
                {
                    fillComponentList(con); // Recursively do containers
                }
            }
            if (com instanceof JComponent) {
                JComponent jc = (JComponent)com;
                MouseListener[] mlList = (MouseListener[])
                    (jc.getListeners(MouseListener.class));
                MouseMotionListener[] mmlList  = (MouseMotionListener[])
                    (jc.getListeners(MouseMotionListener.class));
                componentList.add(new ComponentListener(jc, mlList, mmlList));
            }
        }
    }

    /**
     * Set object state according to whether the interface is in
     * normal mode or editing mode.
     * When we enter editing mode, this method removes all the
     * normal MouseListeners and MouseMotionListeners from
     * the VObj object and all of its children.  They are
     * replaced by a special MouseListener for editing.
     * On leaving edit mode, the special MouseListener is
     * removed, and the normal listeners reinstalled.
     * @param b Are we editing?
     */
    public void setEditMode(boolean b) {
        if (b == inEditMode) {
            // Prevent overdoing it.
            return;
        }
        if (b) {
            // Remove all current mouse listeners and
            // install the editing mouse listeners in
            // this and all components we contain.
            componentList.clear();
            fillComponentList(this);
            int count = componentList.size();
            for (int i=0; i<count; ++i) {
                ComponentListener cl = componentList.get(i);
                JComponent jc = (JComponent)cl.component;

                // Remove the normal mouse listeners
                MouseListener[] mlList = cl.mlList;
                MouseMotionListener[] mmlList = cl.mmlList;
                for (int j=0; mlList != null && j<mlList.length; ++j) {
                    jc.removeMouseListener(mlList[j]);
                }
                for (int j=0; mmlList != null && j<mmlList.length; ++j) {
                    jc.removeMouseMotionListener(mmlList[j]);
                }
                // Install the editing mouse listeners
                jc.addMouseListener(editML);
            }

            setOpaque(b);
            defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
            xRatio = 1.0;
            yRatio = 1.0;
        } else {
            int count = componentList.size();
            for (int i=0; i<count; ++i) {
                ComponentListener cl = componentList.get(i);
                JComponent jc = (JComponent)cl.component;

                // Remove the editing mouse listeners
                jc.removeMouseListener(editML);

                // Restore the normal mouse listeners
                MouseListener[] mlList = cl.mlList;
                MouseMotionListener[] mmlList = cl.mmlList;
                for (int j=0; mlList != null && j<mlList.length; ++j) {
                    jc.addMouseListener(mlList[j]);
                }
                for (int j=0; mmlList != null && j<mmlList.length; ++j) {
                    jc.addMouseMotionListener(mmlList[j]);
                }
            }

            if ((bg != null) || (isActive < 1)) {
                setOpaque(true);
            } else {
                setOpaque(false);
            }
        }
        inEditMode = b;
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        repaint();
    }

    public void changeFocus(boolean b) {
        isFocused = b;
        repaint();
    }

    public String getAttribute(int attr) {
        switch (attr) {
          case TYPE:
            return type;
          case FGCOLOR:
            return fg;
          case BGCOLOR:
            return bg;
          case FONT_NAME:
            return fontName;
          case FONT_STYLE:
            return fontStyle;
          case FONT_SIZE:
            return fontSize;
          case VALUE:
            return value;
          case LABEL:
            return label;
          case VARIABLE:
            return vnmrVar;
          case SETVAL:
            return valueExpr;
          case SHOW:
            return showExpr;
          case CMD:
            return vnmrCmd;
          case CMD2:
            return vnmrCmd2;
          case SETCHOICE:
            return menuLabels;
          case SETCHVAL:
            return menuValues;
          case STATPAR:
            return statusParam;
          default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        if (c != null) {
            c = c.trim();
            if (c.length() == 0) {c = null;}
        }
        switch (attr) {
          case TYPE:
            type = c;
            break;
          case FGCOLOR:
            fg = c;
            fgColor = DisplayOptions.getColor(c);
            setForeground(fgColor);
            repaint();
            break;
          case BGCOLOR:
            bg = c;
            if (c != null) {
                bgColor = DisplayOptions.getColor(c);
                setOpaque(true);
            } else {
                bgColor = orgBg;
                if (isActive < 1) {
                    setOpaque(true);
                } else {
                    setOpaque(inEditMode);
                }
            }
            setBackground(bgColor);
            repaint();
            break;
          case FONT_NAME:
            fontName = c;
            changeFont();
            break;
          case FONT_STYLE:
            fontStyle = c;
            changeFont();
            break;
          case FONT_SIZE:
            fontSize = c;
            changeFont();
            break;
          case VALUE:
            value = c;
            break;
          case LABEL:
            label = c;
            break;
          case VARIABLE:
            vnmrVar = c;
            break;
          case SETVAL:
            valueExpr = c;
            break;
          case SHOW:
            showExpr = c;
            break;
          case CMD:
            vnmrCmd = c;
            break;
          case CMD2:
            vnmrCmd2 = c;
            break;
          case SETCHOICE:
            menuLabels = c;
            break;
          case SETCHVAL:
            menuValues = c;
            break;
          case STATPAR:
            statusParam = c;
            updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        }
        validate();
        repaint();
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
    };

    public void updateValue() {
        if (vnmrIf == null) {
            return;
        }
        if (showExpr != null) {
            vnmrIf.asyncQueryShow(this, showExpr);
        }
        if (valueExpr != null) {
            vnmrIf.asyncQueryParam(this, valueExpr);
        }
    }

    /**
     * Set the internal member "value" variable.
     * The derived class may override this to set actual component value.
     */
    public void updateStatus(String msg) {
        if (msg == null || msg.length() == 0 || msg.equals("null") ||
            statusParam == null) {
            return;
        }
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals(statusParam)) {
                value = tok.nextToken();
            }
        }
    }

    /**
     * Set the internal member "value" variable.
     * The derived class may override this to set actual component value.
     */
    public void setValue(ParamIF pf) {
        value = pf.value;
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            isActive = Integer.parseInt(s);
            if (isActive > 0) {
                setOpaque(inEditMode);
                if (bg != null) {
                    setOpaque(true);
                    setBackground(bgColor);
                }
            }
            else {
                setOpaque(true);
                if (isActive == 0)
                    setBackground(Global.IDLECOLOR);
                else
                    setBackground(Global.NPCOLOR);
            }
            if (isActive >= 0) {
                setEnabled(true);
                isEnabled=true;
                if (valueExpr != null) {
                    vnmrIf.asyncQueryParam(this, valueExpr, precision);
                }
            }
            else {
                setEnabled(false);
                isEnabled = false;
            }
        }
    }

    public void refresh() {}
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

    public Object[][] getAttributes() {
        return attributes;
    }

    private final static Object[][] attributes = {
        {new Integer(LABEL),    "Label of item:"},
        {new Integer(VARIABLE), "Vnmr variables:"},
        {new Integer(SETVAL),   "Value of item:"},
        {new Integer(SHOW),     "Enable condition:"},
        {new Integer(CMD),      "Vnmr command:"},
        {new Integer(CMD2),     "Vnmr command 2:"},
        {new Integer(SETCHOICE),"Labels of choices:"},
        {new Integer(SETCHVAL), "Values of choices:"},
        {new Integer(NUMDIGIT), "Decimal places:"},
        {new Integer(STATPAR),  "Status variable:"},
    };

    public void setModalMode(boolean b) {
        inModalMode = b;
    }

    public void sendVnmrCmd() {
        if (vnmrIf == null)
            return;
        if (vnmrCmd != null) {
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
    }

    /**
     * Makes a list of all the VObjIF objects contained in this component.
     * @param comp The component to look in.
     * @param list The list to put th VObjIFs into.
     */
    public static void getVObjs(Component comp, java.util.List<VObjIF> list) {
        synchronized(comp.getTreeLock()) {
            if (comp instanceof VObjIF) {
                list.add((VObjIF)comp);
            }
            if (comp instanceof Container) {
                Container con = (Container)comp;
                int count = con.getComponentCount();
                for (int i=0; i<count; i++) {
                    getVObjs(con.getComponent(i), list);
                }
            }
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

    public void setSizeRatio(double x, double y) {
        double xRatio =  x;
        double yRatio =  y;
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


    class ComponentListener {
        private JComponent component; /** The component */
        /** List of MouseListeners removed when we go into edit mode. */
        private MouseListener[] mlList;
        /** List of MouseMotionListeners removed when we go into edit mode. */
        private MouseMotionListener[] mmlList;

        public ComponentListener(JComponent comp,
                                 MouseListener[] mlList,
                                 MouseMotionListener[] mmlList) {
            this.component = comp;
            this.mlList = mlList;
            this.mmlList = mmlList;
        }
    }

}
