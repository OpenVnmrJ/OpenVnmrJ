/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import javax.swing.*;
import javax.swing.border.BevelBorder;
import javax.swing.border.Border;

import java.awt.*;
import java.awt.event.*;
import java.beans.PropertyChangeEvent;
import java.util.Set;
import java.util.TreeSet;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;

import static vnmr.lc.LcMethodParameter.*;


public class VLcTableItem extends VObj
    implements VObjDef, ActionListener, FocusListener,
               LayoutManager, ActionComponent {

    private static Border sm_downBevel
        = BorderFactory.createBevelBorder(BevelBorder.LOWERED);
    private static Border sm_upBevel
        = BorderFactory.createBevelBorder(BevelBorder.RAISED);
    private static String sm_tooltip
        = Util.getLabel("Insert/Remove from Method Table");

    protected final static Color FADED_FILL_COLOR = new Color(235, 235, 200);
    protected final static Color FADED_FRAME_COLOR = new Color(100, 100, 100);


    private JLabel m_itemLabel;
    private JLabel m_unitLabel;
    private String m_strUnits = "";
    private JComponent m_valueItem;
    private ArrayToggle m_arrayButton;
    private String m_parName = null; // Use like vnmrVar (VObjDef.VARIABLE)
    private String m_actionCmd = null;
    private String m_strType = null;
    private String m_strTabled = null;
    private String m_strLabelWidth = null;
    private String m_strEntryWidth = null;
    private LcMethod m_method = null;
    private boolean m_firstTime = true;
    private String m_lastVals = "";
    private Set<ActionListener> m_actionListenerList
        = new TreeSet<ActionListener>();
    private Border m_entryBorder
        = BorderFactory.createBevelBorder(BevelBorder.LOWERED);



    public VLcTableItem(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
        setLayout(this);
        setEntryBackground();
    }

    private boolean isTrue(String strValue) {
        return (strValue != null && strValue.startsWith("y"));
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
          // Don't let these get set
          case SETVAL:
            valueExpr = null;
            break;
          case SHOW:
            showExpr = null;
            break;

          case PANEL_PARAM:
            m_parName = c;
            break;
          case ACTIONCMD:
            m_actionCmd = c;
            break;
          case SUBTYPE:
            m_strType = c;
            initialize();
            break;
          case TABLED:
            m_strTabled = c;
            break;
          case SIZE1:
            m_strLabelWidth = c;
            break;
          case SIZE2:
            m_strEntryWidth = c;
            break;
          case UNITS:
            m_strUnits = c;
            break;
          case VALUE:
            // NB: Do not use
            Messages.postDebug("VLcTableItem.setAttribute: "
                               + "Tried to set VALUE attribute");
            // no need to handle arrayed values
            //setValue(c);
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
        String strAttr = null;
        switch (attr) {
          case PANEL_PARAM:
            strAttr = m_parName;
            break;
          case ACTIONCMD:
              strAttr = m_actionCmd;
              break;
          case SUBTYPE:
            strAttr = m_strType;
            break;
          case TABLED:
            strAttr = m_strTabled;
            break;
          case VALUE:
            if (m_strType.equals(BOOLEAN_TYPE)) {
                strAttr = ((AbstractButton)m_valueItem).isSelected() ?
                        "y" : "n";
            } else if (m_strType.equals(MS_SCAN_TYPE)) {
                strAttr = ((JComboBox)m_valueItem).getSelectedItem().toString();
            } else {
                strAttr = ((JTextField)m_valueItem).getText();
            }
            break;
          case SIZE1:
            strAttr = m_strLabelWidth;
            break;
          case SIZE2:
            strAttr = m_strEntryWidth;
            break;
          case UNITS:
            strAttr = m_strUnits;
            break;
          default:
            strAttr = super.getAttribute(attr);
            break;
        }
        return strAttr;
    }

    public boolean initialize() {
        if (m_firstTime) {
            Messages.postDebug("VLcTableItem", "Initializing " + m_parName);
            m_itemLabel = new JLabel(label);
            m_itemLabel.setOpaque(false);
            m_itemLabel.setHorizontalAlignment(SwingConstants.LEFT);
            add(m_itemLabel);
            if (m_strLabelWidth != null) {
                int height = getSize().height;
                int width = 0;
                try {
                    width = Integer.parseInt(m_strLabelWidth);
                    m_itemLabel.setPreferredSize(new Dimension(width, height));
                } catch (NumberFormatException nfe) {
                    Messages.postDebug("VLcTableItem: Bad label width spec in "
                                       + label + ": " + m_strLabelWidth);
                }
            }

            m_unitLabel = new JLabel(m_strUnits);
            m_unitLabel.setOpaque(false);
            m_unitLabel.setHorizontalAlignment(SwingConstants.LEFT);
            add(m_unitLabel);
            {
                int height = getSize().height;
                int width = 10; // Dummy value
                m_unitLabel.setPreferredSize(new Dimension(width, height));
            }
        }
        m_itemLabel.setText(label);
        m_unitLabel.setText(m_strUnits);

        // Make select-for-putting-in-table button
        if (m_firstTime) {
            m_arrayButton = new ArrayToggle();
            m_arrayButton.setOpaque(false);
            m_arrayButton.addActionListener(this);
            m_arrayButton.setActionCommand("select " + m_parName);
            m_arrayButton.setToolTipText(sm_tooltip);
            add(m_arrayButton);
            {
                int height = getSize().height;
                int width = 25;
                m_arrayButton.setPreferredSize(new Dimension(width, height));
            }
        }
        if (m_strTabled == null) {
            m_strTabled = "never";      // The default
        }
        if (m_strTabled.equals("never") || m_strTabled.equals("no")) {
            m_arrayButton.setSelected(false);
            if (m_strTabled.equals("never")) {
                // Don't show checkbox
                m_arrayButton.setVisible(false);
            }
        } else if (m_strTabled.equals("always") || m_strTabled.equals("yes")) {
            m_arrayButton.setSelected(true);
        } else {
            //m_arrayButton.setActionCommand("select " + m_parName);
            m_arrayButton.setSelected(m_strTabled.startsWith("yes"));
            if (m_strTabled.startsWith("yes")) {
                setTabled(true);
            }
        }

        // Make the value widget (the "m_valueItem")
        if (m_strType == null) {
            m_strType = "";
        }
        if (value == null) {
            value = "";
        }
        if (m_strType.equals(STRING_TYPE)) {
            if (m_valueItem != null && !(m_valueItem instanceof JTextField)) {
                // Widget type changed - reinitialize
                remove(m_valueItem);
                remove(m_arrayButton);
                m_firstTime = true;
            }
            if (m_firstTime) {
                m_valueItem = new JTextField(value);
                m_valueItem.setBorder(m_entryBorder);
                ((JTextField)m_valueItem).setColumns(5); // TODO: Calc size
                ((JTextField)m_valueItem).setBackground(bgColor);
                m_valueItem.setOpaque(true);
                add(m_valueItem);
                setValue(value);
                ((JTextField)m_valueItem).addActionListener(this);
                ((JTextField)m_valueItem).addFocusListener(this);
            }
            ((JTextField)m_valueItem).setText(value);
            ((JTextField)m_valueItem).setActionCommand("entry " + m_parName);
        } else if (m_strType.equals(INT_TYPE)) {
            if (m_valueItem != null && !(m_valueItem instanceof JTextField)) {
                // Widget type changed - reinitialize
                remove(m_valueItem);
                remove(m_arrayButton);
                m_firstTime = true;
            }
            if (m_firstTime) {
                if (value.equals("")) {
                    value = "0";
                }
                m_valueItem = new JTextField(value);
                m_valueItem.setBorder(m_entryBorder);
                ((JTextField)m_valueItem).setColumns(5); // TODO: Calc size
                ((JTextField)m_valueItem).setBackground(bgColor);
                m_valueItem.setOpaque(true);
                add(m_valueItem);
                setValue(value);
                ((JTextField)m_valueItem).addActionListener(this);
                ((JTextField)m_valueItem).addFocusListener(this);
            }
            ((JTextField)m_valueItem).setText(value);
            ((JTextField)m_valueItem).setActionCommand("entry " + m_parName);
        } else if (m_strType.equals(DOUBLE_TYPE)) {
            if (m_valueItem != null && !(m_valueItem instanceof JTextField)) {
                // Widget type changed - reinitialize
                remove(m_valueItem);
                remove(m_arrayButton);
                m_firstTime = true;
            }
            if (m_firstTime) {
                if (value.equals("")) {
                    value = "0";
                }
                m_valueItem = new JTextField(value);
                m_valueItem.setBorder(m_entryBorder);
                ((JTextField)m_valueItem).setColumns(5); // TODO: Calc size
                ((JTextField)m_valueItem).setBackground(bgColor);
                m_valueItem.setOpaque(true);
                add(m_valueItem);
                setValue(value);
                ((JTextField)m_valueItem).addActionListener(this);
                ((JTextField)m_valueItem).addFocusListener(this);
            }
            ((JTextField)m_valueItem).setText(value);
            ((JTextField)m_valueItem).setActionCommand("entry " + m_parName);
        } else if (m_strType.equals(BOOLEAN_TYPE)) {
            if (m_valueItem != null && !(m_valueItem instanceof JToggleButton)) {
                // Widget type changed - reinitialize
                remove(m_valueItem);
                remove(m_arrayButton);
                m_firstTime = true;
            }
            if (m_firstTime) {
                m_valueItem= new OnOffToggle();
                int height = getSize().height;
                int width = 30;
                m_valueItem.setPreferredSize(new Dimension(width, height));
                m_valueItem.setOpaque(false);
                add(m_valueItem);
                setValue(value);
                ((AbstractButton)m_valueItem).addActionListener(this);
            }
            ((AbstractButton)m_valueItem).setSelected(isTrue(value));
            ((AbstractButton)m_valueItem).setActionCommand("entry " + m_parName);
        } else if (m_strType.equals(MS_SCAN_TYPE)) {
            if (m_valueItem != null && !(m_valueItem instanceof MsScanMenu)) {
                // Widget type changed - reinitialize
                remove(m_valueItem);
                remove(m_arrayButton);
                m_firstTime = true;
            }
            if (m_firstTime) {
                m_valueItem = new MsScanMenu();
                int height = getSize().height;
                int width = getWidth() - m_itemLabel.getWidth()
                    - m_arrayButton.getWidth();
                m_valueItem.setPreferredSize(new Dimension(width, height));
                m_valueItem.setOpaque(true);
                add(m_valueItem);
                ((MsScanMenu)m_valueItem).setSelectedItem(value);
                ((MsScanMenu)m_valueItem).addActionListener(this);
            }
            ((MsScanMenu)m_valueItem).setActionCommand("entry " + m_parName);
        } else {
            Messages.postDebug("Don't know about entry component type: \""
                               + m_strType + "\"");
            if (m_firstTime) {
                m_valueItem = new JTextField("");
                m_valueItem.setBorder(m_entryBorder);
                ((JTextField)m_valueItem).setBackground(bgColor);
                m_valueItem.setOpaque(true);
                add(m_valueItem);
            }
        }
        if (m_firstTime && m_valueItem instanceof JTextField) {
            int width = 4;
            if (m_strEntryWidth != null) {
                try {
                    width = Integer.parseInt(m_strEntryWidth);
                } catch (NumberFormatException nfe) {
                    Messages.postDebug("VLcTableItem: Bad entry width spec in "
                                       + label + ": " + m_strEntryWidth);
                }
            }
            int height = getSize().height;
            m_valueItem.setPreferredSize(new Dimension(10, height));// Dummy width
            ((JTextField)m_valueItem).setColumns(width);// Real width
        }

        if (font != null) {
            m_itemLabel.setFont(font);
            m_unitLabel.setFont(font);
            m_valueItem.setFont(font);
        }
        if (fgColor != null) {
            m_itemLabel.setForeground(fgColor);
            m_unitLabel.setForeground(fgColor);
            m_valueItem.setForeground(fgColor);
        }

        m_firstTime = false;
        return true;
    }

    /**
     * Convert a list of strings delimited by single quotes to a list
     * of strings delimited by "; ".
     */
    public String convertQuotedToSemicoloned(String qStr) {
        QuotedStringTokenizer toker
                = new QuotedStringTokenizer(qStr, ", ", "'");
        String sStr = "";
        while (toker.hasMoreTokens()) {
            if (sStr.length() > 0) {
                sStr = sStr + "; ";
            }
            sStr = sStr + toker.nextToken();
        }
        return sStr;
    }

    public boolean isSelected() {
        if (m_arrayButton == null) {
            return (m_strTabled != null && m_strTabled.startsWith("always"));
        } else {
            return m_arrayButton.isSelected();
        }
    }

    public void setSelected(boolean b) {
        if (b != isSelected()) {
            m_arrayButton.setSelected(b);
            handleChange();
        }
    }

    public void focusLost(FocusEvent evt) {
        Messages.postDebug("VLcTableItem",
                           "VLcTableItem.focusLost: " + m_parName);
        if (evt.getComponent() instanceof JTextField) {
            String cmd = "VObjAction";
            sendActionCmd(cmd);
        }
        handleChange();
    }
    
    public void focusGained(FocusEvent evt) {
    }

    public void actionPerformed(ActionEvent ae) {
        String cmd = ae.getActionCommand();
        Messages.postDebug("VLcTableItem",
                           "VLcTableItem.actionPerformed: " + cmd);

        if (cmd.startsWith("select")) {
            ArrayToggle button = (ArrayToggle)ae.getSource();
            setEntryBackground();
            sendActionCmd(cmd + (button.isSelected() ? " true" : " false"));
        } else {
            cmd = "VObjAction";
            sendActionCmd(cmd);
        }

        handleChange();
    }

    public void setEnabled(boolean b) {
        m_arrayButton.setEnabled(b);
        boolean isTabled = m_arrayButton.isSelected();
        m_valueItem.setEnabled(b && !isTabled);
    }

    public void setTabled(boolean isTabled) {
        m_arrayButton.setSelected(isTabled);
        m_valueItem.setEnabled(!isTabled);
    }

    private void handleChange() {
        setTabled(isSelected());
    }

    public void propertyChange(PropertyChangeEvent evt){
        changeFont();
        setEntryBackground();
     }

    /**
     * 
     */
    private void setEntryBackground() {
        if (isSelected()) {
            bgColor = null;
        } else {
            bgColor = DisplayOptions.getColor("VJEntryBG");
            if (bgColor != null && bgColor.equals(Color.BLACK)) {
                bgColor = Color.white;//Util.getBgColor();
            }
        }
        if (m_valueItem instanceof JTextField) {
            ((JTextField)m_valueItem).setBackground(bgColor);
        }

    }

   /**
     * Sets the single value of this item from a ParamIF.
     * If the parameter is arrayed, set the first array value (and this
     * item is disabled).
     */
     public void setValue(ParamIF pf) {
        Messages.postDebug("lcVnmrComm",
                           "VLcTableItem[" + m_parName + "].setValue(): \""
                           + pf.value + "\"");
        super.setValue(pf);
        String pfval = pf.value;
        if (pfval != null) {
            setValue(pfval);
        }
     }

    /**
     * Sets item to a single value (not in table).
     */
    public void setValue(String val) {
        if (m_strType == null || m_valueItem == null) {
            return;
        }
        if (m_strType.equals(STRING_TYPE)) {
            ((JTextField)m_valueItem).setText(val);
        } else if (m_strType.equals(INT_TYPE)) {
            ((JTextField)m_valueItem).setText(val);
        } else if (m_strType.equals(DOUBLE_TYPE)) {
            ((JTextField)m_valueItem).setText(val);
        } else if (m_strType.equals(BOOLEAN_TYPE)) {
            ((AbstractButton)m_valueItem).setSelected(isTrue(val));
        } else if (m_strType.equals(MS_SCAN_TYPE)) {
            ((MsScanMenu)m_valueItem).setSelectedItem(val);
        }
        if (m_method != null && m_method.getRowCount() > 1
            && !m_strTabled.equals("always"))
        {
            setSelected(false);
        }
        handleChange();
    }

    public Object getValue() {
        if (m_strType.equals(STRING_TYPE)) {
            return ((JTextField)m_valueItem).getText();
        } else if (m_strType.equals(INT_TYPE)) {
            Integer val;
            try {
                val = new Integer(((JTextField)m_valueItem).getText());
            } catch (NumberFormatException nfe) {
                val = new Integer(0);
            }
            return val;
        } else if (m_strType.equals(DOUBLE_TYPE)) {
            Double val;
            try {
                val = new Double(((JTextField)m_valueItem).getText());
            } catch (NumberFormatException nfe) {
                val = new Double(0);
            }
            return val;
        } else if (m_strType.equals(BOOLEAN_TYPE)) {
            return new Boolean(((AbstractButton)m_valueItem).isSelected());
        } else if (m_strType.equals(MS_SCAN_TYPE)) {
            return ((JComboBox)m_valueItem).getSelectedItem().toString();
        } else {
            Messages.postDebug("Can't deal with entry component type: \""
                               + m_strType + "\"");
            return "";
        }
    }

    public Object[][] getAttributes()
    {
        return attributes;
    }

    private final static String[] types
        = {STRING_TYPE, INT_TYPE, DOUBLE_TYPE, BOOLEAN_TYPE};
    private final static String[] tableOptions
        = {"yes", "no", "always", "never", "invisible"};

    private final static Object[][] attributes = {
        {new Integer(LABEL),    "Label of item:"},
        {new Integer(VARIABLE), "Vnmr variables:"},
        {new Integer(SUBTYPE),  "Type:", "menu", types},
        {new Integer(SETVAL),   "Value:"},
        {new Integer(TABLED),   "Put in table:", "menu", tableOptions},
        {new Integer(SIZE1),    "Label Width:"},
        {new Integer(SIZE2),    "Entry Width:"},
        {new Integer(UNITS),    "Units:"},
        {new Integer(LAYOUT),   "Layout hints:"},
    };

    public void addLayoutComponent(String name, Component comp) {}

    public void removeLayoutComponent(Component comp) {}

    public Dimension preferredLayoutSize(Container target) {
        return new Dimension(0, 0); // unused
    }

    public Dimension minimumLayoutSize(Container target) {
        return new Dimension(0, 0); // unused
    }

    /**
     * Do the layout
     * @param target component to be laid out
     */
    public void layoutContainer(Container target) {
        final int PAD = 3;
        synchronized (target.getTreeLock()) {
            if (m_itemLabel == null || m_valueItem == null || m_arrayButton == null) {
                return;
            }
            Dimension targetSize = target.getSize();
            Insets insets = target.getInsets();
            Dimension labelDim = m_itemLabel.getPreferredSize();
            Dimension entryDim = m_valueItem.getPreferredSize();
            Dimension selectDim = m_arrayButton.getPreferredSize();
            if (!m_arrayButton.isVisible()) {
                selectDim.width = 0;
            }
            int width = targetSize.width - insets.left - insets.right;
            int height = targetSize.height - insets.top - insets.bottom;

            // Lay out: Select - Label - Entry - Units
            int x1 = insets.left;
            int x2 = x1 + selectDim.width;
            int x3 = x2 + labelDim.width;
            int x4 = "0".equals(m_strEntryWidth)
                    ? x3
                    : Math.min(x3 + entryDim.width, x1 + width);
            int x5 = x1 + width;
            int y1 = (height - selectDim.height) / 2;
            int y2 = (height - labelDim.height) / 2;
            int y3 = (height - entryDim.height) / 2;
            m_arrayButton.setBounds(x1, y1, x2 - x1, selectDim.height);
            m_itemLabel.setBounds(x2, y2, x3 - x2, labelDim.height);
            m_valueItem.setBounds(x3, y3, x4 - x3, entryDim.height);
            m_unitLabel.setBounds(x4 + PAD, y2, x5 - x4 - PAD, labelDim.height);

            // Draw a border above and below the label
            int ww = (m_valueItem.getWidth() > 0) ? 0 : 1;
            Color frameColor = getBackground().darker();
            Border out = BorderFactory.createMatteBorder(1, 0, 1, ww,
                                                         frameColor);
            Border in = BorderFactory.createEmptyBorder(0, PAD, 0, PAD);
            m_itemLabel.setBorder(BorderFactory.createCompoundBorder(out, in));

            // Lay out: Label - Select - Entry - Units
            /*
            int x1 = insets.left;
            int x2 = x1 + labelDim.width + PAD2;
            int x3 = x2 + selectDim.width + PAD;
            int x4 = Math.min(x3 + entryDim.width, x1 + width);
            int x5 = x1 + width;
            int y1 = (height - selectDim.height) / 2;
            int y2 = (height - labelDim.height) / 2;
            int y3 = (height - entryDim.height) / 2;
            m_itemLabel.setBounds(x1, y1, x2 - x1 - PAD2, selectDim.height);
            m_arrayButton.setBounds(x2, y2, x3 - x2 - PAD, labelDim.height);
            m_valueItem.setBounds(x3, y3, x4 - x3, entryDim.height);
            m_unitLabel.setBounds(x4, y2, x5 - x4, labelDim.height);
            */
        }
    }


    public class OnOffToggle extends JToggleButton implements ItemListener {

        public OnOffToggle() {
            super();
            setIcon(new OnOffIcon(this));
            setBorder(sm_upBevel);
            setPreferredSize(new Dimension(30, 20));
            addItemListener(this);
        }

        public void itemStateChanged(ItemEvent ie) {
            if (ie.getStateChange() == ItemEvent.SELECTED) {
                setBorder(sm_downBevel);
            } else {
                setBorder(sm_upBevel);
            }
        }

        private class OnOffIcon implements Icon {

            OnOffToggle m_button;

            public OnOffIcon(OnOffToggle parent) {
                m_button = parent;
            }

            private Rectangle getIconRectangle() {
                int w = m_button.getWidth();
                int h = m_button.getHeight();
                Insets inset = m_button.getInsets();
                Rectangle r = new Rectangle(inset.left, inset.top,
                                            w - inset.left - inset.right,
                                            h - inset.top - inset.bottom);
                return r;
            }

            public int getIconHeight() {
                Rectangle rect = getIconRectangle();
                return rect.height;
            }

            public int getIconWidth() {
                Rectangle rect = getIconRectangle();
                return rect.width;
            }

            public void paintIcon(Component c, Graphics g, int xpos, int ypos) {
                Graphics2D g2 = (Graphics2D)g;
                g2.setFont(font);
                String text = m_button.isSelected() ? "On" : "Off";
                Rectangle r = getIconRectangle();
                FontMetrics metrics = g2.getFontMetrics();
                int stringWidth = metrics.stringWidth(text);
                int stringHeight = metrics.getHeight();
                int x = r.x + (r.width - 1) / 2 - stringWidth / 2;
                 // Font height tends to be too big; fudge by 2 pixels:
                int y = r.y + (r.height - 1) / 2 + stringHeight / 2 - 2;
                if (m_button.isEnabled()) {
                    if (m_button.isSelected()) {
                        g.setColor(Util.changeBrightness(c.getBackground(),
                                                         -10));
                        g.fillRect(r.x, r.y, r.width, r.height);
                    }
                    g2.setColor(m_button.getForeground());
                    g2.drawString(text, x, y);
                } else {
                    g2.setColor(Color.WHITE);
                    g2.drawString(text, x, y);
                    g2.setColor(m_button.getBackground().darker());
                    g2.drawString(text, x - 1, y - 1);
                }
            }
        }
    }

    class ArrayToggle extends JToggleButton implements ItemListener {
        
        public ArrayToggle() {
            setIcon(new TableIcon(this));
            setBorder(sm_upBevel);
            setPreferredSize(new Dimension(25, 20));
            addItemListener(this);
        }


        public void itemStateChanged(ItemEvent ie) {
            if (ie.getStateChange() == ItemEvent.SELECTED) {
                setBorder(sm_downBevel);
            } else {
                setBorder(sm_upBevel);
            }
        }

        private class TableIcon implements Icon {

            ArrayToggle m_button;

            public TableIcon(ArrayToggle button) {
                m_button = button;
            }

            private Rectangle getIconRectangle() {
                int w = m_button.getWidth();
                int h = m_button.getHeight();
                Insets inset = m_button.getInsets();
                Rectangle r = new Rectangle(inset.left, inset.top,
                                            w - inset.left - inset.right,
                                            h - inset.top - inset.bottom);
                return r;
            }

            public int getIconHeight() {
                Rectangle rect = getIconRectangle();
                return rect.height;
            }

            public int getIconWidth() {
                Rectangle rect = getIconRectangle();
                return rect.width;
            }

            public void paintIcon(Component c, Graphics g, int xpos, int ypos) {
                Rectangle r = getIconRectangle();
                int minHorzMargin = 2;
                int minVertMargin = 1;
                int x0 = r.x + minHorzMargin;
                int y0 = r.y + minVertMargin;
                int x1 = x0 + (r.width - 1) - 2 * minHorzMargin;
                int y1 = y0 + (r.height - 1) - 2 * minVertMargin;

                Color fillColor = m_button.isEnabled()
                        ? Color.YELLOW : FADED_FILL_COLOR;
                Color frameColor = m_button.isEnabled()
                        ? Color.BLACK : FADED_FRAME_COLOR;
            
                if (m_button.isSelected()) {
                    Color bg = Util.changeBrightness(c.getBackground(), -10);
                    g.setColor(bg);
                    g.fillRect(r.x, r.y, r.width, r.height);
                    g.setColor(frameColor);
                    g.drawRect(x0, y0, x1 - x0, y1 - y0);
                    int colWidth = 7;
                    int nCols = (x1 - x0) / colWidth;
                    nCols = Math.max(nCols, 2);
                    colWidth = (x1 - x0) / nCols;
                    for (int i = 1; i < nCols; i++) {
                        int x = x0 + i * colWidth;
                        if (i == nCols - 1) {
                            g.setColor(fillColor);
                            g.fillRect(x + 1, y0 + 1,
                                       x1 - x - 1, y1 - y0 - 1);
                            g.setColor(frameColor);
                        }
                        g.drawLine(x, y0, x, y1);
                    }
                    int rowHeight = 4;
                    int nRows = (y1 - y0) / rowHeight;
                    nRows = Math.max(nRows, 2);
                    rowHeight = (y1 - y0) / nRows;
                    for (int i = 1; i < nRows; i++) {
                        int y = y0 + i * rowHeight;
                        g.drawLine(x0, y, x1, y);
                    }
                } else {
                    int cellWidth = 2 * (x1 - x0) / 3;
                    cellWidth = Math.max(cellWidth, 10);
                    if (cellWidth < x1 - x0) {
                        int dw = x1 - x0 - cellWidth;
                        x0 += dw / 2;
                        x1 -= dw / 2;
                    }
                    int cellHeight = cellWidth / 2;
                    if (cellHeight < y1 - y0) {
                        int dh = y1 - y0 - cellHeight;
                        y0 += dh / 2;
                        y1 -= dh / 2;
                    }
                    g.setColor(fillColor);
                    g.fillRect(x0, y0, x1 - x0 - 1, y1 - y0 - 1);
                    g.setColor(frameColor);
                    g.drawRect(x0, y0, x1 - x0 - 1, y1 - y0 - 1);
                }
            }
        }
    }

    /**
     * Send an ActionEvent to all the action listeners, only if the
     * actionCmd has been set.
     */
    private void sendActionCmd() {
        sendActionCmd(m_actionCmd);
    }

    /**
     * Send a specific action command to the action listeners.
     * @param actionCmd The command string to send.
     */
    private void sendActionCmd(String actionCmd) {
        if (actionCmd != null) {
            ActionEvent event = new ActionEvent(this, hashCode(), actionCmd);
            for (ActionListener listener : m_actionListenerList) {
                listener.actionPerformed(event);
            }
        }
    }


    // ActionComponent interface

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

}
