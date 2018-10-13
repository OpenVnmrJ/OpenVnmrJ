/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import javax.swing.*;
import javax.swing.text.*;
import java.awt.*;
import java.util.*;
import java.awt.event.*;
import vnmr.bo.*;


public class VnmrJLogWindow extends JFrame implements ActionListener {
    public static final int COMMAND = 1;
    public static final int PARAM = 2;
    public static final int QUERY = 3;
    public static final int UICMD = 4;
    public static final int SHOWLOG = 5;
    public static final int ALL = 6;
    public static final int STATUS_NUM = 6;

    private static VnmrJLogWindow instance = null;
    private static boolean doExit = false;

    private java.util.List <VjLogListener> listenerList = null;
    private boolean[] status;

    private JCheckBox cmdChk;
    private JCheckBox paramChk;
    private JCheckBox queryChk;
    private JCheckBox uicmdChk;
    private JCheckBox topChk;
    private JTextPane textPan;
    private JScrollBar vScrollBar = null;
    private Runnable scrollUpdater;
    private StyledDocument textDoc;
    private Style  plainStyle;
    private Style  cmdStyle;
    private Style  queryStyle;
    private Style  paramStyle;
    private Style  uicmdStyle;
    private Style  markStyle;
    
    public static VnmrJLogWindow getInstance() {
        if (instance == null)
            instance = new VnmrJLogWindow();
        return instance;
    }

    public VnmrJLogWindow() {
        super("VnmrJ Log");
        status = new boolean[STATUS_NUM];
        for (int i = 0; i < STATUS_NUM; i++)
            status[i] = false;
        status[COMMAND] = true;
        listenerList = Collections.synchronizedList(new LinkedList<VjLogListener>());
        buildGUi();
        setAlwaysOnTop(true);
    }

    private void updateScrollBar() {
        int vMax = vScrollBar.getMaximum();
        vScrollBar.setValue(vMax);
    }

    private void buildGUi() {
        
        Container cont = getContentPane();
        JPanel ctrlPanel = new JPanel(); 
        
        cont.add(ctrlPanel, BorderLayout.NORTH);
        ctrlPanel.setLayout(new SimpleHLayout());

        cmdChk = new JCheckBox("Command");
        paramChk = new JCheckBox("Changed Parameters");
        queryChk = new JCheckBox("Value Set");
        uicmdChk = new JCheckBox("VnmrjCmd");
        
        ctrlPanel.add(cmdChk);
        ctrlPanel.add(paramChk);
        ctrlPanel.add(queryChk);
        ctrlPanel.add(uicmdChk);

        cmdChk.setActionCommand("cmd");
        paramChk.setActionCommand("param");
        queryChk.setActionCommand("query");
        uicmdChk.setActionCommand("vjcmd");
        cmdChk.setSelected(true);

        textPan = new JTextPane();
        textDoc = textPan.getStyledDocument();

        JScrollPane scrollPane = new JScrollPane(textPan,
                            JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                            JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);

        cont.add(scrollPane, BorderLayout.CENTER);

        vScrollBar = scrollPane.getVerticalScrollBar();

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent event) {
                 if (doExit)
                    System.exit(0);
                 else
                    processLogEvent(SHOWLOG, false);
            }

            public void windowOpened(WindowEvent event) {
                 // processLogEvent(SHOWLOG, true);
            }

        });

        cmdChk.addActionListener(this);
        paramChk.addActionListener(this);
        queryChk.addActionListener(this);
        uicmdChk.addActionListener(this);

        JPanel bottomPanel = new JPanel();
        bottomPanel.setBorder(BorderFactory.createEmptyBorder(0, 5, 10, 5));
        bottomPanel.setLayout(new SimpleH2Layout(SimpleH2Layout.CENTER, 20, 5, false));

        topChk = new JCheckBox("Stay on Top");
        topChk.setSelected(true);
        topChk.setActionCommand("top");
        topChk.addActionListener(this);
        JButton markButton = new JButton(Util.getLabel("Mark"));
        markButton.setActionCommand("mark");
        markButton.addActionListener(this);
        JButton clearButton = new JButton(Util.getLabel("Clear"));
        clearButton.setActionCommand("clear");
        clearButton.addActionListener(this);
        JButton closeButton = new JButton(Util.getLabel("Close"));
        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        bottomPanel.add(topChk);
        bottomPanel.add(markButton);
        bottomPanel.add(clearButton);
        bottomPanel.add(closeButton);

        cont.add(bottomPanel, BorderLayout.SOUTH);

        addStylesToDocument();

        scrollUpdater = new Runnable() {
           public void run() {
                  updateScrollBar();
           }
        };

        if (!DebugOutput.isSetFor("traceXML"))  // debug mode
        {
            // uicmdChk.setVisible(false);
        }

        Dimension ds = Toolkit.getDefaultToolkit().getScreenSize();
        int w = ds.width / 3;
        int h = ds.height - 200;
        setSize(w, h);
        setLocation(10, 30);
    }

    public void append(int type, int id, VObjIF obj, String mess) {
        if (type < 1 || type > UICMD)
            return;
        if (!status[type])
            return;
        int vp = id + 1;
        Style  s = cmdStyle;
        StringBuffer sb = new StringBuffer();
        switch (type) {
            case COMMAND:
                    sb.append("  cmd ");
                    break;
            case PARAM:
                    sb.append("  parameters ");
                    s = paramStyle;
                    break;
            case QUERY:
                    sb.append("  value ");
                    s = queryStyle;
                    break;
            case UICMD:
                    sb.append("  vnmrjcmd ");
                    s = uicmdStyle;
                    break;

        }
        sb.append("(vp ").append(vp);
        if (obj != null) {
            String name = obj.getClass().getName();
            int n = name.lastIndexOf('.');
            if (n > 1) {
               n++;
               if (name.charAt(n) == 'V')
                  n++;
               sb.append(" ").append(name.substring(n));
            }
            else
               sb.append(" ").append(name);
            String label = obj.getAttribute(VObjDef.PANEL_NAME);
            if (label != null && label.length() > 1)
               sb.append(" ").append(label);
            else {
               label = obj.getAttribute(VObjDef.LABEL);
               if (label != null)
                   sb.append(" ").append(label);
            }
        }
        sb.append("): \n");
        try {
            textDoc.insertString(textDoc.getLength(), sb.toString(), s);
            textDoc.insertString(textDoc.getLength(), mess, plainStyle);
            textDoc.insertString(textDoc.getLength(), "\n", plainStyle);
        }
        catch (BadLocationException e) { }
        SwingUtilities.invokeLater(scrollUpdater);
    }

    public void appendVbgCmd(int id, String mess) {
        if (!status[COMMAND])
            return;
        int vp = id + 1;
        String name = "  cmd (vp "+vp+" button):\n";
        try {
            textDoc.insertString(textDoc.getLength(), name, cmdStyle);
            textDoc.insertString(textDoc.getLength(), mess, plainStyle);
            textDoc.insertString(textDoc.getLength(), "\n", plainStyle);
        }
        catch (BadLocationException e) { }
    }

    private void addStylesToDocument() {
        Style def = StyleContext.getDefaultStyleContext().
                        getStyle(StyleContext.DEFAULT_STYLE);

        plainStyle = textDoc.addStyle("plain", def);
        StyleConstants.setFontFamily(plainStyle, "SansSerif");
        StyleConstants.setForeground(plainStyle, Color.black);

        cmdStyle = textDoc.addStyle("cmd", plainStyle);
        StyleConstants.setBold(cmdStyle, true);
        StyleConstants.setItalic(cmdStyle, true);
        StyleConstants.setForeground(cmdStyle, Color.blue);

        queryStyle = textDoc.addStyle("value", cmdStyle);
        StyleConstants.setBold(queryStyle, true);
        StyleConstants.setForeground(queryStyle, VnmrRgb.getColorByName("darkGreen"));

        paramStyle = textDoc.addStyle("param", cmdStyle);
        StyleConstants.setBold(paramStyle, true);
        StyleConstants.setForeground(paramStyle, VnmrRgb.getColorByName("brown"));

        markStyle = textDoc.addStyle("mark", plainStyle);
        StyleConstants.setBold(markStyle, true);
        StyleConstants.setForeground(markStyle, Color.red);

        uicmdStyle = textDoc.addStyle("uicmd", cmdStyle);
        StyleConstants.setBold(uicmdStyle, true);
        StyleConstants.setForeground(uicmdStyle, Color.green);
    }

    private void dispatchLogStatus(VjLogListener listener) {
        if (listener == null)
           return;

        for (int i = 1; i < STATUS_NUM; i++) {
           listener.setLogMode(i, status[i]);
        }

    }

    public void addEventListener(VjLogListener listener) {
        if (!listenerList.contains(listener)) {
            listenerList.add(listener);
        }
    }

    public void removeEventListener(VjLogListener listener) {
        listenerList.remove(listener);
    }

    private void processLogEvent(int type, boolean value) {
        status[type] = value;
        synchronized (listenerList) {
            Iterator itr = listenerList.iterator();
            while (itr.hasNext()) {
                VjLogListener l = (VjLogListener)itr.next();
                dispatchLogStatus(l);
            }
        }
    }

    public void setLogMode(int type, boolean value) {
        if (type < 1 || type >= STATUS_NUM)
            return;
        processLogEvent(type, value);
        if (type == SHOWLOG) {
            setVisible(value);
        }
        else if (value)
            setVisible(value);
        if (value) {
            if (getState() == Frame.ICONIFIED)
                setState(Frame.NORMAL);
            toFront();
        }
    }

    private void checkAction(JCheckBox obj, String cmd) {
        boolean value = obj.isSelected();
        int  type = 0;
       
        if (cmd.equals("top")) {
             setAlwaysOnTop(value);
             return;
        }
        if (cmd.equals("cmd"))
             type = COMMAND;
        else if (cmd.equals("param"))
             type = PARAM;
        else if (cmd.equals("query"))
             type = QUERY;
        else if (cmd.equals("vjcmd"))
             type = UICMD;
       
        setLogMode(type, value);
    }

    public void actionPerformed( ActionEvent e ) {
        String cmd = e.getActionCommand();
        Component src = (Component)e.getSource();

        if (src instanceof JCheckBox) {
            checkAction((JCheckBox) src, cmd);
            return;
        }
        if (cmd.equals("clear")) {
            textPan.setText("");
            return;
        }
        if (cmd.equals("close")) {
            processLogEvent(SHOWLOG, false);
            setVisible(false);
            return;
        }
        if (cmd.equals("mark")) {
           try {
              textDoc.insertString(textDoc.getLength(), "@@@@@@\n",
                                 markStyle);
           }
           catch (BadLocationException er) { }
        }
    }

    public static void main(String[] args) {
        VnmrJLogWindow window = VnmrJLogWindow.getInstance();
        doExit = true;
        window.setLocation(20, 20);
        window.setVisible(true);
    }

}
