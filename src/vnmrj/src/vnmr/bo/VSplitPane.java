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
import java.io.*;
import javax.swing.*;
import javax.swing.event.MouseInputAdapter;
import javax.swing.plaf.basic.BasicSplitPaneDivider;

import java.awt.geom.*;
import java.awt.event.*;
import java.beans.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.templates.*;


public class VSplitPane extends VGroup implements LayoutManager {

    private static final int VERTICAL = JSplitPane.VERTICAL_SPLIT;
    private static final int HORIZONTAL = JSplitPane.HORIZONTAL_SPLIT;
    private static final int DIVIDER_WIDTH = 8;

    protected JSplitPane m_splitPane = null;
    protected String m_setval = null;
    protected String m_value = "";
    protected String m_orient = "";
    protected float m_splitLocation = (float)0.5;
    protected int m_dividerLocation = -1;
    protected VObjCommandFilter m_filter = new VObjCommandFilter(1000, true);

    private boolean m_loaded = false;
    private boolean m_firstLayout = true;
    private boolean m_doingSetValue = false;

    /**
     * Constructor
     * @param sshare    The session share object.
     * @param vif       The button callback interface.
     * @param typ The type of the object.
     */
    public VSplitPane(SessionShare sshare, ButtonIF vif, String typ)
    {
        super(sshare, vif, typ);
        m_splitPane = new JSplitPane();
        m_splitPane.setUI(new javax.swing.plaf.metal.MetalSplitPaneUI());
        initSplitPane();
        m_splitPane.setVisible(false);
        m_splitPane.setBackground(null);
        add(m_splitPane);
        setLayout(this);
        m_filter.setThrottle(100);
        m_splitPane.addPropertyChangeListener(this);
    }

    private void initSplitPane() {
        m_splitPane.setContinuousLayout(true);
        m_splitPane.setOneTouchExpandable(true);
        m_splitPane.setDividerSize(DIVIDER_WIDTH);
        m_splitPane.setResizeWeight(0.5);
    }

    public void restoreDividerLocation() {
        m_splitPane.setDividerLocation(m_splitPane.getLastDividerLocation());
    }

    public void setDividerVisible(boolean b) {
        if (m_splitPane.getDividerSize() > 0) {
            m_dividerLocation = m_splitPane.getDividerLocation();
        }
        int width = b ? DIVIDER_WIDTH : 0;
        m_splitPane.setDividerSize(width);
        if (b) {
            if (m_dividerLocation > 0 ) {
                m_splitPane.setDividerLocation(m_dividerLocation);
            } else {
                m_splitPane.setDividerLocation(m_splitLocation);
            }
        }
    }

    public void initDivider() {
        Messages.postDebug("SplitPane", "initDivider: " + m_setval);
        double[] split = getProportionalDividerLocation();
        if (m_splitLocation <= split[1] || m_splitLocation >= split[2]) {
            m_splitLocation = Math.min(1, Math.max(0, m_splitLocation));
            m_doingSetValue = true;
            Messages.postDebug("SplitPane", "initDivider: setting divider");
            m_splitPane.setDividerLocation(m_splitLocation);
            Messages.postDebug("SplitPane", "initDivider: divider set\n");
            m_doingSetValue = false;
        }
    }

    public Component getLeftComponent() {
        return m_splitPane.getLeftComponent();
    }

    public Component getRightComponent() {
        return m_splitPane.getRightComponent();
    }

    public void  updateValue(Vector params) {
        Messages.postDebug("SplitPane",
                           title + ".updateValue(params): "
                           + ", nComps=" + getComponentCount());
        //Messages.writeStackTrace(new Exception("DEBUG STACK TRACE"));
        String vars = getAttribute(VARIABLE);
        int pnum = params.size();
        if (vars != null) {
            StringTokenizer tok = new StringTokenizer(vars, " ,\n");
            boolean got = false;
            while (!got && tok.hasMoreTokens()) {
                String v = tok.nextToken();
                for (int k = 0; k < pnum; k++) {
                    if (v.equals(params.elementAt(k))) {
                        got = true;
                        updateValue();
                        break;
                    }
                }
            }
        }
        super.updateValue(params);
    }

    /**
     * Updates the value.
     */
    public void updateValue() {
        Messages.postDebug("SplitPane",
                           title + ".updateValue(): "
                           + ", nComps=" + getComponentCount());
        //Messages.writeStackTrace(new Exception("DEBUG STACK TRACE"));
        if (vnmrIf == null) {
            return;
        }
        if (m_setval != null) {
            vnmrIf.asyncQueryParam(this, m_setval);
        }
    }

    // Gets "setValue" messages
    public void setValue(ParamIF pf) {
        if (pf != null) {
            String value = pf.value;
            if (!m_filter.isValExpected(value)) {
                m_doingSetValue = true;
                m_value = value;
                String orient = "h";
                float splitLocation = (float)0.5;
                StringTokenizer toker = new StringTokenizer(value);
                if (toker.hasMoreTokens()) {
                    orient = toker.nextToken();
                }
                if (toker.hasMoreTokens()) {
                    try {
                        splitLocation = Float.parseFloat(toker.nextToken());
                    } catch (NumberFormatException nfe) {}
                }
                splitLocation = Math.max(0, Math.min(1, splitLocation));

                if (!orient.equals(m_orient)) {
                    m_orient = orient;
                    if (orient.startsWith("v")) {
                        m_splitPane.setOrientation(VERTICAL);
                    } else if (orient.startsWith("h")) {
                        m_splitPane.setOrientation(HORIZONTAL);
                    }
                }
                Messages.postDebug("SplitPane",
                                   title + ": orient=" + orient
                                   + ", splitLocation=" + splitLocation);
                //Messages.writeStackTrace(new Exception("DEBUG STACK TRACE"));
                m_splitLocation = splitLocation;
                m_splitPane.setDividerLocation(splitLocation);
                m_splitPane.setVisible(true);
                
                m_splitPane.invalidate();
                m_doingSetValue = false;
            }
        }
    }

    /**
     *  Sets the value for various attributes.
     *  @param attr attribute to be set.
     *  @param c    value of the attribute.
     */
    public void setAttribute(int attr, String c) {
        if (c != null) {
            c = c.trim();
            if (c.length() == 0) {
                c = null;
            }
        }
        switch (attr) {
          case SETVAL:
            m_setval = c;
            break;
          default:
            super.setAttribute(attr, c);
            break;
        }
    }

    /**
     *  Gets that value of the specified attribute.
     *  @param attr attribute whose value should be returned.
     */
    public String getAttribute(int attr) {
        String strAttr;
        switch (attr) {
          case SETVAL:
            strAttr = m_setval;
            break;
          case VALUE:
            strAttr = m_value;
            break;
          default:
            strAttr = super.getAttribute(attr);
            break;
        }
        return strAttr;
    }

    /**
     *  Returns the attributes for this graph.
     */
    public Object[][] getAttributes()
    {
        return attributes;
    }

    /**
     *  Attribute array for panel editor (ParamInfoPanel)
     */
    private final static Object[][] attributes = {
        {new Integer(VARIABLE),     "Vnmr variables:"},
        {new Integer(SETVAL),       "Orientation expression:"},
        {new Integer(SHOW),         "Enable condition:"},
    };

    public void loadPanes() {
        if (m_loaded) {
            return;
        }
        synchronized (getTreeLock()) {
            Component[] comps = getComponents();
            int nComponents = comps.length;
            int j = 1;
            for (int i = 0; i < nComponents; i++) {
                if (comps[i] instanceof Container) {
                    // Remove color from divider bar
                    Component[] subcomps;
                    subcomps = ((Container)comps[i]).getComponents();
                    for (int k = 0; k < subcomps.length; k++) {
                        if (subcomps[k] instanceof BasicSplitPaneDivider) {
                            subcomps[k].setBackground(null);
                        }
                    }
                }
                if (!(comps[i] instanceof JSplitPane)) {
                    if (j <= 2) {
                        if (j == 1) {
                            m_splitPane.setLeftComponent(comps[i]);
                        } else {
                            m_splitPane.setRightComponent(comps[i]);
                        }
                        if (comps[i] instanceof VObjIF) {
                            ((VObjIF)comps[i]).updateValue();
                        }
                    }
                    remove(comps[i]);
                    j++;
                }
            }
        }
        m_doingSetValue = true;
        initSplitPane();
        m_splitPane.setDividerLocation(0.5);
        m_loaded = true;
        m_doingSetValue = false;
    }

    /*
     * LayoutManager methods
     */
    public void addLayoutComponent(String name, Component comp) {}

    public void removeLayoutComponent(Component comp) {}

    public Dimension preferredLayoutSize(Container target) {
        return new Dimension(0, 0);
    } // unused

    public Dimension minimumLayoutSize(Container target) {
        return new Dimension(0, 0);
    } // unused

    /**
     * Layout the Split Pane in the panel
     * @param target component to be laid out
     */
    public void layoutContainer(Container target) {
        if (m_splitPane == null) {
            return;
        }
        synchronized (target.getTreeLock())
        {
            m_doingSetValue = true;
            Dimension targetSize = target.getSize();
            Insets insets = target.getInsets();
            int width = targetSize.width - insets.left - insets.right;
            int height = targetSize.height - insets.top - insets.bottom;

            Component leftComp = m_splitPane.getLeftComponent();
            Component rightComp = m_splitPane.getRightComponent();
            if (width <= 0 || height <= 0) {
                if (leftComp != null) {
                    leftComp.setSize(0, 0);
                }
                if (rightComp != null) {
                    rightComp.setSize(0, 0);
                }
            }

            Messages.postDebug("VSplitPane",
                               title + ": setBounds(" + insets.left + ", "
                               + insets.top + ", " + width + "x"
                               + height + ")");
            m_splitPane.setBounds(insets.left, insets.top, width, height);
            if (!m_loaded) {
                loadPanes();
            }
            if (m_firstLayout) {
                initSplitPane();
                m_splitPane.setDividerLocation(0.5);
                m_firstLayout = false;
            }
            m_doingSetValue = false;
        }
    }

    /**
     * Gets the divider location as a ratio of the first panel size
     * to the sum of the panel sizes.  (Panel "size" being the length
     * in the direction split by the divider.)
     * Also gives an idea of the precision (limited by the finite
     * number of pixels) by returning the next lower and next higher
     * possible ratios.
     * @return An array of 3 doubles giving, the actual location, the next
     * smaller possible location, the next larger location.
     */
    public double[] getProportionalDividerLocation() {
        double[] rtn = new double[3];
        int width;
        int divide = m_splitPane.getDividerLocation();
        int dwidth = m_splitPane.getDividerSize();
        if (m_splitPane.getOrientation() == JSplitPane.VERTICAL_SPLIT) {
            width = m_splitPane.getHeight();
        } else {
            width = m_splitPane.getWidth();
        }
        int w1 = divide - 1;
        int w2 = width - divide - dwidth - 1;
        rtn[0] = w1 / (double)(w1 + w2);
        rtn[1] = (w1 - 1) / (double)(w1 + w2);
        rtn[2] = (w1 + 1) / (double)(w1 + w2);
        return rtn;
    }

    /*
     * PropertyChangeListener method
     */
    public void propertyChange(PropertyChangeEvent pce) {
//        if(bg!=null) {
//            bgColor=DisplayOptions.getColor(bg);
//            setBackground(bgColor);
//        }
        String name = pce.getPropertyName();
        if (name != null && name.equals("dividerLocation")) {
            double split = getProportionalDividerLocation()[0];
            String orient;
            if (m_splitPane.getOrientation() == JSplitPane.VERTICAL_SPLIT) {
                orient = "v";
            } else {
                orient = "h";
            }
            m_value = orient + " " + Fmt.f(3, split);
            Messages.postDebug("SplitPane", "propertyChange: split=" + m_value);
            if (!m_doingSetValue && showCmd != null) {
                m_filter.setLastVal(m_value);
                vnmrIf.sendVnmrCmd(this, showCmd);
            }
        }
    }
    
}
