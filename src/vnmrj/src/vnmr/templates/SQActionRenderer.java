/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.templates;

import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Insets;
import java.awt.LayoutManager;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.Map;
import java.util.TreeMap;

import javax.swing.BorderFactory;
import javax.swing.JLabel;
import javax.swing.border.Border;

import vnmr.bo.VObj;
import vnmr.util.DisplayOptions;
import vnmr.util.FileUtil;
import vnmr.util.Fmt;
import vnmr.util.Messages;
import vnmr.util.Util;
import vnmr.util.DisplayOptions.DisplayStyle;

import static vnmr.templates.ProtocolBuilder.*;

public class SQActionRenderer extends VObj {

    /**
     * This ad hoc margin is the difference between the label's width and
     * it's container's width. ("This" is the container.)
     */
    public static final int LABEL_MARGIN = 8;

    private VElement m_model;
    private Color m_defaultBg;

    private JLabel m_label = new JLabel();

    private Border m_idleBorder
        = BorderFactory.createMatteBorder(1, 2, 1, 2, Color.lightGray);

    private Border m_selBorder
        = BorderFactory.createMatteBorder(1, 2, 1, 2, Color.red.darker());

    private Border m_idleSquishBorder
        = BorderFactory.createMatteBorder(1, 2, 0, 2, Color.lightGray);

    private Border m_selSquishBorder
        = BorderFactory.createMatteBorder(1, 2, 0, 2, Color.red.darker());

    private String m_labelLayout =
        "<HTML><TABLE WIDTH=%width% CELLSPACING=0 CELLPADDING=1>"
        + "<TR><TD NOWRAP VALIGN=BOTTOM><B>%title%</B></TD>"
        + "<TD NOWRAP VALIGN=BOTTOM><SMALL>%status%</SMALL></TD>"
        + "<TD NOWRAP VALIGN=BOTTOM ALIGN=RIGHT><SMALL>%\\ \\[time\\]%</SMALL></TD></TR>"
        + "</TABLE>"
        ;

    private String m_tooltipLayout = "%tooltext%";

    /** Map of node status to status label. */
    private Map<String, String> m_statusLabelMap;

    private long m_tipReadTime = 0;

    private Dimension m_widgetSize = new Dimension();

    protected static Dimension m_maxWidgetSize = null;


    /**
     * A Swing component used to display the VActionElement nodes in
     * the ProtocolBuilder's PBTree.
     */
    public SQActionRenderer() {
        m_model = null;
        setBorder(BorderFactory.createLineBorder(Color.GRAY));

        setLayout(new MyLayout());
        add(m_label);
        readLayouts();
        m_statusLabelMap = readStatusLabels();
        m_defaultBg = getBackground();
    }

    public String getToolTipText() {
        long now = System.currentTimeMillis();
        String layout = readLayout("ActionToolTipLayout", m_tipReadTime);
        if (layout != null) {
            m_tooltipLayout = layout;
        }
        m_tipReadTime = now;
        String label = replaceAttrsWithValues(m_tooltipLayout, false);
        return label.length() == 0 ? null : label;
    }

    /**
     * Read in (update) the layout for the Action's label.
     */
    public void readLayouts() {
        String layout = readLayout("ActionLabelLayout", 0);
        if (layout != null) {
            m_labelLayout = layout;
        }
    }

    private String readLayout(String name, long lastRead) {
        String labelSpec = null;
        String filespec = "INTERFACE/" + name + ".html";
        String path = FileUtil.openPath(filespec);
        if (new File(path).lastModified() > lastRead) {
            if (path == null) {
                Messages.postDebug("traceXML", "File not found: " + filespec);
            } else {
                Messages.postDebug("traceXML", "Reading: " + path);
                BufferedReader reader = null;
                try {
                    reader = new BufferedReader(new FileReader(path));
                    String str;
                    StringBuffer sb = new StringBuffer();
                    while ((str = reader.readLine()) != null) {
                        if (!str.trim().startsWith("#")) {
                            sb.append(str);
                        }
                    }
                    str = sb.toString();
                    if (str.trim().length() > 0) {
                        labelSpec = str;
                    }
                } catch (FileNotFoundException e) {
                    Messages.writeStackTrace(e);
                } catch (IOException e) {
                    Messages.writeStackTrace(e);
                } finally {
                    try {
                        reader.close();
                    } catch (Exception e) { }
                }
            }
        }
        return labelSpec;
    }

    private Map<String, String> readStatusLabels() {
        Map<String, String> map = new TreeMap<String, String>();
        String filespec = "INTERFACE/SQStatusLabels.txt";
        String path = FileUtil.openPath(filespec);
        if (path != null) {
            Messages.postDebug("traceXML", "Reading: " + path);
            BufferedReader reader = null;
            try {
                reader = new BufferedReader(new FileReader(path));
                String str;
                while ((str = reader.readLine()) != null) {
                    Messages.postDebug("SQStatusLabels",
                                       "readStatusLabels: str=" + str);
                    if (!str.trim().startsWith("#")) {
                        String[] keyVal = str.split("[ \t:=]+", 2);
                        if (keyVal.length >= 1) {
                            String key = keyVal[0].toLowerCase();
                            String val = keyVal.length == 1 ? "" : keyVal[1];
                            Messages.postDebug("SQStatusLabels",
                                               "readStatusLabels: key=\"" + key
                                               + "\", val=\"" + val + "\"");
                            map.put(key, val);
                        }
                    }
                }
            } catch (FileNotFoundException e) {
                // Don't insist on it
            } catch (IOException e) {
                Messages.writeStackTrace(e);
            } finally {
                try {
                    reader.close();
                } catch (Exception e) { }
            }
        }
        return map;
    }

    public double getProgress() {
        double progress = 0;
        try {
            if (m_model != null) {
                String str = m_model.getAttribute(ATTR_PROGRESS);
                progress = Double.valueOf(str);
            }
        } catch (NumberFormatException nfe) {
        }
        return progress;
    }

    public String getStatus() {
        String status = "";
        if (m_model != null) {
            status = m_model.getAttribute(ATTR_STATUS);
        }
        return status;
    }

    public String getId() {
        String id = "";
        if (m_model != null) {
            id = m_model.getAttribute(ATTR_ID);
        }
        return id;
    }

    public SQActionRenderer setToDisplay(VActionElement protocolNode,
                                         boolean isSelected)
    {
        m_model = protocolNode;
        if (m_maxWidgetSize != null) {
            // Adjust our width for this node's indentation
            m_widgetSize .height = m_maxWidgetSize.height;
            m_widgetSize.width = m_maxWidgetSize.width - m_model.getIndent();
        }
        String type = "Action";
        setBorder(isSelected);
        DisplayStyle style = getStyleForNode(m_model, type);
        font = style.getFont();
        Color color = style.getFontColor();
        m_label.setFont(font);
        m_label.setForeground(color);
        m_label.setText(replaceAttrsWithValues(m_labelLayout, true));
        setNodeBackground(getStatus());
        return this;
    }

    private String replaceAttrsWithValues(String layout, boolean isLabel) {
        StringBuffer label = new StringBuffer();
        String[] tokens = layout.split("%");
        // Odd number tokens are attribute names
        for (int i = 1; i < tokens.length; i += 2) {
            tokens[i] = replaceNameWithValue(tokens[i], isLabel);
        }
        for (String token : tokens) {
            label.append(token);
        }
        // Make escaped spaces no-break
        String strLabel = label.toString().replaceAll("\\\\ ", "\u2002");
        return strLabel;
    }

    private String replaceNameWithValue(String str, boolean isLabel) {
        String bareName = str.replaceAll("\\\\.", "");
        String value = getAttribute(bareName, isLabel);
        if (value == null || value.trim().length() == 0) {
            return "";
        }
        String text = str.replace(bareName, value)
            .replaceAll("\\\\ ", "\u2002") // Make escaped spaces no-break
            .replaceAll("\\\\", ""); // Remove backslashes
        return text;
    }

    protected String getAttribute(String name) {
        return getAttribute(name, false);
    }

    protected String getAttribute(String name, boolean isLabel) {
        if ("width".equals(name)) {
            if (m_maxWidgetSize == null) {
                // Means we are getting the minimum size for this node
                return "0";
            } else {
                return Integer.toString(m_widgetSize.width);
            }
        }
        if ("height".equals(name)) {
            if (m_maxWidgetSize == null) {
                // Means we are getting the minimum size for this node
                return "0";
            } else {
                return Integer.toString(m_widgetSize.height);
            }
        }
        String value = "";
        if (m_model != null) {
            value = m_model.getAttribute(name);
            if (ATTR_TIME.equals(name)) {
                value = getTimeString(value, isLabel);
            } else if (isLabel && value != null) {
                if (ATTR_STATUS.equals(name)) {
                    value = getStatusLabel(value);
                } else if (ATTR_TITLE.equals(name)) {
                    value = value.replaceAll("\\[null\\]", "");
                }
            }
        }
        return value;
    }

    /**
     * Get the label to display for a given status.
     * @param value The status.
     * @return The label to display.
     */
    private String getStatusLabel(String value) {
        // Get label to display for this status
        String label = m_statusLabelMap.get(value.toLowerCase());
        Messages.postDebug("SQStatusLabels", "GetStatusLabel: value=" + value
                           + ", label=" + label);
        if (label == null) {
            label = value;
        }
        if (label.trim().length() > 0) {
            // Translate the label
            String key = "SQNode_" + label;
            label = Util.getLabel(key, label);
        }
        return label.trim();
    }

    /**
     * @param timeAttr The value of the "time" attribute (ATTR_TIME).
     * @param isLabel True if this is being retrieved for a label.
     * @return The string to use for the time label.
     */
    private String getTimeString(String timeAttr, boolean isLabel) {
        String timeLabel = null;
        if (ProtocolBuilder.isTimeShown()) {
            // Check for ATTR_TIMEDAY and ATTR_TIMENIGHT
            timeLabel = getDayNightTimes();
            if (timeLabel == null || timeLabel.length() == 0) {
                // Try the ATTR_TIME string
                timeLabel = reformatTimeAttr(timeAttr);
            }
        }
        if (timeLabel == null || timeLabel.equals("null")) {
            timeLabel = "";
        }
        return timeLabel;
    }

    private String reformatTimeAttr(String value) {
        if (value == null || value.trim().length() == 0) {
            return "";
        }

        String timeLabel = null;
        value = value.trim();
        // Maybe it's a single value
        try {
            Double.parseDouble(value);
            timeLabel = toHms(value);
        } catch (NumberFormatException nfe) {}
        if (timeLabel != null) {
            return timeLabel;
        }

        // Parse a complicated string,
        // e.g.: "Day:11 min, 4 sec Night: 1 min, 24 sec"
        String val = value.toLowerCase();
        val = val.replaceAll("[:]", "");
        int dayIdx = val.indexOf("day");
        int nightIdx = val.indexOf("night");
        if (dayIdx >= 0 || nightIdx >= 0) {
            timeLabel = "";
            if (dayIdx >= 0) {
                String daystr = nightIdx > dayIdx
                ? val.substring(dayIdx + 3, nightIdx)
                        : val.substring(dayIdx + 3);
                timeLabel += toHms(daystr);
                if (!timeLabel.equals("") && nightIdx > dayIdx) {
                    timeLabel += " / ";
                }
            }
            if (nightIdx >= 0) {
                String nightstr = val.substring(nightIdx + 5);
                timeLabel += toHms(nightstr);
            }
        } else {
            // Parse something like: "2 hr, 11 min, 4 sec"
            timeLabel = toHms(value);
        }

        if (timeLabel == null) {
            timeLabel = value;
        }

        return timeLabel;
    }

    private String getDayNightTimes() {
        String daytime = getAttribute(ATTR_TIMEDAY);
        if (daytime != null && daytime.length() > 0) {
            daytime = toHms(daytime);
        } else {
            daytime = null;
        }

        String nighttime = getAttribute(ATTR_TIMENIGHT);
        if (nighttime != null && nighttime.length() > 0) {
            nighttime = toHms(nighttime);
        } else {
            nighttime = null;
        }

        String timeLabel = null;
        if (daytime != null || nighttime != null) {
            if (daytime != null) {
                timeLabel = daytime;
                if (nighttime != null) {
                    timeLabel += " / " + nighttime;
                }
            } else {
                timeLabel = nighttime;
            }
        }
        return timeLabel;
    }

    protected void setAttribute(String name, String value) {
        if (m_model != null) {
            m_model.setAttribute(name, value);
        }
    }

    private String toHms(String value) {
        if (value == null) {
            return value;
        }
        // Convert to seconds ("3 hr, 30 min, 5 sec" -> "12605")
        if (value.endsWith("min")) {
            value = value + " 0 sec";
        }
        String[] tokens = value.split("[, ]+");
        int len = tokens.length;
        if (len == 2 || len == 4 || len == 6) {
            int value_sec = 0;
            int placeValue = 1; // Sexagesimal place value (seconds = 1)
            try {
                for (int i = len - 2; i >= 0; i -= 2, placeValue *= 60) {
                    value_sec += Integer.parseInt(tokens[i]) * placeValue;
                }
                value = Integer.toString(value_sec);
            } catch (NumberFormatException nfe) {}
        }

        // If we now have only a single number - convert to desired format
        if (value.matches("[0-9\\.]+")) {
            try {
                int t_hr = 0;
                int t_min = 0;
                double sec = Double.parseDouble(value);
                int t_sec = (int)Math.round(sec);
                //int t_sec = Integer.parseInt(value);
                StringBuffer sb = new StringBuffer();
                t_min = t_sec / 60;
                t_sec = t_sec % 60;
                t_hr = t_min / 60;
                t_min = t_min % 60;
                sb.insert(0, Fmt.d(2, t_sec, false, '0'));
                sb.insert(0, ":");
                if (t_hr == 0) {
                    sb.insert(0, t_min);
                } else {
                    sb.insert(0, Fmt.d(2, t_min, false, '0'));
                    sb.insert(0, ":");
                    sb.insert(0, t_hr);
                }
                value = sb.toString();
            } catch (NumberFormatException nfe) {}
        }
        return value.trim();
    }

    /**
     * Returns true if the given status is one that is expected to have
     * an assigned color in the Display Options.
     * @param status The specified status.
     * @return True if the status has its own color.
     */
    public static boolean hasColorAssigned(String status) {
        return SQ_READY.equals(status)
                || SQ_EXECUTING.equals(status)
                || SQ_SKIPPED.equals(status)
                || SQ_COMPLETED.equals(status)
                || SQ_QUEUED.equals(status)
                || SQ_ERROR.equals(status)
                || SQ_ACTIVE.equals(status)
                || SQ_CUSTOMIZED.equals(status);
    }

    private void setNodeBackground(String status) {
        Color color = ProtocolBuilder.getBgForNode(null, m_model);
        if (color != null) {
            // The color has been set explicitly on this node
            setBackground(color);
        } else if ("SampInfo".equalsIgnoreCase(getAttribute("macro"))) {
            setBackground(m_defaultBg);
        } else if (hasColorAssigned(status)) {
            // Make the color match the status state
            String colorKey = status;
            if (isNightQueue()
                    && (SQ_READY.equals(status)
                            || SQ_CUSTOMIZED.equals(status)
                            || SQ_QUEUED.equals(status)))
            {
                colorKey += "Night";
            }
            color = DisplayOptions.getColor("SQ" + colorKey + "Bg");
            setBackground(color);
        } else {
            setBackground(m_defaultBg);
        }
    }

    private boolean isNightQueue() {
        boolean rtn = getAttribute(ATTR_TITLE).toLowerCase().contains("night")
             || getAttribute(ATTR_NIGHT).toLowerCase().startsWith("y");
        return rtn;
    }

    // NB: Future: use this to set progressBar position
    ///**
    // * Set the position of the progress bar background.
    // * @param progress The current amount of progress, between 0 and 1.
    // */
    //public void setProgress(double progress) {
    //    if (progress >= 0) {
    //        super.setIndeterminate(false);
    //        super.setValue((int)(progress * m_progressResolution));
    //    } else {
    //        super.setIndeterminate(true);
    //    }
    //}

    // NB: Future: use this to set progressBar color
    //private void setBarColor(Color c) {
    //    super.setForeground(c);
    //}

    private void setBorder(boolean isSelected) {
        if (isSquished()) {
            if (isSelected) {
                setBorder(m_selSquishBorder);
            } else {
                setBorder(m_idleSquishBorder);
            }
        } else {
            if (isSelected) {
                setBorder(m_selBorder);
            } else {
                setBorder(m_idleBorder);
            }
        }
    }

    @Override
    public String toString() {
        if (m_model == null) {
            return "null";
        } else {
            return getId();
        }
    }

    @Override
    public Dimension getPreferredSize() {
        if (isSquished()) {
            int count = 0;
            try {
                count = Integer.parseInt(getAttribute(ATTR_SQUISH_COUNT));
            } catch (NumberFormatException nfe) {
            }
            int height = (count >= MAX_SQUISH) ? 0 : 4;
            return new Dimension(m_widgetSize.width, height);
        }
        return m_widgetSize;
    }

    /**
     * See if this node is to be displayed with minimal size.
     * @return True if it should be displayed squished.
     */
    private boolean isSquished() {
        return "true".equals(getAttribute(ATTR_SQUISHED));
    }

    public Dimension getRequiredSize() {
        Dimension dim = m_label.getPreferredSize();
        dim.width += LABEL_MARGIN;
        dim.height += 1;
        return dim;
    }

    // For testing:
//    private static int m_count = 0;
//    public void paintComponent(java.awt.Graphics g) {
//        super.paintComponent(g);
//        Messages.postDebug("SQPaint",
//                           "paintComponent " + this
//                           + ", x=" + getX()
//                           + ", count=" + ++m_count);
//    }/*TMP*/


    class MyLayout implements LayoutManager {

        @Override
        public void addLayoutComponent(String target, Component arg1) {
        }

        /**
         * Layout for this Action widget.
         */
        @Override
        public void layoutContainer(Container target) {
            Messages.postDebug("SQPaint", "SQActionRenderer.layoutContainer(): "
                               + target);
            synchronized (target.getTreeLock()) {
                Dimension targetSize = target.getPreferredSize();
                target.setSize(targetSize);
                Insets insets = target.getInsets();

                int x0 = insets.left + 2;
                int y0 = insets.top;
                Dimension labelDim = m_label.getPreferredSize();
                m_label.setBounds(x0, y0, labelDim.width, labelDim.height);
            }
        }

        @Override
        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        @Override
        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        @Override
        public void removeLayoutComponent(Component target) {
        }

    }

    /**
     * The "max size" is the height of the widget and the
     * width of a widget with no left indent. The actual width of
     * any widget is (max size) - (left indent).
     * This makes the right-hand end of all the widgets line up at
     * x=(max widget width).
     * @param dim The new size, or null to force recalculation of size.
     */
    public static void setMaxSize(Dimension dim) {
        m_maxWidgetSize = dim;
    }

    public static Dimension getMaxSize() {
        return m_maxWidgetSize;
    }

}
